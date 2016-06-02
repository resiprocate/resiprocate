/* 
This header file contains functions that will return error message associated with error number/code from following functions :
-> SSL_ERROR_SYSCALL  [http://www.cl.cam.ac.uk/cgi-bin/manpage?3+errno]    
-> SSL_get_error [https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html]
-> X509_verify_cert_error_string(verifyErrorCode) [https://www.openssl.org/docs/manmaster/crypto/X509_STORE_CTX_get_error.html]

At present this header file can only be called from stack/ssl files.
*/

/*  Q. Why strerror(int errno) function is not used ? (http://www.cplusplus.com/reference/cstring/strerror/)
    A. If error code is 22, strerror(int errno) will print "Invalid argument". But this is not so convenient, instead of using this function,
       I have created function 'errortostringXX(int errno)' which will print error description in the format "EINVAL (Invalid argument)" 
       which is more easy to read and understand.
 */

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from SSL_ERROR_SYSCALL [http://www.cl.cam.ac.uk/cgi-bin/manpage?3+errno]            */

extern "C" char* errortostringOS(int errnumOS);

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from SSL_get_error [https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html]             */

extern "C" char* errortostringSSL(int errnumSSL);

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from X509_verify_cert_error_string(verifyErrorCode) [https://www.openssl.org/docs/manmaster/crypto/X509_STORE_CTX_get_error.html]             */

extern "C" char* errortostringX509(int errnumX509);

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/
