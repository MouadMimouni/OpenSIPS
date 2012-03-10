/*
 * $Id: ratelimit.c 8109 2011-06-30 18:16:54Z bogdan_iancu $
 *
 * ratelimit module
 *
 * Copyright (C) 2006 Hendrik Scholz <hscholz@raisdorf.net>
 * Copyright (C) 2008 Ovidiu Sas <osas@voipembedded.com>
 *
 * This file is part of opensips, a free SIP server.
 *
 * opensips is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version
 *
 * opensips is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License 
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * History:
 * ---------
 *
 * 2008-01-10 ported from SER project (osas)
 * 2008-01-16 ported enhancements from openims project (osas) 
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <regex.h>
#include <math.h>

#include "../../sr_module.h"
#include "../../mem/mem.h"
#include "../../mem/shm_mem.h"
#include "../../dprint.h"
#include "../../timer.h"
#include "../../ut.h"
#include "../../locking.h"
#include "../../mod_fix.h"
#include "../../data_lump.h"
#include "../../data_lump_rpl.h"
#include "../../socket_info.h"
#include "../signaling/signaling.h"
#include "ratelimit.h"


/* === these change after startup */
gen_lock_t * rl_lock;

static double * rl_load_value;     /* actual load, used by PIPE_ALGO_FEEDBACK */
static double * pid_kp, * pid_ki, * pid_kd, * pid_setpoint; /* PID tuning params */
static int * drop_rate;         /* updated by PIPE_ALGO_FEEDBACK */
static int *rl_feedback_limit;

int * rl_network_load;	/* network load */
int * rl_network_count;	/* flag for counting network algo users */

/* these only change in the mod_init() process -- no locking needed */
int rl_timer_interval = RL_TIMER_INTERVAL;

static str db_url = {0,0};
str db_prefix = str_init("rl_pipe_");

/* === */

#ifndef RL_DEBUG_LOCKS
# define LOCK_GET lock_get
# define LOCK_RELEASE lock_release
#else
# define LOCK_GET(l) do { \
	LM_INFO("%d: + get\n", __LINE__); \
	lock_get(l); \
	LM_INFO("%d: - get\n", __LINE__); \
} while (0)

# define LOCK_RELEASE(l) do { \
	LM_INFO("%d: + release\n", __LINE__); \
	lock_release(l); \
	LM_INFO("%d: - release\n", __LINE__); \
} while (0)
#endif

/* module functions */
static int mod_init(void);
static int mod_child(int);

/* fixup prototype */
static int fixup_rl_check(void **param, int param_no);

struct mi_root* mi_stats(struct mi_root* cmd_tree, void* param);
struct mi_root* mi_reset_pipe(struct mi_root* cmd_tree, void* param);
struct mi_root* mi_set_pid(struct mi_root* cmd_tree, void* param);
struct mi_root* mi_get_pid(struct mi_root* cmd_tree, void* param);


static cmd_export_t cmds[] = {
	{"rl_check", (cmd_function)w_rl_check_2, 2,
		fixup_rl_check, 0, REQUEST_ROUTE|LOCAL_ROUTE},
	{"rl_check", (cmd_function)w_rl_check_3, 3,
		fixup_rl_check, 0, REQUEST_ROUTE|LOCAL_ROUTE},
	{"rl_dec_count", (cmd_function)w_rl_dec, 1,
		fixup_spve_null, 0, REQUEST_ROUTE|LOCAL_ROUTE},
	{"rl_reset_count", (cmd_function)w_rl_reset, 1,
		fixup_spve_null, 0, REQUEST_ROUTE|LOCAL_ROUTE},
	{0,0,0,0,0,0}
};

static param_export_t params[] = {
	{ "timer_interval",		INT_PARAM,				 &rl_timer_interval},
	{ "expire_time",		INT_PARAM,				 &rl_expire_time},
	{ "hash_size",			INT_PARAM,				 &rl_hash_size},
	{ "default_algorithm",	STR_PARAM,				 &rl_default_algo_s.s},
	{ "cachedb_url",		STR_PARAM,				 &db_url.s},
	{ "db_prefix",			STR_PARAM,				 &db_prefix.s},
	{ 0,					0,						0}
};

#define RLH1 "Params: [pipe] ; Lists the parameters and variabiles in the " \
	"ratelimit module; If no pipe is specified, all existing pipes are listed."
#define RLH2 "Params: pipe ; Resets the counter of a specified pipe."
#define RLH3 "Params: ki kp kd ; Sets the PID Controller parameters for the " \
	"Feedback Algorithm."
#define RLH4 "Params: none ; Gets the list of in use PID Controller parameters."

static mi_export_t mi_cmds [] = {
	{"rl_list",       RLH1, mi_stats,      0,                0, 0},
	{"rl_reset_pipe", RLH2, mi_reset_pipe, 0,                0, 0},
	{"rl_set_pid",    RLH3, mi_set_pid,    0,                0, 0},
	{"rl_get_pid",    RLH4, mi_get_pid,    MI_NO_INPUT_FLAG, 0, 0},
	{0,0,0,0,0,0}
};

struct module_exports exports= {
	"ratelimit",
	MODULE_VERSION,
	DEFAULT_DLFLAGS,	/* dlopen flags */
	cmds,
	params,
	0,					/* exported statistics */
	mi_cmds,			/* exported MI functions */
	0,					/* exported pseudo-variables */
	0,					/* extra processes */
	mod_init,			/* module initialization function */
	0,
	mod_destroy,		/* module exit function */
	mod_child			/* per-child init function */
};


/* not using /proc/loadavg because it only works when our_timer_interval == theirs */
int get_cpuload(void)
{
	static 
	long long o_user, o_nice, o_sys, o_idle, o_iow, o_irq, o_sirq, o_stl;
	long long n_user, n_nice, n_sys, n_idle, n_iow, n_irq, n_sirq, n_stl;
	static int first_time = 1;
	int scan_res;
	FILE * f = fopen("/proc/stat", "r");

	if (! f)
		return -1;
	scan_res = fscanf(f, "cpu  %lld%lld%lld%lld%lld%lld%lld%lld",
			&n_user, &n_nice, &n_sys, &n_idle, &n_iow, &n_irq, &n_sirq, &n_stl);
	fclose(f);

	if (scan_res <= 0) {
		LM_ERR("/proc/stat didn't contain expected values");
		return -1;
	}

	if (first_time) {
		first_time = 0;
		*rl_load_value = 0;
	} else {		
		long long d_total =	(n_user - o_user)	+ 
					(n_nice	- o_nice)	+ 
					(n_sys	- o_sys)	+ 
					(n_idle	- o_idle)	+ 
					(n_iow	- o_iow)	+ 
					(n_irq	- o_irq)	+ 
					(n_sirq	- o_sirq)	+ 
					(n_stl	- o_stl);
		long long d_idle =	(n_idle - o_idle);

		*rl_load_value = 1.0 - ((double)d_idle) / (double)d_total;
	}

	o_user	= n_user; 
	o_nice	= n_nice; 
	o_sys	= n_sys; 
	o_idle	= n_idle; 
	o_iow	= n_iow; 
	o_irq	= n_irq; 
	o_sirq	= n_sirq; 
	o_stl	= n_stl;
	
	return 0;
}

static double int_err = 0.0;
static double last_err = 0.0;

void pid_setpoint_limit(int limit)
{
	*pid_setpoint = 0.01 * (double)limit;
}

/* (*load_value) is expected to be in the 0.0 - 1.0 range
 * (expects rl_lock to be taken)
 */
void do_update_load(void)
{
	double err, dif_err, output;

	/* PID update */
	err = *pid_setpoint - *rl_load_value;

	dif_err = err - last_err;

	/*
	 * TODO?: the 'if' is needed so low cpu loads for 
	 * long periods (which can't be compensated by 
	 * negative drop rates) don't confuse the controller
	 *
	 * NB: - "err < 0" means "desired_cpuload < actual_cpuload"
	 *     - int_err is integral(err) over time
	 */
	if (int_err < 0 || err < 0)
		int_err += err;

	output =	(*pid_kp) * err + 
				(*pid_ki) * int_err + 
				(*pid_kd) * dif_err;
	last_err = err;

	*drop_rate = (output > 0) ? output  : 0;
}

#define RL_SHM_MALLOC(_p, _s) \
	do { \
		_p = shm_malloc((_s)); \
		if (!_p) { \
			LM_ERR("no more shm memory\n"); \
			return -1; \
		} \
		memset(_p, 0, (_s)); \
	} while (0)

#define RL_SHM_FREE(_p) \
	do { \
		if (_p) { \
			shm_free(_p); \
			_p = 0; \
		} \
	} while (0)

/* initialize ratelimit module */
static int mod_init(void)
{
	unsigned int n;

	LM_INFO("Ratelimit module - initializing ...\n");
	
	if (rl_timer_interval < 0) {
		LM_ERR("invalid timer interval\n");
		return -1;
	}
	if (rl_expire_time < 0) {
		LM_ERR("invalid expire time\n");
		return -1;
	}

	if (db_url.s) {
		db_url.len = strlen(db_url.s);
		db_prefix.len = strlen(db_prefix.s);
		LM_DBG("using CacheDB url: %s\n", db_url.s);
	}

	RL_SHM_MALLOC(rl_network_count, sizeof(int));
	RL_SHM_MALLOC(rl_network_load, sizeof(int));
	RL_SHM_MALLOC(rl_load_value, sizeof(double));
	RL_SHM_MALLOC(pid_kp, sizeof(double));
	RL_SHM_MALLOC(pid_ki, sizeof(double));
	RL_SHM_MALLOC(pid_kd, sizeof(double));
	RL_SHM_MALLOC(pid_setpoint, sizeof(double));
	RL_SHM_MALLOC(drop_rate, sizeof(int));
	RL_SHM_MALLOC(rl_feedback_limit, sizeof(int));

	/* init ki value for feedback algo */
	*pid_ki = -25.0;

	rl_lock = lock_alloc();
	if (!rl_lock) {
		LM_ERR("cannot alloc lock\n");
		return -1;
	}

	if (!lock_init(rl_lock)) {
		LM_ERR("failed to init lock\n");
		return -1;
	}

	/* register timer to reset counters */
	if (register_timer_process(rl_timer, NULL, rl_timer_interval,
				TIMER_PROC_INIT_FLAG) == NULL) {
		LM_ERR("could not register timer function\n");
		return -1;
	}

	/* if db_url is not used */
	for( n=0 ; n < 8 * sizeof(unsigned int) ; n++) {
		if (rl_hash_size==(1<<n))
			break;
		if (rl_hash_size<(1<<n)) {
			LM_WARN("hash_size is not a power "
					"of 2 as it should be -> rounding from %d to %d\n",
					rl_hash_size, 1<<(n-1));
			rl_hash_size = 1<<(n-1);
		}
	}

	if (init_rl_table(rl_hash_size) < 0) {
		LM_ERR("cannot allocate the table\n");
		return -1;
	}

	return 0;
}

static int mod_child(int rank)
{
	/* init the cachedb */
	if (db_url.s && db_url.len)
		return init_cachedb(&db_url);
	LM_DBG("db_url not set - using standard behaviour\n");
	return 0;
}

void mod_destroy(void)
{
	unsigned int i;
	if (rl_htable.maps) {
		for (i = 0; i < rl_htable.size; i++)
			map_destroy(rl_htable.maps[i], 0);
		shm_free(rl_htable.maps);
		rl_htable.maps = 0;
		rl_htable.size = 0;
	}
	if (rl_htable.locks) {
		lock_set_destroy(rl_htable.locks);
		lock_set_dealloc(rl_htable.locks);
		rl_htable.locks = 0;
		rl_htable.locks_no = 0;
	}
	if (rl_lock) {
		lock_destroy(rl_lock);
		lock_dealloc(rl_lock);
	}
	RL_SHM_FREE(rl_network_count);
	RL_SHM_FREE(rl_network_load);
	RL_SHM_FREE(rl_load_value);
	RL_SHM_FREE(pid_kp);
	RL_SHM_FREE(pid_ki);
	RL_SHM_FREE(pid_kd);
	RL_SHM_FREE(pid_setpoint);
	RL_SHM_FREE(drop_rate);
	RL_SHM_FREE(rl_feedback_limit);

	if (db_url.s && db_url.len)
		destroy_cachedb();
}


/* this is here to avoid using rand() ... which doesn't _always_ return
 * exactly what we want (see NOTES section in 'man 3 rand')
 */
int hash[100] = {18, 50, 51, 39, 49, 68, 8, 78, 61, 75, 53, 32, 45, 77, 31, 
	12, 26, 10, 37, 99, 29, 0, 52, 82, 91, 22, 7, 42, 87, 43, 73, 86, 70, 
	69, 13, 60, 24, 25, 6, 93, 96, 97, 84, 47, 79, 64, 90, 81, 4, 15, 63, 
	44, 57, 40, 21, 28, 46, 94, 35, 58, 11, 30, 3, 20, 41, 74, 34, 88, 62, 
	54, 33, 92, 76, 85, 5, 72, 9, 83, 56, 17, 95, 55, 80, 98, 66, 14, 16, 
	38, 71, 23, 2, 67, 36, 65, 27, 1, 19, 59, 89, 48};


/**
 * runs the pipe's algorithm
 * (expects rl_lock to be taken)
 * \return	-1 if drop needed, 1 if allowed
 */
int rl_pipe_check(rl_pipe_t *pipe)
{
	switch (pipe->algo) {
		case PIPE_ALGO_NOP:
			LM_ERR("no algorithm defined for this pipe\n");
			return 1;
		case PIPE_ALGO_TAILDROP:
			return (pipe->counter <= pipe->limit * rl_timer_interval) ?
				1 : -1;
		case PIPE_ALGO_RED:
			if (!pipe->load)
				return 1;
			return pipe->counter % pipe->load ? -1 : 1;
		case PIPE_ALGO_NETWORK:
			return pipe->load;
		case PIPE_ALGO_FEEDBACK:
			return (hash[pipe->counter % 100] < *drop_rate) ? -1 : 1;
		default:
			LM_ERR("ratelimit algorithm %d not implemented\n", pipe->algo);
	}
	return 1;
}

/*
 * MI functions
 *
 * mi_stats() dumps the current config/statistics
 */

/* mi function implementations */
struct mi_root* mi_stats(struct mi_root* cmd_tree, void* param)
{
	struct mi_root *rpl_tree;
	struct mi_node *node=NULL, *rpl=NULL;
	int len;
	char * p;

	node = cmd_tree->node.kids;

	rpl_tree = init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
	if (rpl_tree==0)
		return 0;
	rpl = &rpl_tree->node;

	if (rl_stats(rpl, &node->value)) {
		LM_ERR("cannoti mi print values\n");
		goto free;
	}

	LOCK_GET(rl_lock);
	p = int2str((unsigned long)(*drop_rate), &len);
	if (!(node = add_mi_node_child(rpl, MI_DUP_VALUE, "DROP_RATE", 9, p, len))) {
		LOCK_RELEASE(rl_lock);
		goto free;
	}

	LOCK_RELEASE(rl_lock);
	return rpl_tree;
free:
	free_mi_tree(rpl_tree);
	return 0;
}

struct mi_root* mi_set_pid(struct mi_root* cmd_tree, void* param)
{
	struct mi_node *node;
	char buf[5];
	int rl_ki, rl_kp, rl_kd;

	if (!(node = cmd_tree->node.kids))
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	if ( !node->value.s || !node->value.len || node->value.len >= 5)
		goto bad_syntax;

	memcpy(buf, node->value.s, node->value.len);
	buf[node->value.len] = '\0';
	rl_ki = strtod(buf, NULL);

	node = node->next;
	if ( !node->value.s || !node->value.len || node->value.len >= 5)
		goto bad_syntax;
	memcpy(buf, node->value.s, node->value.len);
	buf[node->value.len] = '\0';
	rl_kp = strtod(buf, NULL);

	node = node->next;
	if ( !node->value.s || !node->value.len || node->value.len >= 5)
		goto bad_syntax;
	memcpy(buf, node->value.s, node->value.len);
	buf[node->value.len] = '\0';
	rl_kd = strtod(buf, NULL);

	LOCK_GET(rl_lock);
	*pid_ki = rl_ki;
	*pid_kp = rl_kp;
	*pid_kd = rl_kd;
	LOCK_RELEASE(rl_lock);

	return init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
bad_syntax:
	return init_mi_tree( 400, MI_BAD_PARM_S, MI_BAD_PARM_LEN);
}

struct mi_root* mi_get_pid(struct mi_root* cmd_tree, void* param)
{
	struct mi_root *rpl_tree;
	struct mi_node *node=NULL, *rpl=NULL;
	struct mi_attr* attr;

	rpl_tree = init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
	if (rpl_tree==0)
		return 0;
	rpl = &rpl_tree->node;
	node = add_mi_node_child(rpl, 0, "PID", 3, 0, 0);
	if(node == NULL)
		goto error;
	LOCK_GET(rl_lock);
	attr= addf_mi_attr(node, 0, "ki", 2, "%0.3f", *pid_ki);
	if(attr == NULL)
		goto error;
	attr= addf_mi_attr(node, 0, "kp", 2, "%0.3f", *pid_kp);
	if(attr == NULL)
		goto error;
	attr= addf_mi_attr(node, 0, "kd", 2, "%0.3f", *pid_kd);
	LOCK_RELEASE(rl_lock);
	if(attr == NULL)
		goto error;

	return rpl_tree;

error:
	LOCK_RELEASE(rl_lock);
	LM_ERR("Unable to create reply\n");
	free_mi_tree(rpl_tree);
	return 0;
}

struct mi_root* mi_reset_pipe(struct mi_root* cmd_tree, void* param)
{
	struct mi_node *node;

	if (!(node = cmd_tree->node.kids))
		return init_mi_tree( 400, MI_MISSING_PARM_S, MI_MISSING_PARM_LEN);

	if (w_rl_set_count(node->value, 0))
		return init_mi_tree( 400, MI_BAD_PARM_S, MI_BAD_PARM_LEN);

	return init_mi_tree( 200, MI_OK_S, MI_OK_LEN);
}


/* fixup functions */
static int fixup_rl_check(void **param, int param_no)
{
	switch (param_no) {
		/* pipe name */
		case 1:
			return fixup_spve(param);
			/* limit */
		case 2:
			return fixup_igp(param);
			/* algorithm */
		case 3:
			return fixup_sgp(param);
			/* error */
		default:
			LM_ERR("[BUG] too many params (%d)\n", param_no);
	}
	return E_UNSPEC;
}
