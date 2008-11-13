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

#ifndef WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <arpa/nameser.h>
#endif
#include <netdb.h>
#endif

#include "ares.h"
#include "ares_dns.h"
#include "ares_private.h"

int rares_parse_ptr_reply(const unsigned char *abuf, int alen, const void *addr,
			 int addrlen, int family, struct hostent **host)
{
  unsigned int qdcount, ancount;
  int status, i, len, rr_type, rr_class, rr_len;
  const unsigned char *aptr;
  char *ptrname, *hostname, *rr_name, *rr_data;
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
  status = rares_expand_name(aptr, abuf, alen, &ptrname, &len);
  if (status != RARES_SUCCESS)
    return status;
  if (aptr + len + QFIXEDSZ > abuf + alen)
    {
      free(ptrname);
      return RARES_EBADRESP;
    }
  aptr += len + QFIXEDSZ;

  /* Examine each answer resource record (RR) in turn. */
  hostname = NULL;
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

      if (rr_class == C_IN && rr_type == T_PTR
	  && strcasecmp(rr_name, ptrname) == 0)
	{
	  /* Decode the RR data and set hostname to it. */
	  status = rares_expand_name(aptr, abuf, alen, &rr_data, &len);
	  if (status != RARES_SUCCESS)
	    break;
	  if (hostname)
	    free(hostname);
	  hostname = rr_data;
	}

      if (rr_class == C_IN && rr_type == T_CNAME)
	{
	  /* Decode the RR data and replace ptrname with it. */
	  status = rares_expand_name(aptr, abuf, alen, &rr_data, &len);
	  if (status != RARES_SUCCESS)
	    break;
	  free(ptrname);
	  ptrname = rr_data;
	}

      free(rr_name);
      aptr += rr_len;
      if (aptr > abuf + alen)
	{
	  status = RARES_EBADRESP;
	  break;
	}
    }

  if (status == RARES_SUCCESS && !hostname)
    status = RARES_ENODATA;
  if (status == RARES_SUCCESS)
    {
      /* We got our answer.  Allocate memory to build the host entry. */
      hostent = malloc(sizeof(struct hostent));
      if (hostent)
	{
	  hostent->h_addr_list = malloc(2 * sizeof(char *));
	  if (hostent->h_addr_list)
	    {
	      hostent->h_addr_list[0] = malloc(addrlen);
	      if (hostent->h_addr_list[0])
		{
		  hostent->h_aliases = malloc(sizeof (char *));
		  if (hostent->h_aliases)
		    {
		      /* Fill in the hostent and return successfully. */
		      hostent->h_name = hostname;
		      hostent->h_aliases[0] = NULL;
		      hostent->h_addrtype = family;
		      hostent->h_length = addrlen;
		      memcpy(hostent->h_addr_list[0], addr, addrlen);
		      hostent->h_addr_list[1] = NULL;
		      *host = hostent;
		      free(ptrname);
		      return RARES_SUCCESS;
		    }
		  free(hostent->h_addr_list[0]);
		}
	      free(hostent->h_addr_list);
	    }
	  free(hostent);
	}
      status = RARES_ENOMEM;
    }
  if (hostname)
    free(hostname);
  free(ptrname);
  return status;
}
