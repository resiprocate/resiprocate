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
#ifndef __CYGWIN__
#  include <arpa/nameser.h>
#endif
#endif

#include <stdlib.h>
#include "ares.h"
#include "ares_dns.h"
#include "ares_private.h"
#include "ares_local.h"

struct qquery {
  ares_callback callback;
  void *arg;
};

static void qcallback(void *arg, int status, unsigned char *abuf, int alen);

void rares_query(ares_channel channel, const char *name, int dnsclass,
		int type, ares_callback callback, void *arg)
{
  struct qquery *qquery;
  unsigned char *qbuf;
  int qlen, rd, status;

  /* See if query can be handled by local pseudo-domain DNS */
  if (rares_local_query(channel, name, dnsclass, type, callback, arg) != 0)
  {
      /* printf("rares_query: query for %s was handled locally\n",name); */
      return;
  }

  /* Compose the query. */
  rd = !(channel->flags & RARES_FLAG_NORECURSE);
  status = rares_mkquery(name, dnsclass, type, channel->next_id, rd, &qbuf,
			&qlen);
  channel->next_id++;
  if (status != RARES_SUCCESS)
    {
      callback(arg, status, NULL, 0);
      return;
    }

  /* Allocate and fill in the query structure. */
  qquery = malloc(sizeof(struct qquery));
  if (!qquery)
    {
      rares_free_string((char*)qbuf);
      callback(arg, RARES_ENOMEM, NULL, 0);
      return;
    }
  qquery->callback = callback;
  qquery->arg = arg;

  /* Send it off.  qcallback will be called when we get an answer. */
  rares_send(channel, qbuf, qlen, qcallback, qquery);
  rares_free_string((char*)qbuf);
}

static void qcallback(void *arg, int status, unsigned char *abuf, int alen)
{
  struct qquery *qquery = (struct qquery *) arg;
  unsigned int ancount;
  int rcode;

  if (status != RARES_SUCCESS)
    qquery->callback(qquery->arg, status, abuf, alen);
  else
    {
      /* Pull the response code and answer count from the packet. */
      rcode = RARES_DNS_HEADER_RCODE(abuf);
      ancount = RARES_DNS_HEADER_ANCOUNT(abuf);

      /* Convert errors. */
      switch (rcode)
	{
	case NOERROR:
	  status = (ancount > 0) ? RARES_SUCCESS : RARES_ENODATA;
	  break;
	case FORMERR:
	  status = RARES_EFORMERR;
	  break;
	case SERVFAIL:
	  status = RARES_ESERVFAIL;
	  break;
	case NXDOMAIN:
	  status = RARES_ENOTFOUND;
	  break;
	case NOTIMP:
	  status = RARES_ENOTIMP;
	  break;
	case REFUSED:
	  status = RARES_EREFUSED;
	  break;
	}
      qquery->callback(qquery->arg, status, abuf, alen);
    }
  free(qquery);
}
