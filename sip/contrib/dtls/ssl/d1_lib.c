/* ssl/t1_lib.c */
/* Copyright (C) 1995-1998 Eric Young (eay@cryptsoft.com)
 * All rights reserved.
 *
 * This package is an SSL implementation written
 * by Eric Young (eay@cryptsoft.com).
 * The implementation was written so as to conform with Netscapes SSL.
 * 
 * This library is free for commercial and non-commercial use as long as
 * the following conditions are aheared to.  The following conditions
 * apply to all code found in this distribution, be it the RC4, RSA,
 * lhash, DES, etc., code; not just the SSL code.  The SSL documentation
 * included with this distribution is covered by the same copyright terms
 * except that the holder is Tim Hudson (tjh@cryptsoft.com).
 * 
 * Copyright remains Eric Young's, and as such any Copyright notices in
 * the code are not to be removed.
 * If this package is used in a product, Eric Young should be given attribution
 * as the author of the parts of the library used.
 * This can be in the form of a textual message at program startup or
 * in documentation (online or textual) provided with the package.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *    "This product includes cryptographic software written by
 *     Eric Young (eay@cryptsoft.com)"
 *    The word 'cryptographic' can be left out if the rouines from the library
 *    being used are not cryptographic related :-).
 * 4. If you include any Windows specific code (or a derivative thereof) from 
 *    the apps directory (application code) you must include an acknowledgement:
 *    "This product includes software written by Tim Hudson (tjh@cryptsoft.com)"
 * 
 * THIS SOFTWARE IS PROVIDED BY ERIC YOUNG ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 * 
 * The licence and distribution terms for any publically available version or
 * derivative of this code cannot be changed.  i.e. this code cannot simply be
 * copied and put under another distribution licence
 * [including the GNU Public Licence.]
 */

#include <stdio.h>
#include <openssl/objects.h>
#include "ssl_locl.h"

const char *dtls1_version_str="DTLSv1" OPENSSL_VERSION_PTEXT;

static long dtls1_default_timeout(void);

static SSL3_ENC_METHOD DTLSv1_enc_data={
    dtls1_enc,
	tls1_mac,
	tls1_setup_key_block,
	tls1_generate_master_secret,
	tls1_change_cipher_state,
	tls1_final_finish_mac,
	TLS1_FINISH_MAC_LENGTH,
	tls1_cert_verify_mac,
	TLS_MD_CLIENT_FINISH_CONST,DTLS_MD_CLIENT_FINISH_CONST_SIZE,
	TLS_MD_SERVER_FINISH_CONST,DTLS_MD_SERVER_FINISH_CONST_SIZE,
	tls1_alert_code,
	};

static SSL_METHOD DTLSv1_data= {
	DTLS1_VERSION,
	dtls1_new,
	dtls1_clear,
	dtls1_free,
	ssl_undefined_function,
	ssl_undefined_function,
	ssl3_read,
	ssl3_peek,
	ssl3_write,
	ssl3_shutdown,
	ssl3_renegotiate,
	ssl3_renegotiate_check,
	dtls1_get_message,
	dtls1_read_bytes,
	dtls1_write_app_data_bytes,
	dtls1_dispatch_alert,
	ssl3_ctrl,
	ssl3_ctx_ctrl,
	ssl3_get_cipher_by_char,
	ssl3_put_cipher_by_char,
	ssl3_pending,
	ssl3_num_ciphers,
	ssl3_get_cipher,
	ssl_bad_method,
	dtls1_default_timeout,
	&DTLSv1_enc_data,
	ssl_undefined_function,
	ssl3_callback_ctrl,
	ssl3_ctx_callback_ctrl,
	};

static long dtls1_default_timeout(void)
	{
	/* 2 hours, the 24 hours mentioned in the DTLSv1 spec
	 * is way too long for http, the cache would over fill */
	return(60*60*2);
	}

SSL_METHOD *dtlsv1_base_method(void)
	{
	return(&DTLSv1_data);
	}

int dtls1_new(SSL *s)
	{
	DTLS1_STATE *d1;

	if (!ssl3_new(s)) return(0);
	if ((d1=OPENSSL_malloc(sizeof *d1)) == NULL) return (0);
	memset(d1,0, sizeof *d1);

	/* d1->handshake_epoch=0; */
	d1->bitmap.length=sizeof(d1->bitmap.map) * 8;
	d1->unprocessed_rcds.q=pqueue_new();
    d1->processed_rcds.q=pqueue_new();
    d1->buffered_messages = pqueue_new();
	d1->sent_messages=pqueue_new();

	if ( s->server)
		{
		d1->cookie_len = sizeof(s->d1->cookie);
		}

	if( ! d1->unprocessed_rcds.q || ! d1->processed_rcds.q 
        || ! d1->buffered_messages || ! d1->sent_messages)
		{
        if ( d1->unprocessed_rcds.q) pqueue_free(d1->unprocessed_rcds.q);
        if ( d1->processed_rcds.q) pqueue_free(d1->processed_rcds.q);
        if ( d1->buffered_messages) pqueue_free(d1->buffered_messages);
		if ( d1->sent_messages) pqueue_free(d1->sent_messages);
		OPENSSL_free(d1);
		return (0);
		}

	s->d1=d1;
	s->method->ssl_clear(s);
	return(1);
	}

void dtls1_free(SSL *s)
	{
    pitem *item = NULL;
    hm_fragment *frag = NULL;

	ssl3_free(s);

    while( (item = pqueue_pop(s->d1->unprocessed_rcds.q)) != NULL)
        {
        OPENSSL_free(item->data);
        pitem_free(item);
        }
    pqueue_free(s->d1->unprocessed_rcds.q);

    while( (item = pqueue_pop(s->d1->processed_rcds.q)) != NULL)
        {
        OPENSSL_free(item->data);
        pitem_free(item);
        }
    pqueue_free(s->d1->processed_rcds.q);

    while( (item = pqueue_pop(s->d1->buffered_messages)) != NULL)
        {
        frag = (hm_fragment *)item->data;
        OPENSSL_free(frag->fragment);
        OPENSSL_free(frag);
        pitem_free(item);
        }
    pqueue_free(s->d1->buffered_messages);

    while ( (item = pqueue_pop(s->d1->sent_messages)) != NULL)
        {
        frag = (hm_fragment *)item->data;
        OPENSSL_free(frag->fragment);
        OPENSSL_free(frag);
        pitem_free(item);
        }
	pqueue_free(s->d1->sent_messages);

	OPENSSL_free(s->d1);
	}

void dtls1_clear(SSL *s)
	{
	ssl3_clear(s);
	s->version=DTLS1_VERSION;
	}

#if 0
long dtls1_ctrl(SSL *s, int cmd, long larg, char *parg)
	{
	return(0);
	}

long dtls1_callback_ctrl(SSL *s, int cmd, void *(*fp)())
	{
	return(0);
	}
#endif



