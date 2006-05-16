
#include <openssl/ssl.h>
#include <time.h>

#include "dtls_shim.h"


#define DTLS_SHIM_DEFAULT_NUM_FINGERPRINTS 5
#define DTLS_SHIM_DEFAULT_NUM_CONNECTIONS  5

typedef struct dtls_shim_table {
    int count;
    int max;
    dtls_shim_con_info_s *connections;
    SSL **ssl;
} dtls_shim_table_s;

typedef struct dtls_shim {
    SSL_CTX *ctx;
    dtls_shim_table_s *table;
    unsigned int num_fingerprints;
    unsigned int max_fingerprints;
    dtls_shim_fingerprint_s **fingerprints;
    void *app_data;
} dtls_shim_s;


dtls_shim_table_s *dtls_shim_table_new();
void              dtls_shim_table_free(dtls_shim_table_s *);
SSL               *dtls_shim_table_find(dtls_shim_table_s *, dtls_shim_con_info_s *);
dtls_shim_con_info_s *dtls_shim_table_get_entries(dtls_shim_table_s *, unsigned int *);


dtls_shim_table_s *
dtls_shim_table_new()
{
    dtls_shim_table_s *table = NULL;

    table = (dtls_shim_table_s *)malloc(sizeof(dtls_shim_table_s));
    if ( table == NULL)
        return NULL;

    memset(table, 0x00, sizeof(dtls_shim_table_s));

    table->count = 0;
    table->max = DTLS_SHIM_DEFAULT_NUM_CONNECTIONS;
    
    table->connections = (dtls_shim_con_info_s *)malloc(sizeof(dtls_shim_con_info_s) *
        DTLS_SHIM_DEFAULT_NUM_CONNECTIONS);

    table->ssl = (SSL **)malloc(sizeof(SSL *) * DTLS_SHIM_DEFAULT_NUM_CONNECTIONS);

    if ( table->connections == NULL || table->ssl == NULL)
    {
        if ( table->connections != NULL) free(table->connections);
        if ( table->ssl != NULL) free(table->ssl);
        free(table);
        return NULL;
    }

    memset(table->connections, 0x00, sizeof(dtls_shim_con_info_s) * 
        DTLS_SHIM_DEFAULT_NUM_CONNECTIONS);

    memset(table->ssl, 0x00, sizeof(SSL *) * DTLS_SHIM_DEFAULT_NUM_CONNECTIONS);

    return table;
}


void
dtls_shim_table_free(dtls_shim_table_s *table)
{
    int i;

    if ( table == NULL)
        return;

    for ( i = 0; i < table->count; i++)
        SSL_free(table->ssl[i]);

    free(table->connections);
    free(table->ssl);
    free(table);
}


SSL *
dtls_shim_find(dtls_shim_table_s *table, dtls_shim_con_info_s *con)
{
    int i;

    for ( i = 0; i < table->count; i++)
    {
        if ( memcmp(&(table->connections[i]), con, 
            sizeof(dtls_shim_con_info_s)) == 0)
            return table->ssl[i];
    }

    return NULL;
}

dtls_shim_con_info_s *
dtls_shim_table_get_entries(dtls_shim_table_s *table, unsigned int *count)
{
    *count = table->count;
    return table->connections;
}


static BIO *g_dummy_bio = NULL;

struct dtls_shim *
dtls_shim_init(const X509 *cert, void *app_data)
{
    dtls_shim_s *handle = NULL;

    handle = (dtls_shim_s *)malloc(sizeof(dtls_shim_s));
    if ( handle == NULL)
        return NULL;

    memset(handle, 0x00, sizeof(dtls_shim_s));

    handle->ctx = SSL_CTX_new(DTLSv1_method());
    handle->table = dtls_shim_table_new();
    handle->app_data = app_data;

    handle->fingerprints = (dtls_shim_fingerprint_s **)malloc(sizeof(dtls_shim_fingerprint_s *) * 
        DTLS_SHIM_DEFAULT_NUM_FINGERPRINTS);
    
    handle->num_fingerprints = 0;
    handle->max_fingerprints = DTLS_SHIM_DEFAULT_NUM_FINGERPRINTS;

    g_dummy_bio = BIO_new(BIO_s_mem());

    if ( handle->ctx == NULL || handle->table == NULL || 
        handle->fingerprints == NULL || g_dummy_bio == NULL)
    {
        if ( handle->ctx != NULL) SSL_CTX_free(handle->ctx);
        if ( handle->table != NULL) dtls_shim_table_free(handle->table);
        if ( handle->fingerprints != NULL) free(handle->fingerprints);
        if ( g_dummy_bio != NULL) BIO_free(g_dummy_bio);
        free(handle);
        handle = NULL;
    }

    return handle;    
}


void
dtls_shim_fini(dtls_shim_s *handle)
{
    int i;

    if ( handle != NULL)
    {
        SSL_CTX_free(handle->ctx);
        dtls_shim_table_free(handle->table);
        for ( i = 0; i < handle->num_fingerprints; i++)
            free(handle->fingerprints[i]);
        free(handle->fingerprints);
        free(handle);
    }

    if ( g_dummy_bio != NULL)
    {
        BIO_free(g_dummy_bio);
        g_dummy_bio = NULL;
    }
}


void *
dtls_shim_get_client_data(dtls_shim_s *handle)
{
    if ( handle != NULL)
        return handle->app_data;
    else
        return NULL;
}


dtls_shim_srtp_key_s *
dtls_shim_get_srtp_key(dtls_shim_s *handle, dtls_shim_con_info_s *con)
{
    dtls_shim_srtp_key_s *srtp_key;
    SSL *ssl = NULL;

    if ( handle == NULL || con == NULL)
        return NULL;

    ssl = dtls_shim_table_find(handle->table, con);
    if (ssl == NULL)
        return NULL;
    if ( SSL_is_init_finished(ssl) && SSL_srtp_negotiated())
    {
        srtp_key = (dtls_shim_srtp_key_s *)
            malloc(sizeof(dtls_shim_srtp_key_s));
        if (srtp_key == NULL)
            return NULL;
        else
        {
            SSL_get_srtp_key(ssl, srtp_key->key_material);
            return srtp_key;
        }
    }

    return NULL;
}


int
dtls_shim_get_timeout(dtls_shim_s *handle, dtls_shim_con_info_s con, int read)
{
    struct timeval timeout;
    SSL *ssl = NULL;
    BIO *bio = NULL;

    if ( handle == NULL)
        return -1;

    ssl = dtls_shim_table_find(handle->table, &con);
    if ( ssl == NULL)
        return -1;
    
    if ( read) 
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = DTLS_SHIM_DEFAULT_RECV_TIMEOUT;
        bio = SSL_get_rbio(ssl);
        return BIO_ctrl(bio, BIO_CTRL_DGRAM_GET_RECV_TIMEOUT, 0, &timeout);
    }
    else 
    {
        timeout.tv_sec = 0;
        timeout.tv_usec = DTLS_SHIM_DEFAULT_SEND_TIMEOUT;
        bio = SSL_get_wbio(ssl);
        return BIO_ctrl(bio, BIO_CTRL_DGRAM_GET_SEND_TIMEOUT, 0, &timeout);
    }
}


int 
dtls_shim_read(dtls_shim_s *handle, dtls_shim_con_info_s con, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen,
	dtls_shim_iostatus_e *status)
{
    int len, err;
    BIO *rbio = NULL;
    SSL *ssl = NULL;

    if ( handle == NULL)
        return -1;

    ssl = dtls_shim_table_find(handle->table, &con);
    if ( ssl == NULL)
    {
        ssl = SSL_new(handle->ctx);
        if ( ssl == NULL)
            return -1;

        /* This connection is the server side of DTLS. */
        SSL_set_accept_state(ssl);

        /* I/O to be set-up later. */
        SSL_set_bio(ssl, NULL, g_dummy_bio);

        dtls_shim_table_add(handle->table, con, ssl);
    }

    rbio = BIO_new_mem_buf((char *)ibuf, ilen);
    if ( rbio == NULL)
        return -1;

    BIO_set_mem_eof_return(rbio, -1);

    ssl->rbio = rbio;

    len = SSL_read(ssl, obuf, olen);
    err = SSL_get_error(ssl, len);
    
    BIO_free(ssl->rbio);
    ssl->rbio = g_dummy_bio;

    *status = DTLS_SHIM_OK;

    if ( len <= 0)
    {
        switch(err)
        {
            case SSL_ERROR_WANT_READ:
                *status = DTLS_SHIM_WANT_READ;
            case SSL_ERROR_WANT_WRITE:
                *status = DTLS_SHIM_WANT_WRITE;
            default:
                *status = DTLS_SHIM_READ_ERROR;
        }
    }

    return len;
}

int 
dtls_shim_write(dtls_shim_s *handle, dtls_shim_con_info_s con, unsigned char *obuf, 
    unsigned int olen, const unsigned char *ibuf, unsigned int ilen, 
	dtls_shim_iostatus_e *status)
{
    int len, err;
    BIO *wbio = NULL;
    SSL *ssl = NULL;

    if ( handle == NULL)
        return -1;

    ssl = dtls_shim_table_find(handle->table, &con);
    if ( ssl == NULL)
    {
        ssl = SSL_new(handle->ctx);
        if (ssl == NULL)
            return -1;

        SSL_set_connect_state(ssl);

        SSL_set_bio(ssl, g_dummy_bio, g_dummy_bio);
        dtls_shim_table_add(handle->table, con, ssl);
    }

    wbio = BIO_new_mem_buf(obuf, olen);
    if ( wbio == NULL)
        return -1;

    BIO_set_mem_eof_return(wbio, -1);
    
    ssl->wbio = wbio;

    len = SSL_write(ssl, ibuf, ilen);

    BIO_free(ssl->wbio);
    ssl->wbio = g_dummy_bio;

    *status = DTLS_SHIM_OK;

    if ( len <= 0)
    {
        err = SSL_get_error(ssl, len);
        switch(err)
        {
            case SSL_ERROR_WANT_READ:
                *status = DTLS_SHIM_WANT_READ;
                break;
            case SSL_ERROR_WANT_WRITE:
                *status = DTLS_SHIM_WANT_WRITE;
                break;
            default:
                *status = DTLS_SHIM_WRITE_ERROR;
                break;
        }
    }

    return len;
}


void 
dtls_shim_close(dtls_shim_s *handle, dtls_shim_con_info_s con)
{
}

dtls_shim_fingerprint_s *
dtls_shim_add_fingerprint(dtls_shim_s *handle, 
    dtls_shim_fingerprint_s *fingerprint)
{
    if ( handle == NULL || fingerprint == NULL)
        return;

    assert( handle->num_fingerprints <= handle->max_fingerprints);

    if ( handle->num_fingerprints == handle->max_fingerprints)
    {
        handle->fingerprints = (dtls_shim_fingerprint_s **)realloc(handle->fingerprints, 
            sizeof(dtls_shim_fingerprint_s *) * (handle->max_fingerprints +
                DTLS_SHIM_DEFAULT_NUM_FINGERPRINTS));

        if ( handle->fingerprints == NULL)
            return NULL;

        handle->max_fingerprints += DTLS_SHIM_DEFAULT_NUM_FINGERPRINTS;
    }

    handle->fingerprints[handle->num_fingerprints] = fingerprint;
    handle->num_fingerprints++;
    return fingerprint;
}

dtls_shim_fingerprint_status_e dtls_shim_fingerprint_match(dtls_shim_s *handle, 
	dtls_shim_con_info_s con)
{
    dtls_shim_fingerprint_status_e status = DTLS_SHIM_FINGERPRINT_MATCH;
    return status;
}

dtls_shim_con_info_s *dtls_shim_get_con_info(dtls_shim_s *handle, 
	unsigned int *count)
{
    if ( handle == NULL || count == NULL)
        return NULL;

    return dtls_shim_table_get_entries(handle->table, count);
}
