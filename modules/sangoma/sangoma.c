/*$Id: sangoma.c 8721 2012-02-20 10:57:44Z razvancrainea $
 *
 * Copyright (C) 2012 Orban Szabolcs 
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
 *
 * History:
 */

#include "../../sr_module.h"

static int my_print_f(struct sip_msg *msg, char *param);
static int mod_init(void);


static cmd_export_t cmds[]={
	{"print", (cmd_function)my_print_f, 0, 0, 0, REQUEST_ROUTE|STARTUP_ROUTE },
  {"print", (cmd_function)my_print_f, 1, 0, 0, REQUEST_ROUTE|STARTUP_ROUTE },
	{0,0,0,0,0,0}
};


struct module_exports exports= {
	"sangoma",  /* module name*/
	MODULE_VERSION,
	DEFAULT_DLFLAGS, /* dlopen flags */
	cmds,       /* exported functions */
	0,          /* module parameters */
	0,          /* exported statistics */
	0,          /* exported MI functions */
	0,          /* exported pseudo-variables */
	0,          /* extra processes */
	mod_init,   /* module initialization function */
	0,          /* response function */
	0,          /* destroy function */
	0,          /* per-child init function */
};


static int mod_init(void)
{
	LM_INFO("initializing...\n");
	return 0;
}


static int my_print_f(struct sip_msg *msg, char *param)
{
  if (param != NULL)
  {
    LM_DBG("Hello, %s!\n", param);
  }
  else
  {
    LM_DBG("Hello, world of sangoma!\n");
  }
  return 1;
}
