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
#include <stdlib.h>
#include <string.h>
#include "ares.h"
#include "ares_dns.h"
#include "ares_private.h"

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <arpa/nameser.h>
#endif
#include <netdb.h>
#endif

int rares_parse_a_reply(const unsigned char *abuf, int alen,
		       struct hostent **host)
{
  unsigned int qdcount, ancount;
  int status, i, len, rr_type, rr_class, rr_len, naddrs;
  int naliases;
  const unsigned char *aptr;
  char *hostname, *rr_name, *rr_data, **aliases;
  struct in_addr *addrs;
  struct hostent *hostent;

  /* Set *host to NULL for all failure cases. */
  *host = NULL;

  /* Give up if abuf doesn't have room for a header. */
  if (alen < HFIXEDSZ)
    return RARES_EBADRESP;

  /* Fetch the question and answer count from the header. */
  qdcount = RARES_DNS_HEADER_QDCOUNT(abuf);
  ancount = RARES_DNS_HEADER_ANCOUNT(abuf);
  if (qdcount != 1)
    return RARES_EBADRESP;

  /* Expand the name from the question, and skip past the question. */
  aptr = abuf + HFIXEDSZ;
  status = rares_expand_name(aptr, abuf, alen, &hostname, &len);
  if (status != RARES_SUCCESS)
    return status;
  if (aptr + len + QFIXEDSZ > abuf + alen)
    {
      free(hostname);
      return RARES_EBADRESP;
    }
  aptr += len + QFIXEDSZ;

  /* Allocate addresses and aliases; ancount gives an upper bound for both. */
  addrs = malloc(ancount * sizeof(struct in_addr));
  if (!addrs)
    {
      free(hostname);
      return RARES_ENOMEM;
    }
  aliases = malloc((ancount + 1) * sizeof(char *));
  if (!aliases)
    {
      free(hostname);
      free(addrs);
      return RARES_ENOMEM;
    }
  naddrs = 0;
  naliases = 0;

  /* Examine each answer resource record (RR) in turn. */
  for (i = 0; i < (int)ancount; i++)
    {
      /* Decode the RR up to the data field. */
      status = rares_expand_name(aptr, abuf, alen, &rr_name, &len);
      if (status != RARES_SUCCESS)
	break;
      aptr += len;
      if (aptr + RRFIXEDSZ > abuf + alen)
	{
	  status = RARES_EBADRESP;
	  break;
	}
      rr_type = RARES_DNS_RR_TYPE(aptr);
      rr_class = RARES_DNS_RR_CLASS(aptr);
      rr_len = RARES_DNS_RR_LEN(aptr);
      aptr += RRFIXEDSZ;

      if (rr_class == C_IN && rr_type == T_A
	  && rr_len == sizeof(struct in_addr)
	  && strcasecmp(rr_name, hostname) == 0)
	{
	  memcpy(&addrs[naddrs], aptr, sizeof(struct in_addr));
	  naddrs++;
	  status = RARES_SUCCESS;
	}

      if (rr_class == C_IN && rr_type == T_CNAME)
	{
	  /* Record the RR name as an alias. */
	  aliases[naliases] = rr_name;
	  naliases++;

	  /* Decode the RR data and replace the hostname with it. */
	  status = rares_expand_name(aptr, abuf, alen, &rr_data, &len);
	  if (status != RARES_SUCCESS)
	    break;
	  free(hostname);
	  hostname = rr_data;
	}
      else
	free(rr_name);

      aptr += rr_len;
      if (aptr > abuf + alen)
	{
	  status = RARES_EBADRESP;
	  break;
	}
    }

  if (status == RARES_SUCCESS && naddrs == 0)
    status = RARES_ENODATA;
  if (status == RARES_SUCCESS)
    {
      /* We got our answer.  Allocate memory to build the host entry. */
      aliases[naliases] = NULL;
      hostent = malloc(sizeof(struct hostent));
      if (hostent)
	{
	  hostent->h_addr_list = malloc((naddrs + 1) * sizeof(char *));
	  if (hostent->h_addr_list)
	    {
	      /* Fill in the hostent and return successfully. */
	      hostent->h_name = hostname;
	      hostent->h_aliases = aliases;
	      hostent->h_addrtype = AF_INET;
	      hostent->h_length = sizeof(struct in_addr);
	      for (i = 0; i < naddrs; i++)
		hostent->h_addr_list[i] = (char *) &addrs[i];
	      hostent->h_addr_list[naddrs] = NULL;
	      *host = hostent;
	      return RARES_SUCCESS;
	    }
	  free(hostent);
	}
      status = RARES_ENOMEM;
    }
  for (i = 0; i < naliases; i++)
    free(aliases[i]);
  free(aliases);
  free(addrs);
  free(hostname);
  return status;
}
