/* 
This header file contains functions that will return error message associated with error number/code from following functions :

-> SSL_ERROR_SYSCALL  [http://www.cl.cam.ac.uk/cgi-bin/manpage?3+errno]    
-> SSL_get_error [https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html]
-> X509_verify_cert_error_string(verifyErrorCode) [https://www.openssl.org/docs/manmaster/crypto/X509_STORE_CTX_get_error.html]

*/

/*  Q. Why strerror(int errno) function is not used ? (http://www.cplusplus.com/reference/cstring/strerror/)
    A. If error code is 22, strerror(int errno) will print "Invalid argument". But this is not so convenient, instead of using this function,
       I have created function 'errortostringXX(int errno)' which will print error description in the format "EINVAL (Invalid argument)" 
       which is more easy to read and understand.
 */

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

#pragma GCC diagnostic ignored "-Wwrite-strings"

#ifndef ERRDES_HXX
#define ERRDES_HXX

#include "Errdes.hxx"
#include <cstdio>
#include <cstring>

#define MAX_ERROR_LEN 256
static char buf[MAX_ERROR_LEN]; 

/*           Below are the error description from SSL_ERROR_SYSCALL or strerror(int errno) [http://www.cl.cam.ac.uk/cgi-bin/manpage?3+errno]            */

static char* messagesOS[] = {
/*0               0   */  "No error 0",
/*EPERM           1   */  "EPERM (Operation not permitted) 1",
/*ENOENT          2   */  "ENOENT (No such file or directory) 2",
/*ESRCH           3   */  "ESRCH (No such process) 3",
/*EINTR           4   */  "EINTR (Interrupted system call) 4",
/*EIO             5   */  "EIO (Input/output error) 5",
/*ENXIO           6   */  "ENXIO (No such device or address) 6",
/*E2BIG           7   */  "E2BIG (Argument list too long) 7",
/*ENOEXEC         8   */  "ENOEXEC (Exec format error) 8",
/*EBADF           9   */  "EBADF (Bad file descriptor) 9",
/*ECHILD          10  */  "ECHILD (No child processes) 10",
/*EAGAIN          11  */  "EAGAIN (Resource temporarily unavailable) 11",
/*ENOMEM          12  */  "ENOMEM (Cannot allocate memory) 12",
/*EACCES          13  */  "EACCES (Permission denied) 13",
/*EFAULT          14  */  "EFAULT (Bad address) 14",
/*ENOTBLK         15  */  "ENOTBLK (Block device required) 15", 
/*EBUSY           16  */  "EBUSY (Device or resource busy) 16",
/*EEXIST          17  */  "EEXIST (File exists) 17",
/*EXDEV           18  */  "EXDEV (Invalid cross-device link) 18",
/*ENODEV          19  */  "ENODEV (No such device) 19",
/*ENOTDIR         20  */  "ENOTDIR (Not a directory) 20",
/*EISDIR          21  */  "EISDIR (Is a directory) 21",
/*EINVAL          22  */  "EINVAL (Invalid argument) 22",
/*ENFILE          23  */  "ENFILE (Too many open files in system) 23",
/*EMFILE          24  */  "EMFILE (Too many open files) 24",
/*ENOTTY          25  */  "ENOTTY (Inappropriate ioctl for device) 25",
/*ETXTBSY         26  */  "ETXTBSY (Text file busy) 26",
/*EFBIG           27  */  "EFBIG (File too large) 27",
/*ENOSPC          28  */  "ENOSPC (No space left on device) 28",
/*ESPIPE          29  */  "ESPIPE (Illegal seek) 29",
/*EROFS           30  */  "EROFS (Read-only file system) 30",
/*EMLINK          31  */  "EMLINK (Too many links) 31",
/*EPIPE           32  */  "EPIPE (Broken pipe) 32",
/*EDOM            33  */  "EDOM (Numerical argument out of domain) 33",
/*ERANGE          34  */  "ERANGE (Numerical result out of range) 34",
/*EDEADLK         35  */  "EDEADLK (Resource deadlock avoided) 35",
/*ENAMETOOLONG    36  */  "ENAMETOOLONG (File name too long) 36",
/*ENOLCK          37  */  "ENOLCK (No locks available) 37",
/*ENOSYS          38  */  "ENOSYS (Function not implemented) 38",
/*ENOTEMPTY       39  */  "ENOTEMPTY (Directory not empty) 39",
/*ELOOP           40  */  "ELOOP (Too many levels of symbolic links) 40",
/*EWOULDBLOCK     41  */  "EWOULDBLOCK (Resource temporarily unavailable) 41",
/*ENOMSG          42  */  "ENOMSG (No message of desired type) 42",
/*EIDRM           43  */  "EIDRM (Identifier removed) 43",
/*ECHRNG          44  */  "ECHRNG (Channel number out of range) 44",
/*EL2NSYNC        45  */  "EL2NSYNC (Level 2 not synchronized) 45",
/*EL3HLT          46  */  "EL3HLT (Level 3 halted) 46",
/*EL3RST          47  */  "EL3RST (Level 3 reset) 47",
/*ELNRNG          48  */  "ELNRNG (Link number out of range) 48",
/*EUNATCH         49  */  "EUNATCH (Protocol driver not attached) 49",
/*ENOCSI          50  */  "ENOCSI (No CSI structure available) 50",
/*EL2HLT          51  */  "EL2HLT (Level 2 halted) 51",
/*EBADE           52  */  "EBADE (Invalid exchange) 52",
/*EBADR           53  */  "EBADR (Invalid request descriptor) 53",
/*EXFULL          54  */  "EXFULL (Exchange full) 54",
/*ENOANO          55  */  "ENOANO (No anode) 55",
/*EBADRQC         56  */  "EBADRQC (Invalid request code) 56",
/*EBADSLT         57  */  "EBADSLT (Invalid slot) 57",
/*EDEADLOCK       58  */  "EDEADLOCK (Resource deadlock avoided) 58",
/*EBFONT          59  */  "EBFONT (Bad font file format) 59",
/*ENOSTR          60  */  "ENOSTR (Device not a stream) 60",
/*ENODATA         61  */  "ENODATA (No data available) 61",
/*ETIME           62  */  "ETIME (Timer expired) 62",
/*ENOSR           63  */  "ENOSR (Out of streams resources) 63",
/*ENONET          64  */  "ENONET (Machine is not on the network) 64",
/*ENOPKG          65  */  "ENOPKG (Package not installed) 65",
/*EREMOTE         66  */  "EREMOTE (Object is remote) 66",
/*ENOLINK         67  */  "ENOLINK (Link has been severed) 67",
/*EADV            68  */  "EADV (Advertise error) 68",
/*ESRMNT          69  */  "ESRMNT (Srmount error) 69",
/*ECOMM           70  */  "ECOMM (Communication error on send) 70",
/*EPROTO          71  */  "EPROTO (Protocol error) 71",
/*EMULTIHOP       72  */  "EMULTIHOP (Multihop attempted) 72",
/*EDOTDOT         73  */  "EDOTDOT (RFS specific error) 73",
/*EBADMSG         74  */  "EBADMSG (Bad message) 74",
/*EOVERFLOW       75  */  "EOVERFLOW (Value too large for defined data type) 75",
/*ENOTUNIQ        76  */  "ENOTUNIQ (Name not unique on network) 76",
/*EBADFD          77  */  "EBADFD (File descriptor in bad state) 77",
/*EREMCHG         78  */  "EREMCHG (Remote address changed) 78",
/*ELIBACC         79  */  "ELIBACC (Can not access a needed shared library) 79",
/*ELIBBAD         80  */  "ELIBBAD (Accessing a corrupted shared library) 80",
/*ELIBSCN         81  */  "ELIBSCN (.lib section in a.out corrupted) 81",
/*ELIBMAX         82  */  "ELIBMAX (Attempting to link in too many shared libraries) 82",
/*ELIBEXEC        83  */  "ELIBEXEC (Cannot exec a shared library directly) 83",
/*EILSEQ          84  */  "EILSEQ (Invalid or incomplete multibyte or wide character) 84",
/*ERESTART        85  */  "ERESTART (Interrupted system call should be restarted) 85",
/*ESTRPIPE        86  */  "ESTRPIPE (Streams pipe error) 86",
/*EUSERS          87  */  "EUSERS (Too many users) 87",
/*ENOTSOCK        88  */  "ENOTSOCK (Socket operation on non-socket) 88",
/*EDESTADDRREQ    89  */  "EDESTADDRREQ (Destination address required) 89",
/*EMSGSIZE        90  */  "EMSGSIZE (Message too long) 90",
/*EPROTOTYPE      91  */  "EPROTOTYPE (Protocol wrong type for socket) 91",
/*ENOPROTOOPT     92  */  "ENOPROTOOPT (Protocol not available) 92",
/*EPROTONOSUPPORT 93  */  "EPROTONOSUPPORT (Protocol not supported) 93",
/*ESOCKTNOSUPPORT 94  */  "ESOCKTNOSUPPORT (Socket type not supported) 94",
/*EOPNOTSUPP      95  */  "EOPNOTSUPP (Operation not supported) 95",
/*EPFNOSUPPORT    96  */  "EPFNOSUPPORT (Protocol family not supported) 96",
/*EAFNOSUPPORT    97  */  "EAFNOSUPPORT (Address family not supported by protocol) 97",
/*EADDRINUSE      98  */  "EADDRINUSE (Address already in use) 98",
/*EADDRNOTAVAIL   99  */  "EADDRNOTAVAIL (Cannot assign requested address) 99",
/*ENETDOWN        100 */  "ENETDOWN (Network is down) 100",
/*ENETUNREACH     101 */  "ENETUNREACH (Network is unreachable) 101",
/*ENETRESET       102 */  "ENETRESET (Network dropped connection on reset) 102",
/*ECONNABORTED    103 */  "ECONNABORTED (Software caused connection abort) 103",
/*ECONNRESET      104 */  "ECONNRESET (Connection reset by peer) 104",
/*ENOBUFS         105 */  "ENOBUFS (No buffer space available) 105",
/*EISCONN         106 */  "EISCONN (Transport endpoint is already connected) 106",
/*ENOTCONN        107 */  "ENOTCONN (Transport endpoint is not connected) 107",
/*ESHUTDOWN       108 */  "ESHUTDOWN (Cannot send after transport endpoint shutdown) 108",
/*ETOOMANYREFS    109 */  "ETOOMANYREFS (Too many references: cannot splice) 109",
/*ETIMEDOUT       110 */  "ETIMEDOUT (Connection timed out) 110",
/*ECONNREFUSED    111 */  "ECONNREFUSED (Connection refused) 111",
/*EHOSTDOWN       112 */  "EHOSTDOWN (Host is down) 112",
/*EHOSTUNREACH    113 */  "EHOSTUNREACH (No route to host) 113",
/*EALREADY        114 */  "EALREADY (Operation already in progress) 114",
/*EINPROGRESS     115 */  "EINPROGRESS (Operation now in progress) 115",
/*ESTALE          116 */  "ESTALE (Stale file handle) 116",
/*EUCLEAN         117 */  "EUCLEAN (Structure needs cleaning) 117",
/*ENOTNAM         118 */  "ENOTNAM (Not a XENIX named type file) 118",
/*ENAVAIL         119 */  "ENAVAIL (No XENIX semaphores available) 119",
/*EISNAM          120 */  "EISNAM (Is a named type file) 120",
/*EREMOTEIO       121 */  "EREMOTEIO (Remote I/O error) 121",
/*EDQUOT          122 */  "EDQUOT (Disk quota exceeded) 122",
/*ENOMEDIUM       123 */  "ENOMEDIUM (No medium found) 123",
/*EMEDIUMTYPE     124 */  "EMEDIUMTYPE (Wrong medium type) 124",
/*ECANCELED       125 */  "ECANCELED (Operation canceled) 125",
/*ENOKEY          126 */  "ENOKEY (Required key not available) 126",
/*EKEYEXPIRED     127 */  "EKEYEXPIRED (Key has expired) 127",
/*EKEYREVOKED     128 */  "EKEYREVOKED (Key has been revoked) 128",
/*EKEYREJECTED    129 */  "EKEYREJECTED (Key was rejected by service) 129",
/*EOWNERDEAD      130 */  "EOWNERDEAD (Owner died) 130",
/*ENOTRECOVERABLE 131 */  "ENOTRECOVERABLE (State not recoverable) 131",
/*ERFKILL         132 */  "ERFKILL (Operation not possible due to RF-kill) 132",
/*EHWPOISON       133 */  "EHWPOISON (Memory page has hardware error) 133",
};
static const int NUM_MESSAGES_OS = sizeof(messagesOS)/sizeof(messagesOS[0]);

static inline char* errortostringOS(int errnumOS)
{
   if( (errnumOS >= 0 )  &&  (errnumOS < NUM_MESSAGES_OS))
   {
      strncpy(buf,messagesOS[errnumOS],MAX_ERROR_LEN-1);
      buf[MAX_ERROR_LEN - 1] = '\0';          /* Ensure null termination */
   }
   else
   {
      snprintf(buf, MAX_ERROR_LEN, "Unknown error %d", errnumOS);
   }
   return buf;
}

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from SSL_get_error [https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html]             */

static char* messagesSSL[] = {
/*  0  */  "SSL_ERROR_NONE 0",
/*  1  */  "SSL_ERROR_SSL 1",
/*  2  */  "SSL_ERROR_WANT_READ 2",
/*  3  */  "SSL_ERROR_WANT_WRITE 3",
/*  4  */  "SSL_ERROR_WANT_X509_LOOKUP 4",
/*  5  */  "SSL_ERROR_SYSCALL 5",
/*  6  */  "SSL_ERROR_ZERO_RETURN 6",
/*  7  */  "SSL_ERROR_WANT_CONNECT 7",
/*  8  */  "SSL_ERROR_WANT_ACCEPT 8",
};
static const int NUM_MESSAGES_SSL = sizeof(messagesSSL)/sizeof(messagesSSL[0]);

static inline char* errortostringSSL(int errnumSSL)
{
   if( (errnumSSL>= 0 )  &&  (errnumSSL < NUM_MESSAGES_SSL))
   {
      strncpy(buf,messagesSSL[errnumSSL],MAX_ERROR_LEN-1);
      buf[MAX_ERROR_LEN - 1] = '\0';          /* Ensure null termination */
   }
   else
   {
      snprintf(buf, MAX_ERROR_LEN, "Unknown error %d", errnumSSL);
   }
   return buf;
}

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from X509_verify_cert_error_string(verifyErrorCode) [https://www.openssl.org/docs/manmaster/crypto/X509_STORE_CTX_get_error.html]             */

static char* messagesX509[] = {
/*  0  */  "X509_V_OK 0",
/*  1  */  "illegal error (for uninitialized values, to avoid X509_V_OK) 1",
/*  2  */  "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT 2",
/*  3  */  "X509_V_ERR_UNABLE_TO_GET_CRL 3",
/*  4  */  "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE 4",
/*  5  */  "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE 5",
/*  6  */  "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY 6",
/*  7  */  "X509_V_ERR_CERT_SIGNATURE_FAILURE 7",
/*  8  */  "X509_V_ERR_CRL_SIGNATURE_FAILURE 8",
/*  9  */  "X509_V_ERR_CERT_NOT_YET_VALID 9",
/*  10 */  "X509_V_ERR_CERT_HAS_EXPIRED 10",
/*  11 */  "X509_V_ERR_CRL_NOT_YET_VALID 11",
/*  12 */  "X509_V_ERR_CRL_HAS_EXPIRED 12",
/*  13 */  "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD 13",
/*  14 */  "X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD 14",
/*  15 */  "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD 15", 
/*  16 */  "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD 16",
/*  17 */  "X509_V_ERR_OUT_OF_MEM 17",
/*  18 */  "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT 18",
/*  19 */  "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN 19",
/*  20 */  "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY 20",
/*  21 */  "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE 21",
/*  22 */  "X509_V_ERR_CERT_CHAIN_TOO_LONG 22",
/*  23 */  "X509_V_ERR_CERT_REVOKED 23",
/*  24 */  "X509_V_ERR_INVALID_CA 24",
/*  25 */  "X509_V_ERR_PATH_LENGTH_EXCEEDED 25",
/*  26 */  "X509_V_ERR_INVALID_PURPOSE 26",
/*  27 */  "X509_V_ERR_CERT_UNTRUSTED 27",
/*  28 */  "X509_V_ERR_CERT_REJECTED 28",
/*  29 */  "X509_V_ERR_SUBJECT_ISSUER_MISMATCH 29",
/*  30 */  "X509_V_ERR_AKID_SKID_MISMATCH 30",
/*  31 */  "X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH 31",
/*  32 */  "X509_V_ERR_KEYUSAGE_NO_CERTSIGN 32",
/*  33 */  "X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER 33",
/*  34 */  "X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION 34",
/*  35 */  "X509_V_ERR_KEYUSAGE_NO_CRL_SIGN 35",
/*  36 */  "X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION 36",
/*  37 */  "X509_V_ERR_INVALID_NON_CA 37",
/*  38 */  "X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED 38",
/*  39 */  "X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE 39",
/*  40 */  "X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED 40",
/*  41 */  "X509_V_ERR_INVALID_EXTENSION 41",
/*  42 */  "X509_V_ERR_INVALID_POLICY_EXTENSION 42",
/*  43 */  "X509_V_ERR_NO_EXPLICIT_POLICY 43",
/*  44 */  "X509_V_ERR_DIFFERENT_CRL_SCOPE 44",
/*  45 */  "X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE 45",
/*  46 */  "X509_V_ERR_UNNESTED_RESOURCE 46",
/*  47 */  "X509_V_ERR_PERMITTED_VIOLATION 47",
/*  48 */  "X509_V_ERR_EXCLUDED_VIOLATION 48",
/*  49 */  "X509_V_ERR_SUBTREE_MINMAX 49",
/*  50 */  "X509_V_ERR_APPLICATION_VERIFICATION 50",
/*  51 */  "X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE 51",
/*  52 */  "X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX 52",
/*  53 */  "X509_V_ERR_UNSUPPORTED_NAME_SYNTAX 53",
/*  54 */  "X509_V_ERR_CRL_PATH_VALIDATION_ERROR 54",
};
static const int NUM_MESSAGES_X509 = sizeof(messagesX509)/sizeof(messagesX509[0]);

static inline char* errortostringX509(int errnumX509)
{
   if( (errnumX509>= 0 )  &&  (errnumX509 < NUM_MESSAGES_X509))
   {
      strncpy(buf,messagesX509[errnumX509],MAX_ERROR_LEN-1);
      buf[MAX_ERROR_LEN - 1] = '\0';          /* Ensure null termination */
   }
   else
   {
      snprintf(buf, MAX_ERROR_LEN, "Unknown error %d", errnumX509);
   }
   return buf;
}

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

#endif

/* ====================================================================
 *
 * Copyright 2013 Daniel Pocock http://danielpocock.com  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */
