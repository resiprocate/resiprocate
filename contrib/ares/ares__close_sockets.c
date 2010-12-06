/* Copyright 1998 by the Massachusetts Institute of Technology.
 *
 * Permission to use, copy, modify, and distribute this
 * software and its documentation for any purpose and without
 * fee is hereby granted, provided that the above copyright
 * notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting
 * documentation, and that the name of M.I.T. not be used in
 * advertising or publicity pertaining to distribution of the
 * software without specific, written prior permission.
 * M.I.T. makes no representations about the suitability of
 * this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */

#include <stdlib.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "ares.h"
#include "ares_private.h"

void ares__close_poll(ares_channel channel, int server_idx) {
    if ( channel->poll_cb_func ) {
	struct server_state *server = &channel->servers[server_idx];
	if ( server->udp_socket!=-1 )
	    (*(channel->poll_cb_func))( channel->poll_cb_data,
		channel, server_idx, server->udp_socket, ARES_POLLACTION_CLOSE);
	if ( server->tcp_socket!=-1 )
	    (*(channel->poll_cb_func))( channel->poll_cb_data,
		channel, server_idx, server->udp_socket, ARES_POLLACTION_CLOSE);
    }
}

void ares__close_sockets(struct server_state *server)
{
  struct send_request *sendreq;

  /* Free all pending output buffers. */
  while (server->qhead)
    {
      /* Advance server->qhead; pull out query as we go. */
      sendreq = server->qhead;
      server->qhead = sendreq->next;
      free(sendreq);
    }
  server->qtail = NULL;

  /* Reset any existing input buffer. */
  if (server->tcp_buffer)
    free(server->tcp_buffer);
  server->tcp_buffer = NULL;
  server->tcp_lenbuf_pos = 0;

  /* Close the TCP and UDP sockets. */
  if (server->tcp_socket != -1)
    {
      ares__kill_socket(server->tcp_socket);
      server->tcp_socket = -1;
    }
  if (server->udp_socket != -1)
    {
      ares__kill_socket(server->udp_socket);
      server->udp_socket = -1;
    }
}
