
#ifndef INCLUDED_DTLS_SHIM
#define INCLUDED_DTLS_SHIM

typedef struct {
    sockaddr remote;
} connection_info_s;

typedef struct {
    unsigned char key_material[48];
} srtp_key_s;

typedef struct {

} fingerprint_s;

typedef dtls_shim_s *dtls_shim_h;

/* 
 * Initialize the library; set-up data structures.
 * RETURNS: NULL handle if an error occurs. 
 */
dtls_shim_h dtls_shim_init();

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
 */
int dtls_shim_read(dtls_shim_h, connection_info_s, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen);

int dtls_shim_write(dtls_shim_h, connection_info_s, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen);

/*
 * This function requests that the connection be closed.
 * dtls_shim_write() should be called after this.
 */
int dtls_shim_close(dtls_shim_h, connection_info_s);

int dtls_shim_add_fingerprint(dtls_shim_h, fingerprint_s);


#endif /* ! INCLUDED_DTLS_SHIM */
