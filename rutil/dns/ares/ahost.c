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

#ifndef WIN32
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ares.h"
#include "ares_dns.h"

#ifndef INADDR_NONE
#define	INADDR_NONE 0xffffffff
#endif

static void callback(void *arg, int status, struct hostent *host);
static void usage(void);

int main(int argc, char **argv)
{
  ares_channel channel;
  int status, nfds;
  fd_set read_fds, write_fds;
  struct timeval *tvp, tv;
  struct in_addr addr;

  if (argc == 0)
    usage();

  status = ares_init(&channel);
  if (status != ARES_SUCCESS)
    {
      fprintf(stderr, "ares_init: %s\n", ares_strerror(status));
      return 1;
    }

  /* Initiate the queries, one per command-line argument. */
  for (argv++; *argv; argv++)
  {
     if (inet_pton(AF_INET, *argv, &addr.s_addr) == 1)
     {
        ares_gethostbyaddr(channel, &addr, sizeof(addr), AF_INET, callback, *argv);
     }
     else
     {
        ares_gethostbyname(channel, *argv, AF_INET, callback, *argv);
     }
  }

  /* Wait for all queries to complete. */
  while (1)
    {
      FD_ZERO(&read_fds);
      FD_ZERO(&write_fds);
      nfds = ares_fds(channel, &read_fds, &write_fds);
      if (nfds == 0)
	break;
      tvp = ares_timeout(channel, NULL, &tv);
      select(nfds, &read_fds, &write_fds, NULL, tvp);
      ares_process(channel, &read_fds, &write_fds);
    }

  ares_destroy(channel);
  return 0;
}

static void callback(void *arg, int status, struct hostent *host)
{
  struct in_addr addr;
  char **p;

  if (status != ARES_SUCCESS)
    {
      fprintf(stderr, "%s: %s\n", (char *) arg, ares_strerror(status));
      return;
    }

  for (p = host->h_addr_list; *p; p++)
    {
      char addrBuf[INET_ADDRSTRLEN];
      memcpy(&addr, *p, sizeof(struct in_addr));
      inet_ntop(AF_INET, &addr, addrBuf, sizeof(addrBuf));
      printf("%-32s\t%s\n", host->h_name, addrBuf);
    }
}

static void usage(void)
{
  fprintf(stderr, "usage: ahost {host|addr} ...\n");
  exit(1);
}
