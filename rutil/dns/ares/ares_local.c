#include "ares.h"
#include "ares_local.h"



int ares_local_gethostbyname(ares_channel channel, const char *name, int family,
                              ares_host_callback callback, void *arg)
{
   (void)(channel);(void)(name);(void)(family);(void)(callback);(void)(arg);
   return 0;
}

int ares_local_gethostbyaddr(ares_channel channel, const char *addr, int addrlen,
                              int family, ares_host_callback callback, void*arg)
{
   (void)(channel);(void)(addr);(void)(addrlen);(void)(family);(void)(callback);(void)(arg);
   return 0;
}

int ares_local_query(ares_channel channel, const char *name, int dnsclass,
                      int type, ares_callback callback, void *arg)
{
   (void)(channel);(void)(name);(void)(dnsclass);(void)(type);(void)(callback);(void)(arg);
   return 0;
}

void ares_local_process_requests()
{
  return;
}
