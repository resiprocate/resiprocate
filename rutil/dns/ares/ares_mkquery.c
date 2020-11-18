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
#include <string.h>
#include "ares.h"
#include "ares_dns.h"

/* Header format, from RFC 1035:
 *                                  1  1  1  1  1  1
 *    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                      ID                       |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    QDCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ANCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    NSCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                    ARCOUNT                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * AA, TC, RA, and RCODE are only set in responses.  Brief description
 * of the remaining fields:
 *	ID	Identifier to match responses with queries
 *	QR	Query (0) or response (1)
 *	Opcode	For our purposes, always QUERY
 * 	RD	Recursion desired
 *	Z	Reserved (zero)
 *	QDCOUNT	Number of queries
 *	ANCOUNT	Number of answers
 *	NSCOUNT	Number of name server records
 *	ARCOUNT	Number of additional records
 *
 * Question format, from RFC 1035:
 *                                  1  1  1  1  1  1
 *    0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                                               |
 *  /                     QNAME                     /
 *  /                                               /
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QTYPE                     |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *  |                     QCLASS                    |
 *  +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * The query name is encoded as a series of labels, each represented
 * as a one-byte length (maximum 63) followed by the text of the
 * label.  The list is terminated by a label of length zero (which can
 * be thought of as the root domain).
 */

int ares_mkquery(const char *name, int dnsclass, int type, unsigned short id,
		 int rd, unsigned char **buf, int *buflen)
{
  size_t len;
  unsigned char *q;
  const char *p;

  /* Allocate a memory area for the maximum size this packet might need. +2
   * is for the length byte and zero termination if no dots or ecscaping is
   * used.
   */
  *buflen = strlen(name) + 2 + HFIXEDSZ + QFIXEDSZ;
  *buf = malloc(*buflen);
  if (!*buf)
      return ARES_ENOMEM;

  /* Set up the header. */
  q = *buf;
  memset(q, 0, HFIXEDSZ);
  DNS_HEADER_SET_QID(q, id);
  DNS_HEADER_SET_OPCODE(q, QUERY);
  DNS_HEADER_SET_RD(q, (rd) ? 1 : 0);
  DNS_HEADER_SET_QDCOUNT(q, 1);

  /* A name of "." is a screw case for the loop below, so adjust it. */
  if (strcmp(name, ".") == 0)
    name++;

  /* Start writing out the name after the header. */
  q += HFIXEDSZ;
  while (*name)
    {
      if (*name == '.')
	return ARES_EBADNAME;

      /* Count the number of bytes in this label. */
      len = 0;
      for (p = name; *p && *p != '.'; p++)
	{
	  if (*p == '\\' && *(p + 1) != 0)
	    p++;
	  len++;
	}
      if (len > MAXLABEL)
	return ARES_EBADNAME;

      /* Encode the length and copy the data. */
      *q++ = len;
      for (p = name; *p && *p != '.'; p++)
	{
	  if (*p == '\\' && *(p + 1) != 0)
	    p++;
	  *q++ = *p;
	}

      /* Go to the next label and repeat, unless we hit the end. */
      if (!*p)
	break;
      name = p + 1;
    }

  /* Add the zero-length label at the end. */
  *q++ = 0;

  /* Finish off the question with the type and class. */
  DNS_QUESTION_SET_TYPE(q, type);
  DNS_QUESTION_SET_CLASS(q, dnsclass);

  q += QFIXEDSZ;

  len = (q - *buf);

  /* Reject names that are longer than the maximum of 255 bytes that's
   * specified in RFC 1035 ("To simplify implementations, the total length of
   * a domain name (i.e., label octets and label length octets) is restricted
   * to 255 octets or less."). */
  if (len > (MAXCDNAME + HFIXEDSZ + QFIXEDSZ)) {
    return ARES_EBADNAME;
  }

  /* we know this fits in an int at this point */
  *buflen = (int)len;

  return ARES_SUCCESS;
}
