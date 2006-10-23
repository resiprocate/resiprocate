#include <stdio.h>
#include <errno.h>
#include <openssl/bio.h>
#include <assert.h>

#define BIO_TYPE_DWRAP       (50 | 0x0400 | 0x0200)

static int dwrap_new(BIO *bio);
static int dwrap_free(BIO *a);
static int dwrap_read(BIO *b, char *out, int outl);
static int dwrap_write(BIO *b, const char *in, int inl);
static int dwrap_puts(BIO *b, const char *in);
static int dwrap_gets(BIO *b, char *buf, int size);
static long dwrap_ctrl(BIO *b, int cmd, long num, void *ptr);
static long dwrap_callback_ctrl(BIO *b, int cmd, bio_info_cb *fp);

static BIO_METHOD methods_dwrap={
     BIO_TYPE_DWRAP,
     "dtls_wrapper",
     dwrap_write,
     dwrap_read,
     dwrap_puts,
     dwrap_gets,
     dwrap_ctrl,
     dwrap_new,
     dwrap_free,
     dwrap_callback_ctrl
};

typedef struct BIO_F_DWRAP_CTX_ {
     int dgram_timer_exp;
} BIO_F_DWRAP_CTX;


BIO_METHOD *BIO_f_dwrap(void) {
  return(&methods_dwrap);
}

static int dwrap_new(BIO *bi) {
  BIO_F_DWRAP_CTX *ctx=OPENSSL_malloc(sizeof(BIO_F_BUFFER_CTX));
  if(!ctx) return(0);

  memset(ctx,0,sizeof(BIO_F_BUFFER_CTX));
  
  bi->init=1;
  bi->ptr=(char *)ctx;
  bi->flags=0;

  return 1;
}

static int dwrap_free(BIO *a){
  if(a) return 0;
  
  free(a);
  return 1;
}

static int dwrap_read(BIO *b, char *out, int outl){
  if(!b || !out) return 0;
  int ret;
  
  BIO_clear_retry_flags(b);
  
  ret=BIO_read(b->next_bio,out,outl);

  if(ret<=0)
    BIO_copy_next_retry(b);

  return ret;
}

static int dwrap_write(BIO *b, const char *in, int inl){
  if(!b || !in || (inl<=0)) return 0;

  return BIO_write(b->next_bio,in,inl);
}

static int dwrap_puts(BIO *b, const char *in){
  assert(0);

  return 0;
}
    
static int dwrap_gets(BIO *b, char *buf, int size){
  assert(0);

  return 0;
}

static long dwrap_ctrl(BIO *b, int cmd, long num, void *ptr){
  long ret;
  BIO_F_DWRAP_CTX *ctx;

  ctx=b->ptr;

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
      ret=BIO_ctrl(b->next_bio,cmd,num,ptr);
      break;
  }

  return ret;
}

static long dwrap_callback_ctrl(BIO *b, int cmd, bio_info_cb *fp){
  long ret;

  ret=BIO_callback_ctrl(b->next_bio,cmd,fp);

  return ret;
}
     
     
