*/

This distribution contains the C source files for Eric Rescorla's 
"SSL and TLS".

This software is Copyright (C) Eric Rescorla.
You are granted a right to use or redistribute this software for any
purpose and without fee, with or without attribution, provided that
you agree that this software is provided "as is" and without any
express or implied warranties, including, without limitation, the
implied warranties of merchantablity and fitness for a particular
purpose.

*/

/* A simple SSL echo server */

#ifndef _common_h
#define _common_h

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <fcntl.h>
#include <signal.h>

#include <openssl/ssl.h>

#define CA_LIST "root.pem"
#define HOST	"localhost"
#define RANDOM  "random.pem"
#define PORT	4433
#define BUFSIZZ 1024

extern BIO *bio_err;
int berr_exit (char *string);
int err_exit(char *string);

SSL_CTX *initialize_ctx(char *keyfile, char *password);
void destroy_ctx(SSL_CTX *ctx);

#endif

#ifndef _server_h
#define _server_h

#define KEYFILE "server.pem"
#define PASSWORD "password"
#define DHFILE "dh1024.pem"

int tcp_listen(void);
void load_dh_params(SSL_CTX *ctx,char *file);
void generate_eph_rsa_key(SSL_CTX *ctx);

#endif




BIO *bio_err=0;
static char *pass;
static int password_cb(char *buf,int num,int rwflag,void *userdata);
static void sigpipe_handle(int x);

/* A simple error and exit routine*/
int err_exit(string)
  char *string;
  {
    fprintf(stderr,"%s\n",string);
    exit(0);
  }

/* Print SSL errors and exit*/
int berr_exit(string)
  char *string;
  {
    BIO_printf(bio_err,"%s\n",string);
    ERR_print_errors(bio_err);
    exit(0);
  }

/*The password code is not thread safe*/
static int password_cb(char *buf,int num,int rwflag,void *userdata)
  {
    if(num<strlen(pass)+1)
      return(0);

    strcpy(buf,pass);
    return(strlen(pass));
  }

static void sigpipe_handle(int x){
}

SSL_CTX *initialize_ctx(keyfile,password)
  char *keyfile;
  char *password;
  {
    SSL_METHOD *meth;
    SSL_CTX *ctx;
    
    if(!bio_err){
      /* Global system initialization*/
      SSL_library_init();
      SSL_load_error_strings();
      
      /* An error write context */
      bio_err=BIO_new_fp(stderr,BIO_NOCLOSE);
    }

    /* Set up a SIGPIPE handler */
    signal(SIGPIPE,sigpipe_handle);
    
    /* Create our context*/
    meth=SSLv3_method();
    ctx=SSL_CTX_new(meth);

    /* Load our keys and certificates*/
    if(!(SSL_CTX_use_certificate_file(ctx,keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Couldn't read certificate file");

    pass=password;
    SSL_CTX_set_default_passwd_cb(ctx,password_cb);
    if(!(SSL_CTX_use_PrivateKey_file(ctx,keyfile,SSL_FILETYPE_PEM)))
      berr_exit("Couldn't read key file");

    /* Load the CAs we trust*/
    if(!(SSL_CTX_load_verify_locations(ctx,CA_LIST,0)))
      berr_exit("Couldn't read CA list");
    SSL_CTX_set_verify_depth(ctx,1);

    /* Load randomness */
    if(!(RAND_load_file(RANDOM,1024*1024)))
      berr_exit("Couldn't load randomness");
       
    return ctx;
  }
      
void destroy_ctx(ctx)
  SSL_CTX *ctx;
  {
    SSL_CTX_free(ctx);
  }


int tcp_listen()
  {
    int sock;
    struct sockaddr_in sin;
    int val=1;
    
    if((sock=socket(AF_INET,SOCK_STREAM,0))<0)
      err_exit("Couldn't make socket");
    
    memset(&sin,0,sizeof(sin));
    sin.sin_addr.s_addr=INADDR_ANY;
    sin.sin_family=AF_INET;
    sin.sin_port=htons(PORT);
    setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&val,sizeof(val));
    
    if(bind(sock,(struct sockaddr *)&sin,sizeof(sin))<0)
      berr_exit("Couldn't bind");
    listen(sock,5);  

    return(sock);
  }

void load_dh_params(ctx,file)
  SSL_CTX *ctx;
  char *file;
  {
    DH *ret=0;
    BIO *bio;

    if ((bio=BIO_new_file(file,"r")) == NULL)
      berr_exit("Couldn't open DH file");

    ret=PEM_read_bio_DHparams(bio,NULL,NULL,NULL);
    BIO_free(bio);
    if(SSL_CTX_set_tmp_dh(ctx,ret)<0)
      berr_exit("Couldn't set DH parameters");
  }

void generate_eph_rsa_key(ctx)
  SSL_CTX *ctx;
  {
    RSA *rsa;

    rsa=RSA_generate_key(512,RSA_F4,NULL,NULL);
    
    if (!SSL_CTX_set_tmp_rsa(ctx,rsa))
      berr_exit("Couldn't set RSA key");

    RSA_free(rsa);
  }
    
  

static int s_server_session_id_context = 1;
         
int main_server(argc,argv)
  int argc;
  char **argv;
  {
    int sock,s;
    BIO *sbio;
    SSL_CTX *ctx;
    SSL *ssl;
    int r;
    
    /* Build our SSL context*/
    ctx=initialize_ctx(KEYFILE,PASSWORD);
    load_dh_params(ctx,DHFILE);
    generate_eph_rsa_key(ctx);
    
    SSL_CTX_set_session_id_context(ctx,(void*)&s_server_session_id_context,
      sizeof s_server_session_id_context);
    
    sock=tcp_listen();

    while(1){
      if((s=accept(sock,0,0))<0)
        err_exit("Problem accepting");
      
      sbio=BIO_new_socket(s,BIO_NOCLOSE);
      ssl=SSL_new(ctx);
      SSL_set_bio(ssl,sbio,sbio);

      if((r=SSL_accept(ssl)<=0))
        berr_exit("SSL accept error");

      echo(ssl,s);
    }
    destroy_ctx(ctx);
    exit(0);
  }

/* A simple SSL client.

   It connects and then forwards data from/to the terminal
   to/from the server
*/
int main_client(argc,argv)
  int argc;
  char **argv;
  {
    SSL_CTX *ctx;
    SSL *ssl;
    BIO *sbio;
    int sock;

    /* Build our SSL context*/
    ctx=initialize_ctx(KEYFILE,PASSWORD);

    /* Connect the TCP socket*/
    sock=tcp_connect();

    /* Connect the SSL socket */
    ssl=SSL_new(ctx);
    sbio=BIO_new_socket(sock,BIO_NOCLOSE);
    SSL_set_bio(ssl,sbio,sbio);
    if(SSL_connect(ssl)<=0)
      berr_exit("SSL connect error");
    check_cert_chain(ssl,HOST);

    /* read and write */
    read_write(ssl,sock);

    destroy_ctx(ctx);
  }

