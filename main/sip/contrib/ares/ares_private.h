/* $Id: ares_private.h,v 1.4 2003/09/14 01:13:34 fluffy Exp $ */

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

#include <sys/types.h>
#ifndef WIN32
#include <netinet/in.h>
#else
#include <errno.h>
#include <winsock2.h>
#include <io.h>
#endif

#include <string.h>
#include <stdio.h>


#define	DEFAULT_TIMEOUT		5
#define DEFAULT_TRIES		4
#ifndef INADDR_NONE
#define	INADDR_NONE 0xffffffff
#endif

#define PATH_RESOLV_CONF	"/etc/resolv.conf"
#ifdef ETC_INET
#define PATH_HOSTS		"/etc/inet/hosts"
#else
#define PATH_HOSTS		"/etc/hosts"
#endif

struct send_request {
  /* Remaining data to send */
  const char *data;
  int len;

  /* Next request in queue */
  struct send_request *next;
};


struct server_state {
  struct in_addr addr;
  int udp_socket;
  int tcp_socket;

  /* Mini-buffer for reading the length word */
  unsigned char tcp_lenbuf[2];
  int tcp_lenbuf_pos;
  int tcp_length;

  /* Buffer for reading actual TCP data */
  unsigned char *tcp_buffer;
  int tcp_buffer_pos;

  /* TCP output queue */
  struct send_request *qhead;
  struct send_request *qtail;
};

struct query {
  /* Query ID from qbuf, for faster lookup, and current timeout */
  unsigned short qid;
  time_t timeout;

  /* Query buf with length at beginning, for TCP transmission */
  char *tcpbuf;
  int tcplen;

  /* Arguments passed to ares_send() (qbuf points into tcpbuf) */
  const char *qbuf;
  int qlen;
  ares_callback callback;
  void *arg;

  /* Query status */
  int try;
  int server;
  int *skip_server;
  int using_tcp;
  int error_status;

  /* Next query in chain */
  struct query *next;
};

/* An IP address pattern; matches an IP address X if X & mask == addr */
struct apattern {
  struct in_addr addr;
  struct in_addr mask;
};

struct ares_channeldata {
  /* Configuration data */
  int flags;
  int timeout;
  int tries;
  int ndots;
  int udp_port;
  int tcp_port;
  char **domains;
  int ndomains;
  struct apattern *sortlist;
  int nsort;
  char *lookups;

  /* Server addresses and communications state */
  struct server_state *servers;
  int nservers;

  /* ID to use for next query */
  unsigned short next_id;

  /* Active queries */
  struct query *queries;
};

void ares__send_query(ares_channel channel, struct query *query, time_t now);
void ares__close_sockets(struct server_state *server);
int ares__get_hostent(FILE *fp, struct hostent **host);
int ares__read_line(FILE *fp, char **buf, int *bufsize);

#ifdef WIN32
#define strcasecmp(a,b) stricmp(a,b)
#define strcasencmp(a,b,n) strnicmp(a,b,n)
#endif

