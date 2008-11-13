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

#ifndef RARES__DNS_H
#define RARES__DNS_H

#define RARES_DNS__16BIT(p)			(((p)[0] << 8) | (p)[1])
#define RARES_DNS__32BIT(p)			(((p)[0] << 24) | ((p)[1] << 16) | \
					 ((p)[2] << 8) | (p)[3])
#define RARES_DNS__SET16BIT(p, v)		(((p)[0] = ((v) >> 8) & 0xff), \
					 ((p)[1] = (v) & 0xff))
#define RARES_DNS__SET32BIT(p, v)		(((p)[0] = ((v) >> 24) & 0xff), \
					 ((p)[1] = ((v) >> 16) & 0xff), \
					 ((p)[2] = ((v) >> 8) & 0xff), \
					 ((p)[3] = (v) & 0xff))

/* Macros for parsing a DNS header */
#define RARES_DNS_HEADER_QID(h)		RARES_DNS__16BIT(h)
#define RARES_DNS_HEADER_QR(h)		(((h)[2] >> 7) & 0x1)
#define RARES_DNS_HEADER_OPCODE(h)		(((h)[2] >> 3) & 0xf)
#define RARES_DNS_HEADER_AA(h)		(((h)[2] >> 2) & 0x1)
#define RARES_DNS_HEADER_TC(h)		(((h)[2] >> 1) & 0x1)
#define RARES_DNS_HEADER_RD(h)		((h)[2] & 0x1)
#define RARES_DNS_HEADER_RA(h)		(((h)[3] >> 7) & 0x1)
#define RARES_DNS_HEADER_Z(h)			(((h)[3] >> 4) & 0x7)
#define RARES_DNS_HEADER_RCODE(h)		((h)[3] & 0xf)
#define RARES_DNS_HEADER_QDCOUNT(h)		RARES_DNS__16BIT((h) + 4)
#define RARES_DNS_HEADER_ANCOUNT(h)		RARES_DNS__16BIT((h) + 6)
#define RARES_DNS_HEADER_NSCOUNT(h)		RARES_DNS__16BIT((h) + 8)
#define RARES_DNS_HEADER_ARCOUNT(h)		RARES_DNS__16BIT((h) + 10)

/* Macros for constructing a DNS header */
#define RARES_DNS_HEADER_SET_QID(h, v)	RARES_DNS__SET16BIT(h, v)
#define RARES_DNS_HEADER_SET_QR(h, v)		((h)[2] |= (((v) & 0x1) << 7))
#define RARES_DNS_HEADER_SET_OPCODE(h, v)	((h)[2] |= (((v) & 0xf) << 3))
#define RARES_DNS_HEADER_SET_AA(h, v)		((h)[2] |= (((v) & 0x1) << 2))
#define RARES_DNS_HEADER_SET_TC(h, v)		((h)[2] |= (((v) & 0x1) << 1))
#define RARES_DNS_HEADER_SET_RD(h, v)		((h)[2] |= (((v) & 0x1)))
#define RARES_DNS_HEADER_SET_RA(h, v)		((h)[3] |= (((v) & 0x1) << 7))
#define RARES_DNS_HEADER_SET_Z(h, v)		((h)[3] |= (((v) & 0x7) << 4))
#define RARES_DNS_HEADER_SET_RCODE(h, v)	((h)[3] |= (((v) & 0xf)))
#define RARES_DNS_HEADER_SET_QDCOUNT(h, v)	RARES_DNS__SET16BIT((h) + 4, v)
#define RARES_DNS_HEADER_SET_ANCOUNT(h, v)	RARES_DNS__SET16BIT((h) + 6, v)
#define RARES_DNS_HEADER_SET_NSCOUNT(h, v)	RARES_DNS__SET16BIT((h) + 8, v)
#define RARES_DNS_HEADER_SET_ARCOUNT(h, v)	RARES_DNS__SET16BIT((h) + 10, v)

/* Macros for parsing the fixed part of a DNS question */
#define RARES_DNS_QUESTION_TYPE(q)		RARES_DNS__16BIT(q)
#define RARES_DNS_QUESTION_CLASS(q)		RARES_DNS__16BIT((q) + 2)

/* Macros for constructing the fixed part of a DNS question */
#define RARES_DNS_QUESTION_SET_TYPE(q, v)	RARES_DNS__SET16BIT(q, v)
#define RARES_DNS_QUESTION_SET_CLASS(q, v)	RARES_DNS__SET16BIT((q) + 2, v)

/* Macros for parsing the fixed part of a DNS resource record */
#define RARES_DNS_RR_TYPE(r)			RARES_DNS__16BIT(r)
#define RARES_DNS_RR_CLASS(r)			RARES_DNS__16BIT((r) + 2)
#define RARES_DNS_RR_TTL(r)			RARES_DNS__32BIT((r) + 4)
#define RARES_DNS_RR_LEN(r)			RARES_DNS__16BIT((r) + 8)

/* Macros for constructing the fixed part of a DNS resource record */
#define RARES_DNS_RR_SET_TYPE(r,v)		RARES_DNS__SET16BIT(r, v)
#define RARES_DNS_RR_SET_CLASS(r,v)		RARES_DNS__SET16BIT((r) + 2, v)
#define RARES_DNS_RR_SET_TTL(r,v)		RARES_DNS__SET32BIT((r) + 4, v)
#define RARES_DNS_RR_SET_LEN(r,v)		RARES_DNS__SET16BIT((r) + 8, v)

#endif /* RARES__DNS_H */
