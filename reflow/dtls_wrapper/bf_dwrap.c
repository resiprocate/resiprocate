#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef USE_SSL 

#include <stdio.h>
#include <errno.h>
#include <openssl/bio.h>
#include <openssl/opensslv.h>
#include "rutil/ResipAssert.h"
#include <memory.h>

#if OPENSSL_VERSION_NUMBER < 0x10100000L

static inline BIO_METHOD *BIO_meth_new(int type, const char *name)
{
  BIO_METHOD *biom = calloc(1, sizeof(BIO_METHOD));

  if (biom != NULL) {
    biom->type = type;
    biom->name = name;
  }
  return biom;
}

#define BIO_meth_set_write(b, f) (b)->bwrite = (f)
#define BIO_meth_set_read(b, f) (b)->bread = (f)
#define BIO_meth_set_puts(b, f) (b)->bputs = (f)
#define BIO_meth_set_gets(b, f) (b)->bgets = (f)
#define BIO_meth_set_ctrl(b, f) (b)->ctrl = (f)
#define BIO_meth_set_create(b, f) (b)->create = (f)
#define BIO_meth_set_destroy(b, f) (b)->destroy = (f)
#define BIO_meth_set_callback_ctrl(b, f) (b)->callback_ctrl = (f)

#define BIO_set_init(b, val) (b)->init = (val)
#define BIO_set_data(b, val) (b)->ptr = (val)
#define BIO_get_data(b) (b)->ptr

#endif /* OPENSSL_VERSION_NUMBER < 0x10100000L */

#define BIO_TYPE_DWRAP       (50 | 0x0400 | 0x0200)

static int dwrap_new(BIO *bio);
static int dwrap_free(BIO *a);
static int dwrap_read(BIO *b, char *out, int outl);
static int dwrap_write(BIO *b, const char *in, int inl);
static int dwrap_puts(BIO *b, const char *in);
static int dwrap_gets(BIO *b, char *buf, int size);
static long dwrap_ctrl(BIO *b, int cmd, long num, void *ptr);
static long dwrap_callback_ctrl(BIO *b, int cmd, bio_info_cb *fp);

typedef struct BIO_F_DWRAP_CTX_ 
{
   int dgram_timer_exp;
} BIO_F_DWRAP_CTX;


BIO_METHOD *BIO_f_dwrap(void) 
{
   BIO_METHOD *meth = BIO_meth_new(BIO_TYPE_DWRAP, "dtls_wrapper");
   BIO_meth_set_write(meth, dwrap_write);
   BIO_meth_set_read(meth, dwrap_read);
   BIO_meth_set_puts(meth, dwrap_puts);
   BIO_meth_set_gets(meth, dwrap_gets);
   BIO_meth_set_ctrl(meth, dwrap_ctrl);
   BIO_meth_set_create(meth, dwrap_new);
   BIO_meth_set_destroy(meth, dwrap_free);
   BIO_meth_set_callback_ctrl(meth, dwrap_callback_ctrl);
   return meth;
}

static int dwrap_new(BIO *bi) 
{
   BIO_F_DWRAP_CTX *ctx=OPENSSL_malloc(sizeof(BIO_F_DWRAP_CTX));
   if(!ctx) return(0);

   memset(ctx,0,sizeof(BIO_F_DWRAP_CTX));

   BIO_set_init(bi, 1);
   BIO_set_data(bi, ctx);
   // bi->flags=0;

   return 1;
}

static int dwrap_free(BIO *a)
{
   if(!a) return 0;

   OPENSSL_free(BIO_get_data(a));
   BIO_set_data(a, NULL);
   BIO_set_init(a, 0);

   return 1;
}

static int dwrap_read(BIO *b, char *out, int outl)
{
   int ret;
   if(!b || !out) return 0;


   BIO_clear_retry_flags(b);

   ret=BIO_read(BIO_next(b),out,outl);

   if(ret<=0)
      BIO_copy_next_retry(b);

   return ret;
}

static int dwrap_write(BIO *b, const char *in, int inl)
{
   if(!b || !in || (inl<=0)) return 0;

   return BIO_write(BIO_next(b),in,inl);
}

static int dwrap_puts(BIO *b, const char *in)
{
   resip_assert(0);

   return 0;
}

static int dwrap_gets(BIO *b, char *buf, int size)
{
   resip_assert(0);

   return 0;
}

static long dwrap_ctrl(BIO *b, int cmd, long num, void *ptr)
{
   long ret;
   BIO_F_DWRAP_CTX *ctx;

   ctx=BIO_get_data(b);

   switch(cmd){
    case BIO_CTRL_DGRAM_GET_RECV_TIMER_EXP:
       if(ctx->dgram_timer_exp){
          ret=1;
          ctx->dgram_timer_exp=0;
       }
       else
          ret=0;
       break;

       /* Overloading this */
    case BIO_CTRL_DGRAM_SET_RECV_TIMEOUT:
       ctx->dgram_timer_exp=1;
       ret=1;
       break;
    default:
       ret=BIO_ctrl(BIO_next(b),cmd,num,ptr);
       break;
   }

   return ret;
}

static long dwrap_callback_ctrl(BIO *b, int cmd, bio_info_cb *fp)
{
   long ret;

   ret=BIO_callback_ctrl(BIO_next(b),cmd,fp);

   return ret;
}

#endif //USE_SSL
     

/* ====================================================================

 Copyright (c) 2007-2008, Eric Rescorla and Derek MacDonald 
 All rights reserved.
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:
 
 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 
 
 3. None of the contributors names may be used to endorse or promote 
    products derived from this software without specific prior written 
    permission. 
 
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

 ==================================================================== */
