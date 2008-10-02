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



static char *RCSSTRING __UNUSED__="$Id: nr_socket_evil.c,v 1.2 2008/04/28 17:59:03 ekr Exp $";

#ifdef nr_socket_local_create
#undef nr_socket_local_create
#endif

#include <csi_platform.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <assert.h>
#include "nr_api.h"
#include "nr_socket.h"
#include "nr_socket_local.h"
#include "transport_addr.h"
#include "registry.h"
#include "nr_socket_evil.h"


static char *nr_socket_evil_magic_cookie = "nr_socket_evil";

typedef enum { EVIL_FILTER_ACTION_DROP = 1,
               EVIL_FILTER_ACTION_PASS } evil_filter_action;

typedef struct evil_filter_ {
    NR_registry registry;
    evil_filter_action action;
    int flags;
    nr_transport_addr addr;
    int netmaskbits;
    STAILQ_ENTRY(evil_filter_) entry;
} evil_filter;

typedef STAILQ_HEAD(evil_filter_head_,evil_filter_) evil_filter_head;

typedef struct nr_socket_evil_ {
  char *magic_cookie;
  nr_socket *sock;
  evil_filter_head send_filters;
  evil_filter_head recv_filters;
} nr_socket_evil;

static int nr_socket_evil_destroy(void **objp);
static int nr_socket_evil_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *to);
static int nr_socket_evil_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *from);
static int nr_socket_evil_getfd(void *obj, NR_SOCKET *fd);
static int nr_socket_evil_getaddr(void *obj, nr_transport_addr *addrp);
static int nr_socket_evil_close(void *obj);
static void nr_socket_evil_dump_filters2(evil_filter_head *filters, char *direction);

static nr_socket_vtbl nr_socket_evil_vtbl={
  nr_socket_evil_destroy,
  nr_socket_evil_sendto,
  nr_socket_evil_recvfrom,
  nr_socket_evil_getfd,
  nr_socket_evil_getaddr,
  nr_socket_evil_close
};

#define NR_TRANSPORT_ADDR_CMP_FLAG_PROTOCOL  (1<<0)
#define NR_TRANSPORT_ADDR_CMP_FLAG_ADDR      (1<<1)
#define NR_TRANSPORT_ADDR_CMP_FLAG_PORT      (1<<2)
#define NR_TRANSPORT_ADDR_CMP_FLAG_MASK      (1<<3)

static int nr_socket_evil_transport_addr_cmp(nr_transport_addr *addr1,nr_transport_addr *addr2,int mask,int flags)
  {
    assert(flags);

    if(addr1->ip_version != addr2->ip_version)
      return(1);

    if(flags&NR_TRANSPORT_ADDR_CMP_FLAG_PROTOCOL) {
      if(addr1->protocol != addr2->protocol)
        return(1);
    }

    assert(addr1->addr_len == addr2->addr_len);
    switch(addr1->ip_version){
      case NR_IPV4:
        if(flags&NR_TRANSPORT_ADDR_CMP_FLAG_MASK) {
          if(mask>0){
            if((ntohl(addr1->u.addr4.sin_addr.s_addr)&((~0)<<(32-mask)))
             !=(ntohl(addr2->u.addr4.sin_addr.s_addr)&((~0)<<(32-mask))))
              return(1);
          }
        }
        else if(flags&NR_TRANSPORT_ADDR_CMP_FLAG_ADDR) {
          if(addr1->u.addr4.sin_addr.s_addr != 0
          && addr2->u.addr4.sin_addr.s_addr != 0
          && addr1->u.addr4.sin_addr.s_addr != addr2->u.addr4.sin_addr.s_addr)
            return(1);
        }
        if(flags&NR_TRANSPORT_ADDR_CMP_FLAG_PORT) {
          if(addr1->u.addr4.sin_port != 0
          && addr2->u.addr4.sin_port != 0
          && addr1->u.addr4.sin_port != addr2->u.addr4.sin_port)
            return(1);
        }
        break;
      case NR_IPV6:
        UNIMPLEMENTED;
      default:
        UNIMPLEMENTED;
    }

    return(0);
  }

static int nr_socket_evil_add_filter(NR_registry parent, evil_filter_head *filters, evil_filter_action action, int flags, char *address, UINT2 port, UCHAR mask)
{
    int r,_status;
    evil_filter *filter = 0;

    assert(flags);

    filter = RCALLOC(sizeof(*filter));
    if (!filter)
        ABORT(R_NO_MEMORY);

    strlcpy(filter->registry, parent, sizeof(filter->registry));
    filter->action = action;
    filter->flags = flags;

    /* the protocol is ignored, so just use UDP for the moment */
    if ((r=nr_ip4_str_port_to_transport_addr(address, port, IPPROTO_UDP, &filter->addr))) {
        r_log(LOG_GENERIC, LOG_ERR, "nr_socket_evil: bad adddress: '%s'", address);
        ABORT(R_FAILED);
    }

    filter->netmaskbits = mask;

    STAILQ_INSERT_TAIL(filters, filter, entry);

    _status=0;
  abort:
    if (_status) RFREE(filter);
    return(_status);
}

static int nr_socket_evil_configure_filters(evil_filter_head *filters, char *direction)
  {
    int r,_status;
    NR_registry parent;
    unsigned int nchildren;
    NR_registry *children = 0;
    unsigned int i;
    char *child;
    char action[10];
    char address[20];
    size_t tmp;
    UINT2 port;
    UCHAR mask;
    int gotaction, gotaddress, gotport, gotmask;
    evil_filter_action eaction;
    int flags;

    if ((r=NR_reg_make_registry("net.socket.evil", direction, parent)))
        ABORT(r);

    nchildren = 0;
    if ((r=NR_reg_get_child_count(parent, &nchildren))) {
        if (r != R_NOT_FOUND)
            ABORT(r);
    }

    children = RCALLOC((nchildren+1) * sizeof(*children));
    if (!children)
        ABORT(R_NO_MEMORY);

    if (nchildren == 0) {
        if ((r=nr_socket_evil_add_filter(parent, filters, EVIL_FILTER_ACTION_DROP, NR_TRANSPORT_ADDR_CMP_FLAG_MASK, "0.0.0.0", 0, 0)))
            ABORT(r);
    }
    else {
        if ((r=NR_reg_get_children(parent, children, (nchildren+1), &tmp)))
            ABORT(r);
        assert(tmp == nchildren);

        for (i = 0; i < nchildren; ++i) {
            child = children[i];

            gotaction = gotaddress = gotport = gotmask = 0;

            if (!NR_reg_get2_string(child, "action", action, sizeof(action)))
                gotaction = 1;

            if (!NR_reg_get2_string(child, "address", address, sizeof(address)))
                gotaddress = 1;

            if (!NR_reg_get2_uint2(child, "port", &port))
                gotport = 1;

            if (!NR_reg_get2_uchar(child, "netmaskbits", &mask))
                gotmask = 1;

            if (!gotaction) {
                r_log(LOG_GENERIC, LOG_ERR, "nr_socket_evil: action not specified: '%s'", child);
                continue;  /* CONTINUE */
            }

            if (strcasecmp("pass", action) == 0)
                eaction = EVIL_FILTER_ACTION_PASS;
            else
                eaction = EVIL_FILTER_ACTION_DROP;
            gotaction = 1;

            flags = 0;

            if (!gotaddress)
                strlcpy(address,"0.0.0.0",sizeof(address));
            if (!gotport)
                port = 0;
            if (!gotmask)
                mask = 32;

            if (!(gotaddress || gotport || gotmask)) {
                mask = 0;
                gotmask = 1;
            }

            if (gotaddress)
                flags |= NR_TRANSPORT_ADDR_CMP_FLAG_ADDR;
            if (gotport)
                flags |= NR_TRANSPORT_ADDR_CMP_FLAG_PORT;
            if (gotmask)
                flags |= NR_TRANSPORT_ADDR_CMP_FLAG_MASK;

            if ((r=nr_socket_evil_add_filter(child, filters, eaction, flags, address, port, mask)))
                ABORT(r);
        }
    }

    _status=0;
  abort:
    RFREE(children);
    return(_status);
  }

int nr_socket_evil_create(nr_transport_addr *addr, nr_socket **sockp)
  {
    int r,_status;
    nr_socket *sock;

    if(r=nr_socket_local_create(addr,&sock))
      ABORT(r);

    if(r=nr_socket_evil_create2(sock,sockp))
      ABORT(r);

    _status=0;
  abort:
    return(_status);
  }

int nr_socket_evil_create2(nr_socket *sock, nr_socket **sockp)
  {
    int r,_status;
    nr_socket_evil *evil=0;

    if(!(evil=RCALLOC(sizeof(nr_socket_evil))))
      ABORT(R_NO_MEMORY);

    evil->magic_cookie = nr_socket_evil_magic_cookie;
    evil->sock = sock;

    STAILQ_INIT(&evil->send_filters);
    STAILQ_INIT(&evil->recv_filters);

    nr_socket_evil_configure_filters(&evil->send_filters, "send");
    nr_socket_evil_configure_filters(&evil->recv_filters, "recv");

    if(r=nr_socket_create_int(evil, &nr_socket_evil_vtbl, sockp))
      ABORT(r);

    _status=0;
  abort:
    if(_status){
      nr_socket_evil_destroy((void **)&evil);
    }
    return(_status);
  }

void nr_socket_evil_always_pass(nr_socket *sock)
{
    int r,_status;
    nr_socket_evil *evil=(nr_socket_evil*)sock->obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);

    STAILQ_INIT(&evil->send_filters);
    STAILQ_INIT(&evil->recv_filters);

    if ((r=nr_socket_evil_add_filter("nr_socket_evil_always_pass", &evil->send_filters, EVIL_FILTER_ACTION_PASS, NR_TRANSPORT_ADDR_CMP_FLAG_MASK, "0.0.0.0", 0, 0)))
        ABORT(r);

    if ((r=nr_socket_evil_add_filter("nr_socket_evil_always_pass", &evil->recv_filters, EVIL_FILTER_ACTION_PASS, NR_TRANSPORT_ADDR_CMP_FLAG_MASK, "0.0.0.0", 0, 0)))
        ABORT(r);

    _status=0;
  abort:
    assert(_status == 0);
    return;
}

void nr_socket_evil_always_drop(nr_socket *sock)
{
    int r,_status;
    nr_socket_evil *evil=(nr_socket_evil*)sock->obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);

    STAILQ_INIT(&evil->send_filters);
    STAILQ_INIT(&evil->recv_filters);

    if ((r=nr_socket_evil_add_filter("nr_socket_evil_always_drop", &evil->send_filters, EVIL_FILTER_ACTION_DROP, NR_TRANSPORT_ADDR_CMP_FLAG_MASK, "0.0.0.0", 0, 0)))
        ABORT(r);

    if ((r=nr_socket_evil_add_filter("nr_socket_evil_always_drop", &evil->recv_filters, EVIL_FILTER_ACTION_DROP, NR_TRANSPORT_ADDR_CMP_FLAG_MASK, "0.0.0.0", 0, 0)))
        ABORT(r);

    _status=0;
  abort:
    assert(_status == 0);
    return;
}

void nr_socket_evil_dump_filters2(evil_filter_head *filters, char *direction)
{
    evil_filter *f;
    char buf[sizeof(f->addr.as_string)];
    char *c;

    STAILQ_FOREACH(f, filters, entry) {
          fprintf(stderr, "[ %s ]", f->registry);

          fprintf(stderr, "  %s on %s", ((f->action == EVIL_FILTER_ACTION_PASS) ? "pass" : "drop"), direction);
          
          if (f->flags & NR_TRANSPORT_ADDR_CMP_FLAG_ADDR) {
              strcpy(buf, &f->addr.as_string[4]);
              c = strchr(buf, ':'); 
              *c = '\0';
              fprintf(stderr, " address %s", buf);
          }
          if (f->flags & NR_TRANSPORT_ADDR_CMP_FLAG_MASK) {
              if (f->netmaskbits == 0)
                  fprintf(stderr, " all");
              else 
                  fprintf(stderr, " netmaskbits /%d", f->netmaskbits);
          }
          if (f->flags & NR_TRANSPORT_ADDR_CMP_FLAG_PORT) {
              strcpy(buf, &f->addr.as_string[4]);
              fprintf(stderr, " port %s", (strchr(buf, ':') + 1));
          }
          if (f->flags & NR_TRANSPORT_ADDR_CMP_FLAG_PROTOCOL)
              fprintf(stderr, " protocol");
          fprintf(stderr, "\n");
    }
}

void nr_socket_evil_dump_filters(nr_socket *sock)
{
    nr_socket_evil *evil=(nr_socket_evil*)sock->obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);

    nr_socket_evil_dump_filters2(&evil->send_filters, "send");
    nr_socket_evil_dump_filters2(&evil->recv_filters, "recv");
}

static int nr_socket_evil_destroy(void **objp)
  {
    int _status;
    nr_socket_evil *evil;
    evil_filter *f1, *f2;

    if(!objp || !*objp)
      return(0);

    evil=*objp;
    *objp=0;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);

    if(evil->sock!=0)
      nr_socket_destroy(&evil->sock);

    STAILQ_FOREACH_SAFE(f1, &evil->send_filters, entry, f2)
        RFREE(f1);

    STAILQ_FOREACH_SAFE(f1, &evil->recv_filters, entry, f2)
        RFREE(f1);

    RFREE(evil);

    _status=0;
/*  abort:*/
    return(_status);
  }

static int nr_socket_evil_pass(evil_filter_head *filters, nr_transport_addr *addr)
  {
      int matches;
      evil_filter *f;

      STAILQ_FOREACH(f, filters, entry) {
          matches = !nr_socket_evil_transport_addr_cmp(addr, &f->addr, f->netmaskbits, f->flags);
          if (matches)
              return (f->action == EVIL_FILTER_ACTION_PASS);  /* RETURN */
      }

      return 0;  /* drop */
  }

static int nr_socket_evil_sendto(void *obj,const void *msg, size_t len,
  int flags, nr_transport_addr *addr)
  {
    int r,_status;
    nr_socket_evil *evil=obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);

    if (nr_socket_evil_pass(&evil->send_filters, addr)) {
        if ((r=nr_socket_sendto(evil->sock,msg,len,flags,addr)))
            ABORT(r);
    }
    else {
        r_log(LOG_GENERIC, LOG_NOTICE, "nr_socket_evil: dropping packet to adddress: '%s'", addr->as_string);
    }

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_evil_recvfrom(void *obj,void * restrict buf,
  size_t maxlen, size_t *len, int flags, nr_transport_addr *addr)
  {
    int r,_status;
    nr_socket_evil *evil=obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);

    if ((r=nr_socket_recvfrom(evil->sock,buf,maxlen,len,flags,addr)))
        ABORT(r);

    if (! nr_socket_evil_pass(&evil->recv_filters, addr)) {
        r_log(LOG_GENERIC, LOG_NOTICE, "nr_socket_evil: dropping packet from adddress: '%s'", addr->as_string);
        *len = 0;
    }

    _status=0;
  abort:
    return(_status);
  }

static int nr_socket_evil_getfd(void *obj, NR_SOCKET *fd)
  {
    nr_socket_evil *evil=obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);
    return nr_socket_getfd(evil->sock,fd);
  }

static int nr_socket_evil_getaddr(void *obj, nr_transport_addr *addrp)
  {
    nr_socket_evil *evil=obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);
    return nr_socket_getaddr(evil->sock,addrp);
  }

static int nr_socket_evil_close(void *obj)
  {
    nr_socket_evil *evil=obj;

    assert(evil->magic_cookie == nr_socket_evil_magic_cookie);
    return nr_socket_close(evil->sock);
  }

