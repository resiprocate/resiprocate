#include "ares.h"
#include "ares_local.h"



int ares_local_gethostbyname(ares_channel channel, const char *name, int family,
                              ares_host_callback callback, void *arg)
{
   (void)(channel,name,family,callback,arg);
   return 0;
}

int ares_local_gethostbyaddr(ares_channel channel, const char *addr, int addrlen,
                              int family, ares_host_callback callback, void*arg)
{
   (void)(channel,addr,addrlen,family,callback,arg);
   return 0;
}

int ares_local_query(ares_channel channel, const char *name, int dnsclass,
                      int type, ares_callback callback, void *arg)
{
   (void)(channel,name,dnsclass,type,callback,arg);
   return 0;
}

void ares_local_process_requests()
{
  return;
}
