/*
Copyright (c) 2007, Adobe Systems, Incorporated
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

* Redistributions of source code must retain the above copyright
  notice, this list of conditions and the following disclaimer.

* Redistributions in binary form must reproduce the above copyright
  notice, this list of conditions and the following disclaimer in the
  documentation and/or other materials provided with the distribution.

* Neither the name of Adobe Systems, Network Resonance nor the names of its
  contributors may be used to endorse or promote products derived from
  this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/



static char *RCSSTRING __UNUSED__="$Id: nr_socket_local.c,v 1.2 2008/04/28 17:59:02 ekr Exp $";

#include <csi_platform.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#include <mswsock.h>
#else
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#endif
#include <assert.h>
#include <errno.h>
#include "nr_api.h"
#include "nr_socket.h"

#ifdef nr_socket_local_create
#undef nr_socket_local_create
#endif

#include "nr_socket_local.h"


typedef struct nr_socket_local_ {
  nr_transport_addr my_addr;
  NR_SOCKET sock;
} nr_socket_local;


static int nr_socket_local_destroy(void **objp);
static int nr_socket_local_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *to);
static int nr_socket_local_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *from);
static int nr_socket_local_getfd(void *obj, NR_SOCKET *fd);
static int nr_socket_local_getaddr(void *obj, nr_transport_addr *addrp);
static int nr_socket_local_close(void *obj);

static nr_socket_vtbl nr_socket_local_vtbl={
  nr_socket_local_destroy,
  nr_socket_local_sendto,
  nr_socket_local_recvfrom,
  nr_socket_local_getfd,
  nr_socket_local_getaddr,
  nr_socket_local_close
};

int nr_socket_local_create(nr_transport_addr *addr, nr_socket **sockp)
  {
    int r,_status;
    nr_socket_local *lcl=0;
    struct sockaddr_in6 addr6;
    unsigned int namelen=sizeof(addr6);
    int stype;

    switch(addr->protocol){
      case IPPROTO_TCP:
        stype=SOCK_STREAM;
        ABORT(R_INTERNAL); /* Can't happen for now */
        break;
      case IPPROTO_UDP:
        stype=SOCK_DGRAM;
        break;
      default:
        ABORT(R_BAD_ARGS);
    }

    if(!(lcl=RCALLOC(sizeof(nr_socket_local))))
      ABORT(R_NO_MEMORY);
    lcl->sock=-1;
      
    if((lcl->sock=socket(addr->addr->sa_family, stype, addr->protocol))<0){
      r_log(LOG_GENERIC,LOG_CRIT,"Couldn't create socket");
      //r_log_e(LOG_GENERIC,LOG_CRIT,"Couldn't create socket");
      ABORT(R_INTERNAL);
    }


    if(bind(lcl->sock, addr->addr, addr->addr_len)<0){
      r_log(LOG_GENERIC,LOG_CRIT,"Couldn't bind socket to address %s",addr->as_string);
      //r_log_e(LOG_GENERIC,LOG_CRIT,"Couldn't bind socket to address %s",addr->as_string);
      ABORT(R_INTERNAL);
    }

    r_log(LOG_GENERIC,LOG_DEBUG,"Creating socket %d with addr %s",lcl->sock,addr->as_string);


#ifdef WIN32
    /* See KB 263823: without this WSAIoctl() call, recvfrom() can get a
     * WSAECONNRESET if sendto() results in an "ICMP port unreachable" response.
     */
  {
    DWORD dwBytesReturned = 0;
    BOOL bNewBehavior = FALSE;

    if(WSAIoctl(lcl->sock, SIO_UDP_CONNRESET, &bNewBehavior, sizeof(bNewBehavior),
                NULL, 0, &dwBytesReturned, NULL, NULL) == SOCKET_ERROR){
      r_log(LOG_GENERIC,LOG_CRIT,"Couldn't fix socket with WSAIoctl");
      // r_log_e(LOG_GENERIC,LOG_CRIT,"Couldn't fix socket with WSAIoctl");
      ABORT(R_INTERNAL);
    }
  
  }
#endif

    nr_transport_addr_copy(&lcl->my_addr,addr);

    /* If we have a wildcard port, patch up the addr */
    if(nr_transport_addr_is_wildcard(addr)){
      if(getsockname(lcl->sock,(struct sockaddr *)&addr6,&namelen)<0){
        r_log(LOG_GENERIC,LOG_ERR,"Couldn't get socket address");
        //r_log_e(LOG_GENERIC,LOG_ERR,"Couldn't get socket address");
        ABORT(R_INTERNAL);
      }
      if(r=nr_sockaddr_to_transport_addr((struct sockaddr *)&addr6,namelen,
        addr->protocol,1,&lcl->my_addr))
        ABORT(r);
    }
    
    if(r=nr_socket_create_int(lcl, &nr_socket_local_vtbl, sockp))
      ABORT(r);

    _status=0;
  abort:
    if(_status){
      nr_socket_local_destroy((void **)&lcl);
    }
    return(_status);
  }


static int nr_socket_local_destroy(void **objp)
  {
    nr_socket_local *lcl;

    if(!objp || !*objp)
      return(0);

    lcl=*objp;
    *objp=0;

    //r_log(LOG_GENERIC,LOG_DEBUG,"Destroying sock (%x:%d)",lcl,lcl->sock);

    if(lcl->sock!=-1)
      NR_SOCKET_CLOSE(lcl->sock);

    RFREE(lcl);
    
    return(0);
  }

static int nr_socket_local_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *addr)
  {
    int r,_status;
    nr_socket_local *lcl=obj;
    int level;

    if(lcl->sock==-1)
      ABORT(R_EOD);

    //r_log(LOG_GENERIC,LOG_DEBUG,"Writing to sock (%x:%d), len=%d",lcl,lcl->sock,len);

    if((r=sendto(lcl->sock, msg, len, flags, addr->addr, addr->addr_len))<0){
      switch (errno) {
      case EHOSTUNREACH:
          level = LOG_INFO;
          break;
      default:
          level = LOG_ERR;
          break;
      }
      r_log_e(LOG_GENERIC,level,"Error in sendto %s", addr->as_string);

      ABORT(R_IO_ERROR);
    }

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_local_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *addr)
  {
    int r,_status;
    nr_socket_local *lcl=obj;
    struct sockaddr_in6 from; // big enough for v4
    socklen_t fromlen=sizeof(from);

    if((r=recvfrom(lcl->sock, buf, maxlen, flags, (struct sockaddr *)&from,
      &fromlen))<0){
      r_log_e(LOG_GENERIC,LOG_ERR,"Error in recvfrom");
      ABORT(R_IO_ERROR);
    }
    *len=r;

    if(r=nr_sockaddr_to_transport_addr((struct sockaddr *)&from,fromlen,
      lcl->my_addr.protocol,0,addr))
      ABORT(r);
    
    //r_log(LOG_GENERIC,LOG_DEBUG,"Read %d bytes from %s",*len,addr->as_string);

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_local_getfd(void *obj, NR_SOCKET *fd)
  {
    nr_socket_local *lcl=obj;

    if(lcl->sock==-1)
      return(R_BAD_ARGS);

    *fd=lcl->sock;

    return(0);
  }

static int nr_socket_local_getaddr(void *obj, nr_transport_addr *addrp)
  {
    nr_socket_local *lcl=obj;
      
    nr_transport_addr_copy(addrp,&lcl->my_addr);

    return(0);
  }

static int nr_socket_local_close(void *obj)
  {
    nr_socket_local *lcl=obj;

    //r_log(LOG_GENERIC,LOG_DEBUG,"Closing sock (%x:%d)",lcl,lcl->sock);

    if(lcl->sock!=-1){
      NR_ASYNC_CANCEL(lcl->sock,NR_ASYNC_WAIT_READ);
      NR_ASYNC_CANCEL(lcl->sock,NR_ASYNC_WAIT_WRITE);
      NR_SOCKET_CLOSE(lcl->sock);
    }
    lcl->sock=-1;

    return(0);
  }
