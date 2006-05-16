#ifndef INCLUDED_DTLS_SHIM
#define INCLUDED_DTLS_SHIM

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <openssl/x509.h>

/* read and write timeouts. */
#define DTLS_SHIM_DEFAULT_RECV_TIMEOUT  250000  /* 250 ms */
#define DTLS_SHIM_DEFAULT_SEND_TIMEOUT  250000  /* 250 ms */

typedef  struct {
    struct sockaddr remote;
} dtls_shim_con_info_s;

typedef struct {
    unsigned char key_material[48];
} dtls_shim_srtp_key_s;

typedef struct {
    unsigned char *fingerprint;
} dtls_shim_fingerprint_s;

typedef enum {
    DTLS_SHIM_OK = 1,          /* no action required. */
    DTLS_SHIM_READ_ERROR = 2,
    DTLS_SHIM_WRITE_ERROR = 3,
    DTLS_SHIM_WANT_READ = 4,   /* _read() needs to be called next. */
	DTLS_SHIM_WANT_WRITE = 5  /* _write() needs to be called next. */
} dtls_shim_iostatus_e;

typedef enum {
   DTLS_SHIM_FINGERPRINT_MATCH    = 1,
   DTLS_SHIM_FINGERPRINT_MISMATCH = 2,
   DTLS_SHIM_FINGERPRINT_CHECK_PENDING = 3
} dtls_shim_fingerprint_status_e;


typedef struct dtls_shim *dtls_shim_h;


/* 
 * Initialize the library; set-up data structures.
 * RETURNS: NULL handle if an error occurs. 
 */
dtls_shim_h dtls_shim_init(const X509 *cert, void *);

/* 
 * Clean-up and release allocated memory.  This function is *not*
 * responsible for cleaning closing DTLS connections. 
 */
void dtls_shim_fini(dtls_shim_h);


/*
 * Return client data.
 */
void *dtls_shim_get_client_data(dtls_shim_h);


/* 
 * Get SRTP keys for a given connection.
 * RETURNS: NULL on error.
 */
dtls_shim_srtp_key_s *dtls_get_srtp_key(dtls_shim_h, dtls_shim_con_info_s);

/* 
 * To be invoked after each read or write call.  If, and when, a timer
 * set to the returned timeout expires, dtls_shim_write() should be
 * invoked.  If read is non zero, then the read timeout is returned,
 * otherwise write timeout is returned.  
 * RETURNS: a non-zero timeout value.  
 */
int dtls_shim_get_timeout(dtls_shim_h, dtls_shim_con_info_s, int read);


/*
 * Decrypts and verifies DTLS records.
 * RETURNS: the number of bytes written to obuf.  If return value is <= 0,
 * then call dtls_shim_get_status() for further information.
 */
int dtls_shim_read(dtls_shim_h, dtls_shim_con_info_s, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen,
	dtls_shim_iostatus_e *status);

/*
 * Encrypts and MACs DTLS records.
 * RETURNS: the number of bytes written to obuf.  If the return value is <= 0,
 * then call dtls_shim_get_status() for further information.
 */
int dtls_shim_write(dtls_shim_h, dtls_shim_con_info_s, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen, 
	dtls_shim_iostatus_e *status);

/*
 * This function requests that the connection be closed.
 * dtls_shim_write() should be called after this.
 */
void dtls_shim_close(dtls_shim_h, dtls_shim_con_info_s);

dtls_shim_fingerprint_s *dtls_shim_add_fingerprint(dtls_shim_h, 
    dtls_shim_fingerprint_s *);

dtls_shim_fingerprint_status_e dtls_shim_fingerprint_match(dtls_shim_h, 
	dtls_shim_con_info_s);

/*
 * Returns an array of connection info structures associated with this
 * session.  
 */
dtls_shim_con_info_s *dtls_shim_get_con_info(dtls_shim_h, 
	unsigned int *count);

#endif /* ! INCLUDED_DTLS_SHIM */
