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

static int mod_init(void);
static int sgm_start_session(struct sip_msg *, char *);
static int sgm_end_session(struct sip_msg *, char *);
static int sgm_join_session(struct sip_msg *, char *);

static cmd_export_t cmds[]={
	{"sgm_start_session", (cmd_function)sgm_start_session, 1, 0, 0,
      REQUEST_ROUTE|BRANCH_ROUTE|ONREPLY_ROUTE },
  {"sgm_end_session", (cmd_function)sgm_end_session, 1, 0, 0, 
      REQUEST_ROUTE|BRANCH_ROUTE|ONREPLY_ROUTE },
  {"sgm_join_session", (cmd_function)sgm_join_session, 1, 0, 0,
      REQUEST_ROUTE|BRANCH_ROUTE|ONREPLY_ROUTE },
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

static int sgm_start_session(struct sip_msg *msg, char *param)
{
  unsigned short port_rtp_a, port_rtcp_a;
  long i;
  int parseRez = parse_sdp(msg);

  struct sdp_info *sdp_body;
  struct sdp_session_cell *session;
  struct sdp_stream_cell *stream;
  struct sdp_payload_attr *payload;

  if (parseRez < 0)
  {
    LM_ERR("Error parsing SIP message...transcoding not possible");
    return -1;
  }
  LM_DBG("Parsed SIP message (SDP body)");

  sdp_body = msg->sdp;
  session = sdp_body->sessions;

  while (session)
  {
    LM_DBG("Session num: %d\n", session->session_num);
    LM_DBG("IP: %.*s\n", session->ip_addr.len, session->ip_addr.s);
    LM_DBG("Number of streams: %d\n", session->streams_num);
    stream = session->streams;
    while (stream)
    {
      LM_DBG("Stream num: %d\n", stream->stream_num);
      LM_DBG("IP: %.*s\n", stream->ip_addr.len, stream->ip_addr.s);
      LM_DBG("RTP: %d\n", stream->is_rtp);
      LM_DBG("Media: %.*s\n", stream->media.len, stream->media.s);
      LM_DBG("Port: %.*s\n", stream->port.len, stream->port.s);
      LM_DBG("Payloads Num: %d", stream->payloads_num);
      payload = stream->payload_attr;
      while (payload)
      {
        LM_DBG("Payload num: %d\n", payload->payload_num);
        LM_DBG("Rtp payload: %.*s\n", payload->rtp_payload.len, payload->rtp_payload.s);
        LM_DBG("Rtp encoding: %.*s\n", payload->rtp_enc.len, payload->rtp_enc.s);
        LM_DBG("Rtp clock: %.*s\n", payload->rtp_clock.len, payload->rtp_clock.s);
        LM_DBG("Rtp params: %.*s\n", payload->rtp_params.len, payload->rtp_params.s);
        LM_DBG("Fmtp string: %.*s\n", payload->fmtp_string.len, payload->fmtp_string.s);
        payload = payload->next;
      }
      stream = stream->next;
    }
    session = session->next;
  }
}


static int sgm_end_session(struct sip_msg *msg, char *param)
{
  return 0;
}

static int sgm_join_session(struct sip_msg *msg, char *param)
{
  return 0;
}

static int mod_init(void)
{
	return 0;
}

