#ifndef ARES_LOCAL__H
#define ARES_LOCAL__H
/*
** Define a suite of callbacks that can return locally configured DNS results.
** Very useful for testing or simulation.
**
** Returns 0 when unable to handle request, non-zero when handled (and callbacks invoked).
**
*/

int ares_local_gethostbyname(ares_channel channel, const char *name, int family,
                              ares_host_callback callback, void *arg);

int ares_local_gethostbyaddr(ares_channel channel, const char *addr, int addrlen,
                              int family, ares_host_callback callback, void*arg);

int ares_local_query(ares_channel channel, const char *name, int dnsclass,
                      int type, ares_callback callback, void *arg);


void ares_local_process_requests();

#endif
