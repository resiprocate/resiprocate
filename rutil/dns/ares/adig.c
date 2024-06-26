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

#ifdef WIN32
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#else
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ares_nameser.h"
#include <unistd.h>
#include <netdb.h>
#endif

#include "rutil/ResipAssert.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include <errno.h>

#include "ares.h"
#include "ares_dns.h"
#include "ares_compat.h"

#if defined(WIN32) || defined(__QNX__)
#define strcasecmp(a,b) stricmp(a,b)
#define strncasecmp(a,b,c) strnicmp(a,b,c)
#endif


#ifndef INADDR_NONE
#define	INADDR_NONE 0xffffffff
#endif

extern int optind;
extern char *optarg;

struct nv {
  const char *name;
  int value;
};

static const struct nv flags[] = {
  { "usevc",		ARES_FLAG_USEVC },
  { "primary",		ARES_FLAG_PRIMARY },
  { "igntc",		ARES_FLAG_IGNTC },
  { "norecurse",	ARES_FLAG_NORECURSE },
  { "stayopen",		ARES_FLAG_STAYOPEN },
  { "noaliases",	ARES_FLAG_NOALIASES }
};
static const int nflags = sizeof(flags) / sizeof(flags[0]);

static const struct nv classes[] = {
  { "IN",	C_IN },
  { "CHAOS",	C_CHAOS },
  { "HS",	C_HS },
  { "ANY",	C_ANY }
};
static const int nclasses = sizeof(classes) / sizeof(classes[0]);

static const struct nv types[] = {
  { "A",	T_A },
  { "NS",	T_NS },
  { "MD",	T_MD },
  { "MF",	T_MF },
  { "CNAME",	T_CNAME },
  { "SOA",	T_SOA },
  { "MB",	T_MB },
  { "MG",	T_MG },
  { "MR",	T_MR },
  { "NULL",	T_NULL },
  { "WKS",	T_WKS },
  { "PTR",	T_PTR },
  { "HINFO",	T_HINFO },
  { "MINFO",	T_MINFO },
  { "MX",	T_MX },
  { "TXT",	T_TXT },
  { "RP",	T_RP },
  { "AFSDB",	T_AFSDB },
  { "X25",	T_X25 },
  { "ISDN",	T_ISDN },
  { "RT",	T_RT },
  { "NSAP",	T_NSAP },
  { "NSAP_PTR",	T_NSAP_PTR },
  { "SIG",	T_SIG },
  { "KEY",	T_KEY },
  { "PX",	T_PX },
  { "GPOS",	T_GPOS },
  { "AAAA",	T_AAAA },
  { "LOC",	T_LOC },
  { "SRV",	T_SRV },
  { "AXFR",	T_AXFR },
  { "MAILB",	T_MAILB },
  { "MAILA",	T_MAILA },
  { "NAPTR",	T_NAPTR },
  { "ANY",	T_ANY }
};
static const int ntypes = sizeof(types) / sizeof(types[0]);

static const char *opcodes[] = {
  "QUERY", "IQUERY", "STATUS", "(reserved)", "NOTIFY",
  "(unknown)", "(unknown)", "(unknown)", "(unknown)",
  "UPDATEA", "UPDATED", "UPDATEDA", "UPDATEM", "UPDATEMA",
  "ZONEINIT", "ZONEREF"
};

static const char *rcodes[] = {
  "NOERROR", "FORMERR", "SERVFAIL", "NXDOMAIN", "NOTIMP", "REFUSED",
  "(unknown)", "(unknown)", "(unknown)", "(unknown)", "(unknown)",
  "(unknown)", "(unknown)", "(unknown)", "(unknown)", "NOCHANGE"
};

static void callback(void *arg, int status, unsigned char *abuf, int alen);
static const unsigned char *display_question(const unsigned char *aptr,
					     const unsigned char *abuf,
					     int alen);
static const unsigned char *display_rr(const unsigned char *aptr,
				       const unsigned char *abuf, int alen);
static const char *type_name(int type);
static const char *class_name(int dnsclass);
static void usage(void);

#ifdef WIN32
struct option
{
      const char *name;
      int has_arg;
      int *flag;
      int val;
};

char *optarg = 0;
int optind = 0;

/**
 * This is a very hacked version for WIN32 that will support only what
 * we need.  This is NOT a generic getopt_long()
 */
int getopt(int argc,	
                      char *  argv[],
                       char *optstring )
{
   static int carg = 0;
   //static int nextchar = 0;
     char * p;

   carg++;
   if ( carg >= argc ) return -1;

 p = argv[carg];
   //register int al = strlen(argv[carg]);

   if (*p  == '-' && isalnum(*(p+1)))
   {
      char o = *(p+1);
	 int i,l;

       l =  (int)strlen(optstring);

      for( i = 0 ; i < l; i++)
      {

         if (optstring[i] == ':') continue;

         if ( optstring[i] == o )	// match option char
         {
	
            if ( optstring[i+1] == ':' ) // arg option
            {
               optind = ++carg;
               optarg = argv[optind];

            }
            else
            {
               optind = 0;
               optarg = 0;
            }
            return (int)o;
         }

      }

      return (int)'?';
   }
   return (int)'?';
}
#endif

int main(int argc, char **argv)
{
  ares_channel channel;
  int c, i, optmask = ARES_OPT_FLAGS, dnsclass = C_IN, type = T_A;
  int status, nfds, count;
  struct ares_options options;
  struct hostent *hostent;
  fd_set read_fds, write_fds;
  struct timeval *tvp, tv;
  char *errmem;

#ifdef WIN32 
   WORD wVersionRequested = MAKEWORD( 2, 2 );
   WSADATA wsaData;
   int err;

   err = WSAStartup( wVersionRequested, &wsaData );
   if ( err != 0 ) 
   {
      // could not find a usable WinSock DLL
      //cerr << "Could not load winsock" << endl;
      resip_assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
      exit(1);
   }
    
   /* Confirm that the WinSock DLL supports 2.2.*/
   /* Note that if the DLL supports versions greater    */
   /* than 2.2 in addition to 2.2, it will still return */
   /* 2.2 in wVersion since that is the version we      */
   /* requested.                                        */
    
   if ( LOBYTE( wsaData.wVersion ) != 2 ||
        HIBYTE( wsaData.wVersion ) != 2 ) 
   {
      /* Tell the user that we could not find a usable */
      /* WinSock DLL.                                  */
      WSACleanup( );
      //cerr << "Bad winsock verion" << endl;
      resip_assert(0); // is this is failing, try a different version that 2.2, 1.0 or later will likely work 
      exit(1);
   }  
#endif

    options.flags = ARES_FLAG_NOCHECKRESP;
  options.servers = NULL;
  options.nservers = 0;

  while ((c = getopt(argc, argv, "f:s:c:t:T:U:")) != -1)
    {
      switch (c)
	{
	case 'f':
	  /* Add a flag. */
	  for (i = 0; i < nflags; i++)
	    {
	      if (strcmp(flags[i].name, optarg) == 0)
		break;
	    }
	  if (i == nflags)
	    usage();
	  options.flags |= flags[i].value;
	  break;

	case 's':
	  /* Add a server, and specify servers in the option mask. */
	  hostent = gethostbyname(optarg);
	  if (!hostent || hostent->h_addrtype != AF_INET)
	    {
	      fprintf(stderr, "adig: server %s not found.\n", optarg);
	      return 1;
	    }
	  options.servers = realloc(options.servers, (options.nservers + 1)
				    * sizeof(struct in_addr));
	  if (!options.servers)
	    {
	      fprintf(stderr, "Out of memory!\n");
	      return 1;
	    }
	  memcpy(&options.servers[options.nservers], hostent->h_addr,
		 sizeof(struct in_addr));
	  options.nservers++;
	  optmask |= ARES_OPT_SERVERS;
	  break;

	case 'c':
	  /* Set the query class. */
	  for (i = 0; i < nclasses; i++)
	    {
	      if (strcasecmp(classes[i].name, optarg) == 0)
		break;
	    }
	  if (i == nclasses)
	    usage();
	  dnsclass = classes[i].value;
	  break;

	case 't':
	  /* Set the query type. */
	  for (i = 0; i < ntypes; i++)
	    {
	      if (strcasecmp(types[i].name, optarg) == 0)
		break;
	    }
	  if (i == ntypes)
	    usage();
	  type = types[i].value;
	  break;

	case 'T':
	  /* Set the TCP port number. */
	  if (!isdigit((unsigned char)*optarg))
	    usage();
	  options.tcp_port = (unsigned short)strtol(optarg, NULL, 0);
	  optmask |= ARES_OPT_TCP_PORT;
	  break;

	case 'U':
	  /* Set the UDP port number. */
	  if (!isdigit((unsigned char)*optarg))
	    usage();
	  options.udp_port = (unsigned short)strtol(optarg, NULL, 0);
	  optmask |= ARES_OPT_UDP_PORT;
	  break;
	}
    }
  argc -= optind;
  argv += optind;
  if (argc == 0)
    usage();

  status = ares_init_options(&channel, &options, optmask);
  if (status != ARES_SUCCESS)
    {
      fprintf(stderr, "ares_init_options: %s\n",
              ares_strerror(status)); //, &errmem));
      ares_free_errmem(errmem);
      return 1;
    }

  /* Initiate the queries, one per command-line argument.  If there is
   * only one query to do, supply NULL as the callback argument;
   * otherwise, supply the query name as an argument so we can
   * distinguish responses for the user when printing them out.
   */
  if (argc == 1)
    ares_query(channel, *argv, dnsclass, type, callback, (char *) NULL);
  else
    {
      for (; *argv; argv++)
	ares_query(channel, *argv, dnsclass, type, callback, *argv);
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
      count = select(nfds, &read_fds, &write_fds, NULL, tvp);
      if (count < 0 && errno != EINVAL)
	{
	  perror("select");
	  return 1;
	}
      ares_process(channel, &read_fds, &write_fds);
    }

  ares_destroy(channel);
  return 0;
}

static void callback(void *arg, int status, unsigned char *abuf, int alen)
{
  char *name = (char *) arg, *errmem;
  int id, qr, opcode, aa, tc, rd, ra, rcode, i;
  unsigned int qdcount, ancount, nscount, arcount;
  const unsigned char *aptr;

  /* Display the query name if given. */
  if (name)
    printf("Answer for query %s:\n", name);

  /* Display an error message if there was an error, but only stop if
   * we actually didn't get an answer buffer.
   */
  if (status != ARES_SUCCESS)
    {
       printf("%s\n", ares_strerror(status));//, &errmem));
      ares_free_errmem(errmem);
      if (!abuf)
	return;
    }

  /* Won't happen, but check anyway, for safety. */
  if (alen < HFIXEDSZ)
    return;

  /* Parse the answer header. */
  id = DNS_HEADER_QID(abuf);             /* query identification number */
  qr = DNS_HEADER_QR(abuf);              /* query response */
  opcode = DNS_HEADER_OPCODE(abuf);      /* opcode */
  aa = DNS_HEADER_AA(abuf);              /* authoritative answer */
  tc = DNS_HEADER_TC(abuf);              /* truncation */
  rd = DNS_HEADER_RD(abuf);              /* recursion desired */
  ra = DNS_HEADER_RA(abuf);              /* recursion available */
  rcode = DNS_HEADER_RCODE(abuf);        /* response code */
  qdcount = DNS_HEADER_QDCOUNT(abuf);    /* question count */
  ancount = DNS_HEADER_ANCOUNT(abuf);    /* answer record count */
  nscount = DNS_HEADER_NSCOUNT(abuf);    /* name server record count */
  arcount = DNS_HEADER_ARCOUNT(abuf);    /* additional record count */

  /* Display the answer header. */
  printf("id: %d\n", id);
  printf("flags: %s%s%s%s%s\n",
	 qr ? "qr " : "",
	 aa ? "aa " : "",
	 tc ? "tc " : "",
	 rd ? "rd " : "",
	 ra ? "ra " : "");
  printf("opcode: %s\n", opcodes[opcode]);
  printf("rcode: %s\n", rcodes[rcode]);

  /* Display the questions. */
  printf("Questions:\n");
  aptr = abuf + HFIXEDSZ;
  for (i = 0; i < (int)qdcount; i++)
    {
      aptr = display_question(aptr, abuf, alen);
      if (aptr == NULL)
	return;
    }

  /* Display the answers. */
  printf("Answers:\n");
  for (i = 0; i < (int)ancount; i++)
    {
      aptr = display_rr(aptr, abuf, alen);
      if (aptr == NULL)
	return;
    }

  /* Display the NS records. */
  printf("NS records:\n");
  for (i = 0; i < (int)nscount; i++)
    {
      aptr = display_rr(aptr, abuf, alen);
      if (aptr == NULL)
	return;
    }

  /* Display the additional records. */
  printf("Additional records:\n");
  for (i = 0; i < (int)arcount; i++)
    {
      aptr = display_rr(aptr, abuf, alen);
      if (aptr == NULL)
	return;
    }
}

static const unsigned char *display_question(const unsigned char *aptr,
					     const unsigned char *abuf,
					     int alen)
{
  char *name;
  int type, dnsclass, status, len;

  /* Parse the question name. */
  status = ares_expand_name(aptr, abuf, alen, &name, &len);
  if (status != ARES_SUCCESS)
    return NULL;
  aptr += len;

  /* Make sure there's enough data after the name for the fixed part
   * of the question.
   */
  if (aptr + QFIXEDSZ > abuf + alen)
    {
      free(name);
      return NULL;
    }

  /* Parse the question type and class. */
  type = DNS_QUESTION_TYPE(aptr);
  dnsclass = DNS_QUESTION_CLASS(aptr);
  aptr += QFIXEDSZ;

  /* Display the question, in a format sort of similar to how we will
   * display RRs.
   */
  printf("\t%-15s.\t", name);
  if (dnsclass != C_IN)
    printf("\t%s", class_name(dnsclass));
  printf("\t%s\n", type_name(type));
  free(name);
  return aptr;
}

static const unsigned char *display_rr(const unsigned char *aptr,
				       const unsigned char *abuf, int alen)
{
  const unsigned char *p;
  char *name;
  int type, dnsclass, ttl, dlen, status, len;
  struct in_addr addr;

  /* Parse the RR name. */
  status = ares_expand_name(aptr, abuf, alen, &name, &len);
  if (status != ARES_SUCCESS)
    return NULL;
  aptr += len;

  /* Make sure there is enough data after the RR name for the fixed
   * part of the RR.
   */
  if (aptr + RRFIXEDSZ > abuf + alen)
    {
      free(name);
      return NULL;
    }

  /* Parse the fixed part of the RR, and advance to the RR data
   * field. */
  type = DNS_RR_TYPE(aptr);
  dnsclass = DNS_RR_CLASS(aptr);
  ttl = DNS_RR_TTL(aptr);
  dlen = DNS_RR_LEN(aptr);
  aptr += RRFIXEDSZ;
  if (aptr + dlen > abuf + alen)
    {
      free(name);
      return NULL;
    }

  /* Display the RR name, class, and type. */
  printf("\t%-15s.\t%d", name, ttl);
  if (dnsclass != C_IN)
    printf("\t%s", class_name(dnsclass));
  printf("\t%s", type_name(type));
  free(name);

  /* Display the RR data.  Don't touch aptr. */
  switch (type)
    {
    case T_CNAME:
    case T_MB:
    case T_MD:
    case T_MF:
    case T_MG:
    case T_MR:
    case T_NS:
    case T_PTR:
      /* For these types, the RR data is just a domain name. */
      status = ares_expand_name(aptr, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
	return NULL;
      printf("\t%s.", name);
      free(name);
      break;

    case T_HINFO:
      /* The RR data is two length-counted character strings. */
      p = aptr;
      len = *p;
      if (p + len + 1 > aptr + dlen)
	return NULL;
      printf("\t%.*s", len, p + 1);
      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
	return NULL;
      printf("\t%.*s", len, p + 1);
      break;

    case T_MINFO:
      /* The RR data is two domain names. */
      p = aptr;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
	return NULL;
      printf("\t%s.", name);
      free(name);
      p += len;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
	return NULL;
      printf("\t%s.", name);
      free(name);
      break;

    case T_MX:
      /* The RR data is two bytes giving a preference ordering, and
       * then a domain name.
       */
      if (dlen < 2)
	return NULL;
      printf("\t%d", (aptr[0] << 8) | aptr[1]);
      status = ares_expand_name(aptr + 2, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
	return NULL;
      printf("\t%s.", name);
      free(name);
      break;

    case T_SOA:
      /* The RR data is two domain names and then five four-byte
       * numbers giving the serial number and some timeouts.
       */
      p = aptr;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
	return NULL;
      printf("\t%s.\n", name);
      free(name);
      p += len;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
	return NULL;
      printf("\t\t\t\t\t\t%s.\n", name);
      free(name);
      p += len;
      if (p + 20 > aptr + dlen)
	return NULL;
      printf("\t\t\t\t\t\t( %d %d %d %d %d )",
	     (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3],
	     (p[4] << 24) | (p[5] << 16) | (p[6] << 8) | p[7],
	     (p[8] << 24) | (p[9] << 16) | (p[10] << 8) | p[11],
	     (p[12] << 24) | (p[13] << 16) | (p[14] << 8) | p[15],
	     (p[16] << 24) | (p[17] << 16) | (p[18] << 8) | p[19]);
      break;

    case T_TXT:
      /* The RR data is one or more length-counted character
       * strings. */
      p = aptr;
      while (p < aptr + dlen)
	{
	  len = *p;
	  if (p + len + 1 > aptr + dlen)
	    return NULL;
	  printf("\t%.*s", len, p + 1);
	  p += len + 1;
	}
      break;

    case T_A:
      /* The RR data is a four-byte Internet address. */
      if (dlen != 4)
	return NULL;
      memcpy(&addr, aptr, sizeof(struct in_addr));
      printf("\t%s", inet_ntoa(addr));
      break;

    case T_WKS:
      /* Not implemented yet */
      break;

    case T_SRV:
      /* The RR data is three two-byte numbers representing the
       * priority, weight, and port, followed by a domain name.
       */
      
      printf("\t%d", DNS__16BIT(aptr));
      printf(" %d", DNS__16BIT(aptr + 2));
      printf(" %d", DNS__16BIT(aptr + 4));
      
      status = ares_expand_name(aptr + 6, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
        return NULL;
      printf("\t%s.", name);
      free(name);
      break;
      
    case T_NAPTR:
      /* The RR data is two two-byte numbers representing the
       * order and preference, followed by three character strings
       * representing flags, services, a regex, and a domain name.
       */

      printf("\t%d", DNS__16BIT(aptr));
      printf(" %d", DNS__16BIT(aptr + 2));

      p = aptr + 4;
      len = *p;
      if (p + len + 1 > aptr + dlen)
        return NULL;
      printf(" %.*s", len, p + 1);
      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
        return NULL;
      printf(" %.*s", len, p + 1);
      p += len + 1;
      len = *p;
      if (p + len + 1 > aptr + dlen)
        return NULL;
      printf(" %.*s", len, p + 1);
      p += len + 1;
      status = ares_expand_name(p, abuf, alen, &name, &len);
      if (status != ARES_SUCCESS)
        return NULL;
      printf("\t%s.", name);
      free(name);
      break;

    default:
      printf("\t[Unknown RR; cannot parse]");
    }
  printf("\n");

  return aptr + dlen;
}

static const char *type_name(int type)
{
  int i;

  for (i = 0; i < ntypes; i++)
    {
      if (types[i].value == type)
	return types[i].name;
    }
  return "(unknown)";
}

static const char *class_name(int dnsclass)
{
  int i;

  for (i = 0; i < nclasses; i++)
    {
      if (classes[i].value == dnsclass)
	return classes[i].name;
    }
  return "(unknown)";
}

static void usage(void)
{
  fprintf(stderr, "usage: adig [-f flag] [-s server] [-c class] "
	  "[-t type] [-p port] name ...\n");
  exit(1);
}
