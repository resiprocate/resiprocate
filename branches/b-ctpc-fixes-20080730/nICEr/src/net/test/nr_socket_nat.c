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



static char *RCSSTRING __UNUSED__="$Id: nr_socket_nat.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";

#ifdef nr_socket_local_create
#undef nr_socket_local_create
#endif

#include <csi_platform.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "nr_api.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "transport_addr.h"
#include "registry.h"
#include "nr_socket_nat.h"


static char *nr_socket_nat_magic_cookie = "nr_socket_nat";

typedef struct nr_socket_nat_addr_ {
  nr_transport_addr addr;

  STAILQ_ENTRY(nr_socket_nat_addr_) entry;
} nr_socket_nat_addr;

typedef STAILQ_HEAD(nr_socket_nat_addr_head_, nr_socket_nat_addr_) nr_socket_nat_addr_head;

typedef struct nr_socket_nat_binding_ {
  nr_transport_addr internal_addr;
  nr_transport_addr external_addr;

  nr_socket_nat_addr_head remote_addrs;
  
  UINT4 creation_time;

  STAILQ_ENTRY(nr_socket_nat_binding_) entry;
} nr_socket_nat_binding;

typedef STAILQ_HEAD(nr_socket_nat_binding_head_, nr_socket_nat_binding_) nr_socket_nat_binding_head;

typedef struct nr_socket_nat_ctx_ {
  int mapping;
#define NR_SOCKET_NAT_TYPE_ADDRESS_INDEPENDENT 1
#define NR_SOCKET_NAT_TYPE_ADDRESS_DEPENDENT 2
  int filtering;
#define NR_SOCKET_NAT_TYPE_ADDRESS_INDEPENDENT 1
#define NR_SOCKET_NAT_TYPE_ADDRESS_DEPENDENT 2
  UINT4 internal_net;
  UINT4 internal_netmask;
  char *external_addr;

  nr_socket_nat_binding_head bindings;
} nr_socket_nat_ctx;

static nr_socket_nat_ctx  *our_nat=0;

typedef struct nr_socket_nat_ {
  char *magic_cookie;
  nr_socket_nat_ctx *ctx;
  nr_socket *sock;
  nr_transport_addr internal_addr;
  nr_transport_addr external_addr;
} nr_socket_nat;


static int nr_socket_nat_ctx_find_binding_outgoing(nr_socket_nat_ctx *ctx, nr_transport_addr *src, nr_transport_addr *dst, nr_socket_nat_binding **bindingp);
static int nr_socket_nat_ctx_find_binding_incoming(nr_socket_nat_ctx *ctx, nr_transport_addr *src, nr_transport_addr *dst, nr_socket_nat_binding **bindingp);


static int nr_socket_nat_destroy(void **objp);
static int nr_socket_nat_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *to);
static int nr_socket_nat_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *from);
static int nr_socket_nat_getfd(void *obj, NR_SOCKET *fd);
static int nr_socket_nat_getaddr(void *obj, nr_transport_addr *addrp);
static int nr_socket_nat_close(void *obj);


static nr_socket_vtbl nr_socket_nat_vtbl={
  nr_socket_nat_destroy,
  nr_socket_nat_sendto,
  nr_socket_nat_recvfrom,
  nr_socket_nat_getfd,
  nr_socket_nat_getaddr,
  nr_socket_nat_close
};



int nr_socket_nat_ctx_create(char *name, nr_socket_nat_ctx **ctxp)
  {
    nr_socket_nat_ctx *ctx=0;
    int r,_status;
    char prefix[1024];
    char indep;
    char *network_string=0;

    if(!(ctx=RCALLOC(sizeof(nr_socket_nat_ctx))))
      ABORT(R_NO_MEMORY);

    snprintf(prefix,sizeof(prefix),"nr_socket_nat.%s",name);
    
    /* NAT type */
    if(r=NR_reg_get2_char(prefix,"map_address_independent",&indep))
      ABORT(r);
    if(indep){
      ctx->mapping=NR_SOCKET_NAT_TYPE_ADDRESS_INDEPENDENT;
    }
    else{
      ctx->mapping=NR_SOCKET_NAT_TYPE_ADDRESS_DEPENDENT;
      UNIMPLEMENTED;
    }

    if(r=NR_reg_get2_char(prefix,"filter_address_independent",&indep))
      ABORT(r);
    if(indep){
      ctx->filtering=NR_SOCKET_NAT_TYPE_ADDRESS_INDEPENDENT;
    }
    else{
      ctx->filtering=NR_SOCKET_NAT_TYPE_ADDRESS_DEPENDENT;
    }

    /* Internal network: note, no sanity checking */
    if(r=NR_reg_alloc2_string(prefix,"network",&network_string))
      ABORT(r);
    ctx->internal_net=ntohl(inet_addr(network_string));
    if(r=NR_reg_get2_uint4(prefix,"netmask",&ctx->internal_netmask))
      ABORT(r);
    assert((ctx->internal_net & ~ctx->internal_netmask)==0);

    /* External addr */
    if(r=NR_reg_alloc2_string(prefix,"external_addr",&ctx->external_addr))
      ABORT(r);


    STAILQ_INIT(&ctx->bindings);
    *ctxp=ctx;

    _status=0;
  abort:
    RFREE(network_string);
    return(_status);
  }


static int nr_socket_nat_ctx_find_binding_outgoing(nr_socket_nat_ctx *ctx, nr_transport_addr *src, nr_transport_addr *dst, nr_socket_nat_binding **bindingp)
  {
    int _status;
    nr_socket_nat_binding *b;
    
    b=STAILQ_FIRST(&ctx->bindings);
    while(b){
      if(nr_transport_addr_cmp(src,&b->internal_addr,NR_TRANSPORT_ADDR_CMP_MODE_ALL))
        goto next_binding;

      /* Add to the filter list if necessary */
      if(ctx->filtering==NR_SOCKET_NAT_TYPE_ADDRESS_DEPENDENT){
        nr_socket_nat_addr *addr;

        addr=STAILQ_FIRST(&b->remote_addrs);
        while(addr){
          if(!nr_transport_addr_cmp(dst,&addr->addr,NR_TRANSPORT_ADDR_CMP_MODE_ALL))
            break;
          addr=STAILQ_NEXT(addr,entry);
        }
        
        if(!addr){
          if(!(addr=RMALLOC(sizeof(nr_socket_nat_addr))))
            ABORT(R_NO_MEMORY);
          nr_transport_addr_copy(&addr->addr,dst);
          STAILQ_INSERT_TAIL(&b->remote_addrs,addr,entry);
        }
      }
      
      *bindingp=b;
      break;
      
    next_binding:
      b=STAILQ_NEXT(b,entry);
    }

    if(!b)
      ABORT(R_NOT_FOUND);

    _status=0;
  abort:
    return(_status);
  }



static int nr_socket_nat_ctx_find_binding_incoming(nr_socket_nat_ctx *ctx, nr_transport_addr *src, nr_transport_addr *dst, nr_socket_nat_binding **bindingp)
  {
    int _status;
    nr_socket_nat_binding *b;
    
    b=STAILQ_FIRST(&ctx->bindings);
    while(b){
      if(nr_transport_addr_cmp(dst,&b->external_addr,NR_TRANSPORT_ADDR_CMP_MODE_ALL))
        goto next_binding;


      if(ctx->filtering==NR_SOCKET_NAT_TYPE_ADDRESS_DEPENDENT){
        nr_socket_nat_addr *addr;

        addr=STAILQ_FIRST(&b->remote_addrs);
        while(addr){
          if(!nr_transport_addr_cmp(src,&addr->addr,NR_TRANSPORT_ADDR_CMP_MODE_ALL))
            break;
          addr=STAILQ_NEXT(addr,entry);
        }
        
        if(!addr)
          goto next_binding;
      }
      
      *bindingp=b;
      break;
      
    next_binding:
      b=STAILQ_NEXT(b,entry);
    }

    if(!b)
      ABORT(R_NOT_FOUND);

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_nat_ctx_create_binding(nr_socket_nat_ctx *ctx, nr_transport_addr *internal, nr_transport_addr *external, nr_transport_addr *remote,nr_socket_nat_binding **bindingp)
  {
    nr_socket_nat_binding *b=0;
    int r,_status;
    nr_socket_nat_addr *addr;

    if(!(b=RCALLOC(sizeof(nr_socket_nat_binding))))
      ABORT(R_INTERNAL);        
    STAILQ_INIT(&b->remote_addrs);


    if(r=nr_transport_addr_copy(&b->internal_addr, internal))
      ABORT(r);
    if(r=nr_transport_addr_copy(&b->external_addr, external))
      ABORT(r);
    
    if(remote){
      if(!(addr=RMALLOC(sizeof(nr_socket_nat_addr))))
        ABORT(R_NO_MEMORY);
      nr_transport_addr_copy(&addr->addr,remote);
      STAILQ_INSERT_TAIL(&b->remote_addrs,addr,entry);
    }
    STAILQ_INSERT_TAIL(&ctx->bindings, b, entry);

    *bindingp=b;
    _status=0;
  abort:
    return(_status);
  }


int nr_socket_nat_create(nr_transport_addr *addr, nr_socket **sockp)
  {
    int r,_status;
    nr_socket *sock;
    nr_transport_addr ext_addr;
    int port;

    if(!our_nat){
      if(r=nr_socket_nat_ctx_create("NAT",&our_nat))
        ABORT(r);
    }
   
    /* Note: we don't port translate! */
    if(r=nr_transport_addr_get_port(addr,&port))
      ABORT(r);
    if(r=nr_ip4_str_port_to_transport_addr(our_nat->external_addr,
      port,IPPROTO_UDP,&ext_addr))
      ABORT(r);

    if(r=nr_socket_local_create(&ext_addr,&sock))
      ABORT(r);

    if(r=nr_socket_nat_create2(addr, sock,sockp))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }

int nr_socket_nat_create2(nr_transport_addr *addr, nr_socket *sock, nr_socket **sockp)
  {
    int r,_status;
    nr_socket_nat *nat=0;
    nr_socket_nat_binding *binding;


    if(!(nat=RCALLOC(sizeof(nr_socket_nat))))
      ABORT(R_NO_MEMORY);

    nat->magic_cookie = nr_socket_nat_magic_cookie;
    nat->sock = sock;
    nat->ctx=our_nat;
    
    if(r=nr_transport_addr_copy(&nat->internal_addr, addr))
      ABORT(r);
    if(r=nr_socket_getaddr(sock, &nat->external_addr))
      ABORT(r);
    if(r=nr_socket_nat_ctx_create_binding(nat->ctx, &nat->internal_addr,
      &nat->external_addr, 0, &binding))
      ABORT(r);

    if(r=nr_socket_create_int(nat, &nr_socket_nat_vtbl, sockp))
      ABORT(r);

    _status=0;
  abort:
    if(_status){
      nr_socket_nat_destroy((void **)&nat);
    }
    return(_status);
  }


static int nr_socket_nat_destroy(void **objp)
  {
    int _status;
    nr_socket_nat *nat;

    if(!objp || !*objp)
      return(0);

    nat=*objp;
    *objp=0;

    assert(nat->magic_cookie == nr_socket_nat_magic_cookie);

    if(nat->sock!=0)
      nr_socket_destroy(&nat->sock);

    RFREE(nat);

    _status=0;
/*  abort:*/
    return(_status);
  }


static int nr_socket_nat_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *addr)
  {
    int r,_status;
    nr_socket_nat *nat=obj;
    nr_socket_nat_binding *binding;
    UINT4 a_int;

    assert(nat->magic_cookie == nr_socket_nat_magic_cookie);
    
    /* Check for attempts to send to own NAT */
    if(r=nr_transport_addr_get_ip4(addr,&a_int))
      ABORT(r);
    assert((a_int & nat->ctx->internal_netmask) != nat->ctx->internal_net);
    
    if(r=nr_socket_nat_ctx_find_binding_outgoing(nat->ctx,
      &nat->internal_addr,addr,&binding))
      ABORT(r);
    
    if ((r=nr_socket_sendto(nat->sock,msg,len,flags,addr)))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_nat_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *addr)
  {
    int r,_status;
    nr_socket_nat *nat=obj;
    nr_socket_nat_binding *binding;

    assert(nat->magic_cookie == nr_socket_nat_magic_cookie);

    if ((r=nr_socket_recvfrom(nat->sock,buf,maxlen,len,flags,addr)))
      ABORT(r);

    if (r=nr_socket_nat_ctx_find_binding_incoming(nat->ctx,
      addr, &nat->external_addr, &binding)){
      if(r==R_NOT_FOUND)
        ABORT(R_WOULDBLOCK);
      ABORT(r);
    }

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_nat_getfd(void *obj, NR_SOCKET *fd)
  {
    nr_socket_nat *nat=obj;

    assert(nat->magic_cookie == nr_socket_nat_magic_cookie);
    return nr_socket_getfd(nat->sock,fd);
  }

static int nr_socket_nat_getaddr(void *obj, nr_transport_addr *addrp)
  {
    nr_socket_nat *nat=obj;

    assert(nat->magic_cookie == nr_socket_nat_magic_cookie);
    return nr_transport_addr_copy(addrp,&nat->internal_addr);
  }

static int nr_socket_nat_close(void *obj)
  {
    nr_socket_nat *nat=obj;

    assert(nat->magic_cookie == nr_socket_nat_magic_cookie);
    return nr_socket_close(nat->sock);
  }

