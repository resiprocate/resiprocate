
#ifndef INCLUDED_DTLS_SHIM
#define INCLUDED_DTLS_SHIM

typedef struct {
    sockaddr remote;
} connection_info_s;

typedef struct {
    unsigned char key_material[48];
} srtp_key_s;

typedef struct {
    unsigned char *fingerprint;
} fingerprint_s;

typedef enum {
    DTLS_SHIM_OK = 1;          /* no action required. */
    DTLS_SHIM_WANT_READ = 2;   /* _read() needs to be called next. */
	DTLS_SHIM_WANT_WRITE = 3;  /* _write() needs to be called next. */
} dtls_shim_iostatus_e;


typedef dtls_shim_s *dtls_shim_h;

/* 
 * Initialize the library; set-up data structures.
 * RETURNS: NULL handle if an error occurs. 
 */
dtls_shim_h dtls_shim_init(const X509 *cert);

/* 
 * Clean-up and release allocated memory.  This function is *not*
 * responsible for cleaning closing DTLS connections. 
 */
void dtls_shim_fini(dtls_shim_h);


/* 
 * Get SRTP keys for a given connection.
 * RETURNS: NULL on error. 
 */
srtp_key_s *dtls_get_srtp_key(dtls_shim_h, connection_info_s, 
    srtp_key_s *);

/* 
 * To be invoked after each read or write call.  If, and when, a timer
 * set to the returned timeout expires, dtls_shim_write() should be
 * invoked. 
 * RETURNS: a non-zero timeout value.
 */
int dtls_shim_get_timeout(dtls_shim_h, connection_info_s);


/*
 * Decrypts and verifies DTLS records.
 * RETURNS: the number of bytes written to obuf.  If return value is <= 0,
 * then call dtls_shim_get_status() for further information.
 */
int dtls_shim_read(dtls_shim_h, connection_info_s, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen);

/*
 * Encrypts and MACs DTLS records.
 * RETURNS: the number of bytes written to obuf.  If the return value is <= 0,
 * then call dtls_shim_get_status() for further information.
 */
int dtls_shim_write(dtls_shim_h, connection_info_s, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen, 
	dtls_shim_iostatus_e *status);

/*
 * This function requests that the connection be closed.
 * dtls_shim_write() should be called after this.
 */
void dtls_shim_close(dtls_shim_h, connection_info_s);


#endif /* ! INCLUDED_DTLS_SHIM */
