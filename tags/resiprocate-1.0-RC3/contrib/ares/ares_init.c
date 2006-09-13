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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <assert.h>

#ifndef WIN32
#include <sys/time.h>
#include <sys/param.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#ifndef __CYGWIN__
#  include <arpa/nameser.h>
#endif
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#else
#include <Winsock2.h>
#include <iphlpapi.h>
#include <io.h>
#include <Windns.h>
#endif

#include "ares.h"
#include "ares_private.h"

static int init_by_options(ares_channel channel, struct ares_options *options,
			   int optmask);
static int init_by_environment(ares_channel channel);
static int init_by_resolv_conf(ares_channel channel);
static int init_by_defaults(ares_channel channel);
static int config_domain(ares_channel channel, char *str);
static int config_lookup(ares_channel channel, const char *str);
static int config_nameserver(struct server_state **servers, int *nservers,
			     const char *str);
static int config_sortlist(struct apattern **sortlist, int *nsort,
			   const char *str);
static int set_search(ares_channel channel, const char *str);
static int set_options(ares_channel channel, const char *str);
static char *try_config(char *s, char *opt);
static const char *try_option(const char *p, const char *q, const char *opt);
static int ip_addr(const char *s, int len, struct in_addr *addr);
static void natural_mask(struct apattern *pat);

static int	inet_pton4(const char *src, u_char *dst);
#ifdef USE_IPV6
static int	inet_pton6(const char *src, u_char *dst);
#endif 

#ifdef WIN32
char w32hostspath[256];
#endif


int ares_capabilities(int capmask)
{
#ifdef USE_IPV6
   static int ares_caps = ARES_CAP_IPV6;
#else
   static int ares_caps = 0;
#endif
   return (capmask & ares_caps);
}

int ares_init(ares_channel *channelptr)
{
   return ares_init_options_with_socket_function(channelptr, NULL, 0, NULL);
}

int ares_init_with_socket_function(ares_channel *channelptr, socket_function_ptr socketFunc)
{
   return ares_init_options_with_socket_function(channelptr, NULL, 0, socketFunc);
}

int ares_init_options(ares_channel *channelptr, struct ares_options *options,
                      int optmask)
{
   return ares_init_options_with_socket_function(channelptr, options, optmask, NULL);
}

int ares_init_options_with_socket_function(ares_channel *channelptr, struct ares_options *options,
                      int optmask, socket_function_ptr socketFunc)
{
  ares_channel channel;
  int i, status;
  struct server_state *server;
#ifdef WIN32
	{
		HKEY hKey;  
		char hostpath[256];
  if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SYSTEM\\CurrentControlSet\\Services\\Tcpip\\Parameters"), 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS)
		{
			DWORD dwSize = sizeof(hostpath);
      if(RegQueryValueEx(hKey, TEXT("DatabasePath"), 0, 0, (LPBYTE)&hostpath, &dwSize) == ERROR_SUCCESS)
			{
				hostpath[dwSize] = '\0';
#if defined(UNDER_CE)
			ZeroMemory(hostpath,strlen(hostpath)*sizeof(TCHAR));
#else
				ExpandEnvironmentStrings(hostpath, w32hostspath, sizeof(w32hostspath));
#endif
				if(strlen(w32hostspath) < sizeof(w32hostspath) - 6) 
				{
					strcat(w32hostspath, "\\hosts");
				}
			}
         RegCloseKey(hKey);
		}
	}
#endif
 //  struct timeval tv;

  channel = malloc(sizeof(struct ares_channeldata));
  if (!channel)
    return ARES_ENOMEM;

  /* Set everything to distinguished values so we know they haven't
   * been set yet.
   */
  channel->socket_function = socketFunc;  
  channel->flags = -1;
  channel->timeout = -1;
  channel->tries = -1;
  channel->ndots = -1;
  channel->udp_port = -1;
  channel->tcp_port = -1;
  channel->nservers = -1;
  channel->ndomains = -1;
  channel->nsort = -1;
  channel->lookups = NULL;

  /* Initialize configuration by each of the four sources, from highest
   * precedence to lowest.
   */
  status = init_by_options(channel, options, optmask);
  if (status == ARES_SUCCESS)
    status = init_by_environment(channel);
  if (status == ARES_SUCCESS)
    status = init_by_resolv_conf(channel);
  if (status == ARES_SUCCESS)
    status = init_by_defaults(channel);
  if (status != ARES_SUCCESS)
    {
      /* Something failed; clean up memory we may have allocated. */
      if (channel->nservers != -1)
	free(channel->servers);
      if (channel->ndomains != -1)
	{
	  for (i = 0; i < channel->ndomains; i++)
	    free(channel->domains[i]);
	  free(channel->domains);
	}
      if (channel->nsort != -1)
	free(channel->sortlist);
      free(channel->lookups);
      free(channel);
      return status;
    }

  /* Trim to one server if ARES_FLAG_PRIMARY is set. */
  if ((channel->flags & ARES_FLAG_PRIMARY) && channel->nservers > 1)
    channel->nservers = 1;

  /* Initialize server states. */
  for (i = 0; i < channel->nservers; i++)
    {
      server = &channel->servers[i];
      server->udp_socket = -1;
      server->tcp_socket = -1;
      server->tcp_lenbuf_pos = 0;
      server->tcp_buffer = NULL;
      server->qhead = NULL;
      server->qtail = NULL;
    }

  /* Choose a somewhat random query ID.  The main point is to avoid
   * collisions with stale queries.  An attacker trying to spoof a DNS
   * answer also has to guess the query ID, but it's only a 16-bit
   * field, so there's not much to be done about that.
   */
//  gettimeofday(&tv, NULL);
//  channel->next_id = (tv.tv_sec ^ tv.tv_usec ^ getpid()) & 0xffff;
  {
	static int cjNextID=1;
	  channel->next_id = cjNextID++;
  }

  channel->queries = NULL;

  *channelptr = channel;
  return ARES_SUCCESS;
}

static int init_by_options(ares_channel channel, struct ares_options *options,
			   int optmask)
{
  int i;

  /* Easy stuff. */
  if ((optmask & ARES_OPT_FLAGS) && channel->flags == -1)
    channel->flags = options->flags;
  if ((optmask & ARES_OPT_TIMEOUT) && channel->timeout == -1)
    channel->timeout = options->timeout;
  if ((optmask & ARES_OPT_TRIES) && channel->tries == -1)
    channel->tries = options->tries;
  if ((optmask & ARES_OPT_NDOTS) && channel->ndots == -1)
    channel->ndots = options->ndots;
  if ((optmask & ARES_OPT_UDP_PORT) && channel->udp_port == -1)
     channel->udp_port = options->udp_port;
  if ((optmask & ARES_OPT_TCP_PORT) && channel->tcp_port == -1)
     channel->tcp_port = options->tcp_port;

  /* Copy the servers, if given. */
  if ((optmask & ARES_OPT_SERVERS) && channel->nservers == -1)
  {
     channel->servers =
        malloc(options->nservers * sizeof(struct server_state));
     if (!channel->servers && options->nservers != 0)
        return ARES_ENOMEM;
     for (i = 0; i < options->nservers; i++)
     {
#ifdef USE_IPV6
		channel->servers[i].family = options->servers[i].family;
		if (options->servers[i].family == AF_INET6)
		{
		  channel->servers[i].addr6 = options->servers[i].addr6;
		}
		else
		{
		  channel->servers[i].addr = options->servers[i].addr;
		}
#else	  
		channel->servers[i].addr = options->servers[i];
#endif
        channel->nservers = options->nservers;
	  }
    }

  /* Copy the domains, if given.  Keep channel->ndomains consistent so
   * we can clean up in case of error.
   */
  if ((optmask & ARES_OPT_DOMAINS) && channel->ndomains == -1)
    {
      channel->domains = malloc(options->ndomains * sizeof(char *));
      if (!channel->domains && options->ndomains != 0)
	return ARES_ENOMEM;
      for (i = 0; i < options->ndomains; i++)
	{
	  channel->ndomains = i;
	  channel->domains[i] = strdup(options->domains[i]);
	  if (!channel->domains[i])
	    return ARES_ENOMEM;
	}
      channel->ndomains = options->ndomains;
    }

  /* Set lookups, if given. */
  if ((optmask & ARES_OPT_LOOKUPS) && !channel->lookups)
    {
      channel->lookups = strdup(options->lookups);
      if (!channel->lookups)
	return ARES_ENOMEM;
    }

  return ARES_SUCCESS;
}

static int init_by_environment(ares_channel channel)
{
  const char *localdomain, *res_options;
  int status;

#if defined(UNDER_CE)
  localdomain = NULL;
#else
  localdomain = getenv("LOCALDOMAIN");
#endif
  if (localdomain && channel->ndomains == -1)
    {
      status = set_search(channel, localdomain);
      if (status != ARES_SUCCESS)
	return status;
    }

#if defined(UNDER_CE)
  res_options = NULL;
#else
  res_options = getenv("RES_OPTIONS");
#endif
  if (res_options)
    {
      status = set_options(channel, res_options);
      if (status != ARES_SUCCESS)
	return status;
    }

  return ARES_SUCCESS;
}

static int init_by_resolv_conf(ares_channel channel)
{
  FILE *fp;
  char *line = NULL, *p;
  int linesize, status, nservers = 0, nsort = 0;
  struct server_state *servers = NULL;
  struct apattern *sortlist = NULL;

  fp = fopen(PATH_RESOLV_CONF, "r");
#if defined(UNDER_CE)
  errno = ENOENT;
#endif
  if (!fp)
    return (errno == ENOENT) ? ARES_SUCCESS : ARES_EFILE;
  while ((status = ares__read_line(fp, &line, &linesize)) == ARES_SUCCESS)
    {
      if ((p = try_config(line, "domain")) && channel->ndomains == -1)
	status = config_domain(channel, p);
      else if ((p = try_config(line, "lookup")) && !channel->lookups)
	status = config_lookup(channel, p);
      else if ((p = try_config(line, "search")) && channel->ndomains == -1)
	status = set_search(channel, p);
      else if ((p = try_config(line, "nameserver")) && channel->nservers == -1)
	status = config_nameserver(&servers, &nservers, p);
      else if ((p = try_config(line, "sortlist")) && channel->nsort == -1)
	status = config_sortlist(&sortlist, &nsort, p);
      else if ((p = try_config(line, "options")))
	status = set_options(channel, p);
      else
	status = ARES_SUCCESS;
      if (status != ARES_SUCCESS)
	break;
    }
  free(line);
  fclose(fp);

  /* Handle errors. */
  if (status != ARES_EOF)
    {
      free(servers);
      free(sortlist);
      return status;
    }

  /* If we got any name server entries, fill them in. */
  if (servers)
    {
      channel->servers = servers;
      channel->nservers = nservers;
    }

  /* If we got any sortlist entries, fill them in. */
  if (sortlist)
    {
      channel->sortlist = sortlist;
      channel->nsort = nsort;
    }

  return ARES_SUCCESS;
}

static int init_by_defaults(ares_channel channel)
{
  char hostname[MAXHOSTNAMELEN + 1];

  if (channel->flags == -1)
    channel->flags = 0;
  if (channel->timeout == -1)
    channel->timeout = DEFAULT_TIMEOUT;
  if (channel->tries == -1)
    channel->tries = DEFAULT_TRIES;
  if (channel->ndots == -1)
    channel->ndots = 1;
  if (channel->udp_port == -1)
    channel->udp_port = htons(NAMESERVER_PORT);
  if (channel->tcp_port == -1)
    channel->tcp_port = htons(NAMESERVER_PORT);

  if (channel->nservers == -1)
    {
#ifdef WIN32
     /*
      * Way of getting nameservers that should work on all Windows from 98 on.
      */
      FIXED_INFO *     FixedInfo;
      ULONG            ulOutBufLen;
      DWORD            dwRetVal;
      IP_ADDR_STRING * pIPAddr;
	  HANDLE           hLib;
	  int num;
	  DWORD (WINAPI *GetNetworkParams)(FIXED_INFO*, DWORD*); 

	  hLib = LoadLibrary(TEXT("iphlpapi.dll"));
	  if(!hLib)
	  {
		  return ARES_ENOTIMP;
	  }

	  (void*)GetNetworkParams = GetProcAddress(hLib, TEXT("GetNetworkParams"));
	  if(!GetNetworkParams)
	  {
		  FreeLibrary(hLib);
		  return ARES_ENOTIMP;
	  }
      //printf("ARES: figuring out DNS servers\n");
      FixedInfo = (FIXED_INFO *) GlobalAlloc( GPTR, sizeof( FIXED_INFO ) );
      ulOutBufLen = sizeof( FIXED_INFO );

      if( ERROR_BUFFER_OVERFLOW == (*GetNetworkParams)( FixedInfo, &ulOutBufLen ) ) 
	  {
        GlobalFree( FixedInfo );
        FixedInfo = (FIXED_INFO *)GlobalAlloc( GPTR, ulOutBufLen );
      }

      if ( dwRetVal = (*GetNetworkParams)( FixedInfo, &ulOutBufLen ) )
      {
        //printf("ARES: couldn't get network params\n");
        GlobalFree( FixedInfo );
  	    FreeLibrary(hLib);
        return ARES_ENODATA;
      }
      else
      {
       /**
        printf( "Host Name: %s\n", FixedInfo -> HostName );
        printf( "Domain Name: %s\n", FixedInfo -> DomainName );
        printf( "DNS Servers:\n" );
        printf( "\t%s\n", FixedInfo -> DnsServerList.IpAddress.String );
        **/

        // Count how many nameserver entries we have and allocate memory for them.
        num = 0;
        pIPAddr = &FixedInfo->DnsServerList;     
        while ( pIPAddr && strlen(pIPAddr->IpAddress.String) > 0)
        {
          num++;
          pIPAddr = pIPAddr ->Next;
        }
        if(num>0)
        {
           channel->servers = malloc( (num) * sizeof(struct server_state));
		   if (!channel->servers)
		   {
	           GlobalFree( FixedInfo );
 		       FreeLibrary(hLib);
			   return ARES_ENOMEM;
		   }

		   channel->nservers = 0;
           pIPAddr = &FixedInfo->DnsServerList;   
           while ( pIPAddr && strlen(pIPAddr->IpAddress.String) > 0)
		   {
             // printf( "ARES: %s\n", pIPAddr ->IpAddress.String );
#ifdef USE_IPV6			 
			 channel->servers[ channel->nservers ].family = AF_INET;
#endif
	         channel->servers[ channel->nservers ].addr.s_addr = inet_addr(pIPAddr ->IpAddress.String);
			 channel->nservers++;

             pIPAddr = pIPAddr ->Next;
           }
           //printf("ARES: got all %d nameservers\n",num);
        }
        else
        {
  		   /* If no specified servers, try a local named. */
		   channel->servers = malloc(sizeof(struct server_state));
		   if (!channel->servers)
              return ARES_ENOMEM;

#ifdef USE_IPV6			 
           channel->servers[0].family = AF_INET;
#endif

		   channel->servers[0].addr.s_addr = htonl(INADDR_LOOPBACK);
		   channel->nservers = 1;
        }

        GlobalFree( FixedInfo );
	    FreeLibrary(hLib);
	  }
#else
		/* If nobody specified servers, try a local named. */
		channel->servers = malloc(sizeof(struct server_state));
		if (!channel->servers)
			return ARES_ENOMEM;
			
		// need a way to test here if v4 or v6 is running
		// if v4 is running...
		channel->servers[0].addr.s_addr = htonl(INADDR_LOOPBACK);
		// if v6 is running...
        //	channel->servers[0].addr6.s_addr = htonl6(IN6ADDR_LOOPBACK_INIT);
		// hard to decide if there is one server or two here
		channel->nservers = 1;
#endif
    
    }

  if (channel->ndomains == -1)
    {
      /* Derive a default domain search list from the kernel hostname,
       * or set it to empty if the hostname isn't helpful.
       */
      if (gethostname(hostname, sizeof(hostname)) == -1
	  || !strchr(hostname, '.'))
	{
	  channel->domains = 0; // malloc(0);
	  channel->ndomains = 0;
	}
      else
	{
	  channel->domains = malloc(sizeof(char *));
	  if (!channel->domains)
	    return ARES_ENOMEM;
	  channel->ndomains = 0;
	  channel->domains[0] = strdup(strchr(hostname, '.') + 1);
	  if (!channel->domains[0])
	    return ARES_ENOMEM;
	  channel->ndomains = 1;
	}
    }

  if (channel->nsort == -1)
    {
      channel->sortlist = NULL;
      channel->nsort = 0;
    }

  if (!channel->lookups)
    {
      channel->lookups = strdup("bf");
      if (!channel->lookups)
	return ARES_ENOMEM;
    }

  return ARES_SUCCESS;
}

static int config_domain(ares_channel channel, char *str)
{
  char *q;

  /* Set a single search domain. */
  q = str;
  while (*q && !isspace((unsigned char)*q))
    q++;
  *q = 0;
  return set_search(channel, str);
}

static int config_lookup(ares_channel channel, const char *str)
{
  char lookups[3], *l;
  const char *p;

  /* Set the lookup order.  Only the first letter of each work
   * is relevant, and it has to be "b" for DNS or "f" for the
   * host file.  Ignore everything else.
   */
  l = lookups;
  p = str;
  while (*p)
    {
      if ((*p == 'b' || *p == 'f') && l < lookups + 2)
	*l++ = *p;
      while (*p && !isspace((unsigned char)*p))
	p++;
      while (isspace((unsigned char)*p))
	p++;
    }
  *l = 0;
  channel->lookups = strdup(lookups);
  return (channel->lookups) ? ARES_SUCCESS : ARES_ENOMEM;
}

static int config_nameserver(struct server_state **servers, int *nservers,
			     const char *str)
{
  struct in_addr addr;
  struct server_state *newserv;
#ifdef USE_IPV6
  u_int8_t family;
  struct in6_addr addr6;

  /* Add a nameserver entry, if this is a valid address. */

  if (inet_pton4(str, (u_char *) & addr))   /* is it an IPv4 address? */
	family = AF_INET;
  else
  { 
	if (inet_pton6(str, (u_char *) & addr6))  /* how about an IPv6 address? */
	  family = AF_INET6;
	else	
	  return ARES_SUCCESS;	/* nope, it was garbage, return early */
  }
#else
  /* Add a nameserver entry, if this is a valid address. */

  if (!inet_pton4(str, (u_char *) & addr))   /* is it an IPv4 address? */
	  return ARES_SUCCESS;	/* nope, it was garbage, return early */
#endif

  newserv = realloc(*servers, (*nservers + 1) * sizeof(struct server_state));
  if (!newserv)
    return ARES_ENOMEM;

#ifdef USE_IPV6
  newserv[*nservers].family = family;
  if (family == AF_INET6)
    newserv[*nservers].addr6 = addr6;  
  else  
#endif
	newserv[*nservers].addr = addr;

  *servers = newserv;
  (*nservers)++;
  return ARES_SUCCESS;
}

static int config_sortlist(struct apattern **sortlist, int *nsort,
			   const char *str)
{
  struct apattern pat, *newsort;
  const char *q;

  /* Add sortlist entries. */
  while (*str && *str != ';')
    {
      q = str;
      while (*q && *q != '/' && *q != ';' && !isspace((unsigned char)*q))
	q++;
      if (ip_addr(str, q - str, &pat.addr) == 0)
	{
	  /* We have a pattern address; now determine the mask. */
	  if (*q == '/')
	    {
	      str = q + 1;
	      while (*q && *q != ';' && !isspace((unsigned char)*q))
		q++;
	      if (ip_addr(str, q - str, &pat.mask) != 0)
		natural_mask(&pat);
	    }
	  else
	    natural_mask(&pat);

	  /* Add this pattern to our list. */
	  newsort = realloc(*sortlist, (*nsort + 1) * sizeof(struct apattern));
	  if (!newsort)
	    return ARES_ENOMEM;
	  newsort[*nsort] = pat;
	  *sortlist = newsort;
	  (*nsort)++;
	}
      else
	{
	  while (*q && *q != ';' && !isspace((unsigned char)*q))
	    q++;
	}
      str = q;
      while (isspace((unsigned char)*str))
	str++;
    }

  return ARES_SUCCESS;
}

static int set_search(ares_channel channel, const char *str)
{
  int n;
  const char *p, *q;

  /* Count the domains given. */
  n = 0;
  p = str;
  while (*p)
    {
      while (*p && !isspace((unsigned char)*p))
	p++;
      while (isspace((unsigned char)*p))
	p++;
      n++;
    }

  channel->domains = malloc(n * sizeof(char *));
  if (!channel->domains && n)
    return ARES_ENOMEM;

  /* Now copy the domains. */
  n = 0;
  p = str;
  while (*p)
    {
      channel->ndomains = n;
      q = p;
      while (*q && !isspace((unsigned char)*q))
	q++;
      channel->domains[n] = malloc(q - p + 1);
      if (!channel->domains[n])
	return ARES_ENOMEM;
      memcpy(channel->domains[n], p, q - p);
      channel->domains[n][q - p] = 0;
      p = q;
      while (isspace((unsigned char)*p))
	p++;
      n++;
    }
  channel->ndomains = n;

  return ARES_SUCCESS;
}

static int set_options(ares_channel channel, const char *str)
{
  const char *p, *q, *val;

  p = str;
  while (*p)
    {
      q = p;
      while (*q && !isspace((unsigned char)*q))
	q++;
      val = try_option(p, q, "ndots:");
      if (val && channel->ndots == -1)
	channel->ndots = atoi(val);
      val = try_option(p, q, "retrans:");
      if (val && channel->timeout == -1)
	channel->timeout = atoi(val);
      val = try_option(p, q, "retry:");
      if (val && channel->tries == -1)
	channel->tries = atoi(val);
      p = q;
      while (isspace((unsigned char)*p))
	p++;
    }

  return ARES_SUCCESS;
}

static char *try_config(char *s, char *opt)
{
  int len;

  len = strlen(opt);
  if (strncmp(s, opt, len) != 0 || !isspace((unsigned char)s[len]))
    return NULL;
  s += len;
  while (isspace((unsigned char)*s))
    s++;
  return s;
}

static const char *try_option(const char *p, const char *q, const char *opt)
{
  int len;

  len = strlen(opt);
  return (q - p > len && strncmp(p, opt, len) == 0) ? p + len : NULL;
}

static int ip_addr(const char *s, int len, struct in_addr *addr)
{
  char ipbuf[16];

  /* Four octets and three periods yields at most 15 characters. */
  if (len > 15)
    return -1;
  memcpy(ipbuf, s, len);
  ipbuf[len] = 0;

  addr->s_addr = inet_addr(ipbuf);
  if (addr->s_addr == INADDR_NONE && strcmp(ipbuf, "255.255.255.255") != 0)
    return -1;
  return 0;
}

static void natural_mask(struct apattern *pat)
{
  struct in_addr addr;

  /* Store a host-byte-order copy of pat in a struct in_addr.  Icky,
   * but portable.
   */
  addr.s_addr = ntohl(pat->addr.s_addr);

  /* This is out of date in the CIDR world, but some people might
   * still rely on it.
   */
  if (IN_CLASSA(addr.s_addr))
    pat->mask.s_addr = htonl(IN_CLASSA_NET);
  else if (IN_CLASSB(addr.s_addr))
    pat->mask.s_addr = htonl(IN_CLASSB_NET);
  else
    pat->mask.s_addr = htonl(IN_CLASSC_NET);
}

#define  NS_INT16SZ   2
#define  NS_INADDRSZ  4
#define  NS_IN6ADDRSZ 16

#ifdef USE_IPV6
/* int
 * inet_pton6(src, dst)
 *	convert presentation level address to network order binary form.
 * return:
 *	1 if `src' is a valid [RFC1884 2.2] address, else 0.
 * notice:
 *	(1) does not touch `dst' unless it's returning 1.
 *	(2) :: in a full address is silently ignored.
 * credit:
 *	inspired by Mark Andrews.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton6(const char *src, u_char *dst)
{
   static const char xdigits_l[] = "0123456789abcdef",
      xdigits_u[] = "0123456789ABCDEF";
   u_char tmp[NS_IN6ADDRSZ], *tp, *endp, *colonp;
   const char *xdigits, *curtok;
   int ch, saw_xdigit;
   u_int val;

   memset((tp = tmp), '\0', NS_IN6ADDRSZ);
   endp = tp + NS_IN6ADDRSZ;
   colonp = NULL;
   /* Leading :: requires some special handling. */
   if (*src == ':')
      if (*++src != ':')
         return (0);
   curtok = src;
   saw_xdigit = 0;
   val = 0;
   while ((ch = *src++) != '\0') {
      const char *pch;

      if ((pch = strchr((xdigits = xdigits_l), ch)) == NULL)
         pch = strchr((xdigits = xdigits_u), ch);
      if (pch != NULL) {
         val <<= 4;
         val |= (pch - xdigits);
         if (val > 0xffff)
            return (0);
         saw_xdigit = 1;
         continue;
      }
      if (ch == ':') {
         curtok = src;
         if (!saw_xdigit) {
            if (colonp)
               return (0);
            colonp = tp;
            continue;
         }
         if (tp + NS_INT16SZ > endp)
            return (0);
         *tp++ = (u_char) (val >> 8) & 0xff;
         *tp++ = (u_char) val & 0xff;
         saw_xdigit = 0;
         val = 0;
         continue;
      }
      if (ch == '.' && ((tp + NS_INADDRSZ) <= endp) &&
          inet_pton4(curtok, tp) > 0) {
         tp += NS_INADDRSZ;
         saw_xdigit = 0;
         break;	/* '\0' was seen by inet_pton4(). */
      }
      return (0);
   }
   if (saw_xdigit) {
      if (tp + NS_INT16SZ > endp)
         return (0);
      *tp++ = (u_char) (val >> 8) & 0xff;
      *tp++ = (u_char) val & 0xff;
   }
   if (colonp != NULL) {
      /*
       * Since some memmove()'s erroneously fail to handle
       * overlapping regions, we'll do the shift by hand.
       */
      const int n = tp - colonp;
      int i;

      for (i = 1; i <= n; i++) {
         endp[- i] = colonp[n - i];
         colonp[n - i] = 0;
      }
      tp = endp;
   }
   if (tp != endp)
      return (0);
   memcpy(dst, tmp, NS_IN6ADDRSZ);
   return (1);
}

#endif

/* int
 * inet_pton4(src, dst)
 *	like inet_aton() but without all the hexadecimal and shorthand.
 * return:
 *	1 if `src' is a valid dotted quad, else 0.
 * notice:
 *	does not touch `dst' unless it's returning 1.
 * author:
 *	Paul Vixie, 1996.
 */
static int
inet_pton4(const char *src, u_char *dst)
{
   static const char digits[] = "0123456789";
   int saw_digit, octets, ch;
   u_char tmp[NS_INADDRSZ], *tp;

   saw_digit = 0;
   octets = 0;
   *(tp = tmp) = 0;
   while ((ch = *src++) != '\0') {
      const char *pch;

      if ((pch = strchr(digits, ch)) != NULL) {
         u_int newVal = *tp * 10 + (pch - digits);

         if (newVal > 255)
            return (0);
         *tp = newVal;
         if (! saw_digit) {
            if (++octets > 4)
               return (0);
            saw_digit = 1;
         }
      } else if (ch == '.' && saw_digit) {
         if (octets == 4)
            return (0);
         *++tp = 0;
         saw_digit = 0;
      } else
         return (0);
   }
   if (octets < 4)
      return (0);

   memcpy(dst, tmp, NS_INADDRSZ);
   return (1);
}
