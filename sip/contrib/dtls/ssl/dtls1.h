/* ssl/dtls1.h */
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

#ifndef HEADER_DTLS1_H 
#define HEADER_DTLS1_H 

#include <openssl/buffer.h>
#include <openssl/pqueue.h>

#ifdef  __cplusplus
extern "C" {
#endif

#define DTLS1_ALLOW_EXPERIMENTAL_CIPHERSUITES	1

#define DTLS1_VERSION			0x0100
#define DTLS1_VERSION_MAJOR		0x01
#define DTLS1_VERSION_MINOR		0x00

#define DTLS1_AD_DECRYPTION_FAILED	21
#define DTLS1_AD_RECORD_OVERFLOW		22
#define DTLS1_AD_UNKNOWN_CA		48	/* fatal */
#define DTLS1_AD_ACCESS_DENIED		49	/* fatal */
#define DTLS1_AD_DECODE_ERROR		50	/* fatal */
#define DTLS1_AD_DECRYPT_ERROR		51
#define DTLS1_AD_EXPORT_RESTRICTION	60	/* fatal */
#define DTLS1_AD_PROTOCOL_VERSION	70	/* fatal */
#define DTLS1_AD_INSUFFICIENT_SECURITY	71	/* fatal */
#define DTLS1_AD_INTERNAL_ERROR		80	/* fatal */
#define DTLS1_AD_USER_CANCELLED		90
#define DTLS1_AD_NO_RENEGOTIATION	100
#define DTLS1_AD_MISSING_HANDSHAKE_MESSAGE    110

/* lengths of messages */
#define DTLS1_COOKIE_LENGTH                     32

#define DTLS1_RT_HEADER_LENGTH                  13

#define DTLS1_HM_HEADER_LENGTH                  12

#define DTLS1_HM_BAD_FRAGMENT                   -2
#define DTLS1_HM_FRAGMENT_RETRY                 -3

#define DTLS1_CCS_HEADER_LENGTH                  3

#define DTLS1_AL_HEADER_LENGTH                   7


typedef struct dtls1_bitmap_st
	{
	unsigned long long map;
	unsigned long length;     /* sizeof the bitmap in bits */
	unsigned long long max_seq_num;  /* max record number seen so far */
	} DTLS1_BITMAP;

struct hm_header_st
	{
	unsigned char type;
	unsigned long msg_len;
	unsigned short seq;
	unsigned long frag_off;
	unsigned long frag_len;
	};

struct ccs_header_st
	{
	unsigned char type;
	unsigned short seq;
	};

struct dtls1_timeout_st
	{
	/* Number of read timeouts so far */
	unsigned int read_timeouts;
	
	/* Number of write timeouts so far */
	unsigned int write_timeouts;
	
	/* Number of alerts received so far */
	unsigned int num_alerts;
	};

typedef struct record_pqueue_st
    {
    unsigned short epoch;
    pqueue q;
    } record_pqueue;

typedef struct hm_fragment_st
    {
    struct hm_header_st msg_header;
    unsigned char *fragment;
    } hm_fragment;

typedef struct dtls1_state_st
	{
	unsigned int send_cookie;
	unsigned char cookie[DTLS1_COOKIE_LENGTH];
	unsigned char rcvd_cookie[DTLS1_COOKIE_LENGTH];
	unsigned int cookie_len;

	/* 
	 * The current data and handshake epoch.  This is initially
	 * undefined, and starts at zero once the initial handshake is
	 * completed 
	 */
	unsigned short r_epoch;
	unsigned short w_epoch;

	/* records being received in the current epoch */
	DTLS1_BITMAP bitmap;

	/* renegotiation starts a new set of sequence numbers */
	DTLS1_BITMAP next_bitmap;

	/* handshake message numbers */
	unsigned short handshake_write_seq;
	unsigned short next_handshake_write_seq;

	unsigned short handshake_read_seq;

    /* only matters for handshake messages */  
	unsigned long long next_expected_seq_num; 

	/* Received handshake records (processed and unprocessed) */
	record_pqueue unprocessed_rcds;
    record_pqueue processed_rcds;

    /* Buffered handshake messages */
    pqueue buffered_messages;

	/* Buffered (sent) handshake records */
	pqueue sent_messages;

	unsigned int mtu; /* max wire packet size */

	struct hm_header_st w_msg_hdr;
	struct hm_header_st r_msg_hdr;

	struct dtls1_timeout_st timeout;
	
	/* storage for Alert/Handshake protocol data received but not
	 * yet processed by ssl3_read_bytes: */
	unsigned char alert_fragment[DTLS1_AL_HEADER_LENGTH];
	unsigned int alert_fragment_len;
	unsigned char handshake_fragment[DTLS1_HM_HEADER_LENGTH];
	unsigned int handshake_fragment_len;

	unsigned int retransmitting;
/*
	unsigned long message_length;
	unsigned long frag_off;
*/

	} DTLS1_STATE;

typedef struct dtls1_record_data_st
	{
	unsigned char *packet;
	unsigned int   packet_length;
	SSL3_BUFFER    rbuf;
	SSL3_RECORD    rrec;
	} DTLS1_RECORD_DATA;

typedef struct dtls1_message_buffer_st
	{
	unsigned char *data;
	unsigned char type;
	unsigned int len;
	unsigned short seq_num;
	unsigned int ccs;  /* set when the message is CCS */
	} dtls1_message_buffer;

/* client methods */
int dtls1_client_hello(SSL *s);
int dtls1_send_client_certificate(SSL *s);
int dtls1_send_client_key_exchange(SSL *s);
int dtls1_send_client_verify(SSL *s);


/* server methods */
int dtls1_send_hello_request(SSL *s);
int dtls1_send_server_hello(SSL *s);
int dtls1_send_server_certificate(SSL *s);
int dtls1_send_server_key_exchange(SSL *s);
int dtls1_send_certificate_request(SSL *s);
int dtls1_send_server_done(SSL *s);

/* common methods */
int dtls1_send_change_cipher_spec(SSL *s, int a, int b);
int dtls1_send_finished(SSL *s, int a, int b, const char *sender, int slen);
unsigned long dtls1_output_cert_chain(SSL *s, X509 *x);
int dtls1_read_failed(SSL *s, int code);
int dtls1_buffer_message(SSL *s, int ccs);
int dtls1_retransmit_message(SSL *s, unsigned short seq, 
	unsigned long frag_off, int *found);
void dtls1_clear_record_buffer(SSL *s);
void dtls1_get_message_header(unsigned char *data, struct hm_header_st *msg_hdr);
void dtls1_get_ccs_header(unsigned char *data, struct ccs_header_st *ccs_hdr);
void dtls1_reset_seq_numbers(SSL *s, int rw);

/* Additional TLS ciphersuites from draft-ietf-tls-56-bit-ciphersuites-00.txt
 * (available if DTLS1_ALLOW_EXPERIMENTAL_CIPHERSUITES is defined, see
 * s3_lib.c).  We actually treat them like SSL 3.0 ciphers, which we probably
 * shouldn't. */
#define DTLS1_CK_RSA_EXPORT1024_WITH_RC4_56_MD5		0x03000060
#define DTLS1_CK_RSA_EXPORT1024_WITH_RC2_CBC_56_MD5	0x03000061
#define DTLS1_CK_RSA_EXPORT1024_WITH_DES_CBC_SHA		0x03000062
#define DTLS1_CK_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA	0x03000063
#define DTLS1_CK_RSA_EXPORT1024_WITH_RC4_56_SHA		0x03000064
#define DTLS1_CK_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA	0x03000065
#define DTLS1_CK_DHE_DSS_WITH_RC4_128_SHA		0x03000066

/* AES ciphersuites from RFC3268 */

#define DTLS1_CK_RSA_WITH_AES_128_SHA			0x0300002F
#define DTLS1_CK_DH_DSS_WITH_AES_128_SHA			0x03000030
#define DTLS1_CK_DH_RSA_WITH_AES_128_SHA			0x03000031
#define DTLS1_CK_DHE_DSS_WITH_AES_128_SHA		0x03000032
#define DTLS1_CK_DHE_RSA_WITH_AES_128_SHA		0x03000033
#define DTLS1_CK_ADH_WITH_AES_128_SHA			0x03000034

#define DTLS1_CK_RSA_WITH_AES_256_SHA			0x03000035
#define DTLS1_CK_DH_DSS_WITH_AES_256_SHA			0x03000036
#define DTLS1_CK_DH_RSA_WITH_AES_256_SHA			0x03000037
#define DTLS1_CK_DHE_DSS_WITH_AES_256_SHA		0x03000038
#define DTLS1_CK_DHE_RSA_WITH_AES_256_SHA		0x03000039
#define DTLS1_CK_ADH_WITH_AES_256_SHA			0x0300003A

/* XXX
 * Inconsistency alert:
 * The OpenSSL names of ciphers with ephemeral DH here include the string
 * "DHE", while elsewhere it has always been "EDH".
 * (The alias for the list of all such ciphers also is "EDH".)
 * The specifications speak of "EDH"; maybe we should allow both forms
 * for everything. */
#define DTLS1_TXT_RSA_EXPORT1024_WITH_RC4_56_MD5		"EXP1024-RC4-MD5"
#define DTLS1_TXT_RSA_EXPORT1024_WITH_RC2_CBC_56_MD5	"EXP1024-RC2-CBC-MD5"
#define DTLS1_TXT_RSA_EXPORT1024_WITH_DES_CBC_SHA	"EXP1024-DES-CBC-SHA"
#define DTLS1_TXT_DHE_DSS_EXPORT1024_WITH_DES_CBC_SHA	"EXP1024-DHE-DSS-DES-CBC-SHA"
#define DTLS1_TXT_RSA_EXPORT1024_WITH_RC4_56_SHA		"EXP1024-RC4-SHA"
#define DTLS1_TXT_DHE_DSS_EXPORT1024_WITH_RC4_56_SHA	"EXP1024-DHE-DSS-RC4-SHA"
#define DTLS1_TXT_DHE_DSS_WITH_RC4_128_SHA		"DHE-DSS-RC4-SHA"

/* AES ciphersuites from RFC3268 */
#define DTLS1_TXT_RSA_WITH_AES_128_SHA			"AES128-SHA"
#define DTLS1_TXT_DH_DSS_WITH_AES_128_SHA		"DH-DSS-AES128-SHA"
#define DTLS1_TXT_DH_RSA_WITH_AES_128_SHA		"DH-RSA-AES128-SHA"
#define DTLS1_TXT_DHE_DSS_WITH_AES_128_SHA		"DHE-DSS-AES128-SHA"
#define DTLS1_TXT_DHE_RSA_WITH_AES_128_SHA		"DHE-RSA-AES128-SHA"
#define DTLS1_TXT_ADH_WITH_AES_128_SHA			"ADH-AES128-SHA"

#define DTLS1_TXT_RSA_WITH_AES_256_SHA			"AES256-SHA"
#define DTLS1_TXT_DH_DSS_WITH_AES_256_SHA		"DH-DSS-AES256-SHA"
#define DTLS1_TXT_DH_RSA_WITH_AES_256_SHA		"DH-RSA-AES256-SHA"
#define DTLS1_TXT_DHE_DSS_WITH_AES_256_SHA		"DHE-DSS-AES256-SHA"
#define DTLS1_TXT_DHE_RSA_WITH_AES_256_SHA		"DHE-RSA-AES256-SHA"
#define DTLS1_TXT_ADH_WITH_AES_256_SHA			"ADH-AES256-SHA"


#define DTLS_CT_RSA_SIGN			1
#define DTLS_CT_DSS_SIGN			2
#define DTLS_CT_RSA_FIXED_DH		3
#define DTLS_CT_DSS_FIXED_DH		4
#define DTLS_CT_NUMBER			4

#define DTLS1_FINISH_MAC_LENGTH		12

#define DTLS_MD_MAX_CONST_SIZE			20
#define DTLS_MD_CLIENT_FINISH_CONST		"client finished"
#define DTLS_MD_CLIENT_FINISH_CONST_SIZE		15
#define DTLS_MD_SERVER_FINISH_CONST		"server finished"
#define DTLS_MD_SERVER_FINISH_CONST_SIZE		15
#define DTLS_MD_SERVER_WRITE_KEY_CONST		"server write key"
#define DTLS_MD_SERVER_WRITE_KEY_CONST_SIZE	16
#define DTLS_MD_KEY_EXPANSION_CONST		"key expansion"
#define DTLS_MD_KEY_EXPANSION_CONST_SIZE		13
#define DTLS_MD_CLIENT_WRITE_KEY_CONST		"client write key"
#define DTLS_MD_CLIENT_WRITE_KEY_CONST_SIZE	16
#define DTLS_MD_SERVER_WRITE_KEY_CONST		"server write key"
#define DTLS_MD_SERVER_WRITE_KEY_CONST_SIZE	16
#define DTLS_MD_IV_BLOCK_CONST			"IV block"
#define DTLS_MD_IV_BLOCK_CONST_SIZE		8
#define DTLS_MD_MASTER_SECRET_CONST		"master secret"
#define DTLS_MD_MASTER_SECRET_CONST_SIZE		13

/* Timeout multipliers (timeout slice is defined in apps/timeouts.h */
#define DTLS1_TMO_READ_COUNT                      2
#define DTLS1_TMO_WRITE_COUNT                     2

#define DTLS1_TMO_ALERT_COUNT                     12

#ifdef CHARSET_EBCDIC
#undef DTLS_MD_CLIENT_FINISH_CONST
#define DTLS_MD_CLIENT_FINISH_CONST    "\x63\x6c\x69\x65\x6e\x74\x20\x66\x69\x6e\x69\x73\x68\x65\x64"  /*client finished*/
#undef DTLS_MD_SERVER_FINISH_CONST
#define DTLS_MD_SERVER_FINISH_CONST    "\x73\x65\x72\x76\x65\x72\x20\x66\x69\x6e\x69\x73\x68\x65\x64"  /*server finished*/
#undef DTLS_MD_SERVER_WRITE_KEY_CONST
#define DTLS_MD_SERVER_WRITE_KEY_CONST "\x73\x65\x72\x76\x65\x72\x20\x77\x72\x69\x74\x65\x20\x6b\x65\x79"  /*server write key*/
#undef DTLS_MD_KEY_EXPANSION_CONST
#define DTLS_MD_KEY_EXPANSION_CONST    "\x6b\x65\x79\x20\x65\x78\x70\x61\x6e\x73\x69\x6f\x6e"  /*key expansion*/
#undef DTLS_MD_CLIENT_WRITE_KEY_CONST
#define DTLS_MD_CLIENT_WRITE_KEY_CONST "\x63\x6c\x69\x65\x6e\x74\x20\x77\x72\x69\x74\x65\x20\x6b\x65\x79"  /*client write key*/
#undef DTLS_MD_SERVER_WRITE_KEY_CONST
#define DTLS_MD_SERVER_WRITE_KEY_CONST "\x73\x65\x72\x76\x65\x72\x20\x77\x72\x69\x74\x65\x20\x6b\x65\x79"  /*server write key*/
#undef DTLS_MD_IV_BLOCK_CONST
#define DTLS_MD_IV_BLOCK_CONST         "\x49\x56\x20\x62\x6c\x6f\x63\x6b"  /*IV block*/
#undef DTLS_MD_MASTER_SECRET_CONST
#define DTLS_MD_MASTER_SECRET_CONST    "\x6d\x61\x73\x74\x65\x72\x20\x73\x65\x63\x72\x65\x74"  /*master secret*/
#endif

#ifdef  __cplusplus
}
#endif
#endif

