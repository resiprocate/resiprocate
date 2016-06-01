/* 
This header file contains error message associated with error number/code from following functions :
-> SSL_ERROR_SYSCALL  [http://www.cl.cam.ac.uk/cgi-bin/manpage?3+errno]    
-> SSL_get_error [https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html]
-> X509_verify_cert_error_string(verifyErrorCode) [https://www.openssl.org/docs/manmaster/crypto/X509_STORE_CTX_get_error.html]

At present this header file only contains error messages that are called from TlsConnection.cxx file.
*/

/*  Q. Why strerror(int errno) is not used ? (http://www.cplusplus.com/reference/cstring/strerror/)
    A. If error code is 22, strerror(int errno) will print "Invalid argument". But this is not so convenient, instead of using this function,
       I have created function 'errfunc_x(int errno)' which will print error description in the format "EINVAL (Invalid argument)" 
       which is more easy to read and understand.
 */

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from SSL_ERROR_SYSCALL [http://www.cl.cam.ac.uk/cgi-bin/manpage?3+errno]            */

static char* messages_1[] = {
/*0               0   */  "No error",
/*EPERM           1   */  "EPERM (Operation not permitted)",
/*ENOENT          2   */  "ENOENT (No such file or directory)",
/*ESRCH           3   */  "ESRCH (No such process)",
/*EINTR           4   */  "EINTR (Interrupted system call)",
/*EIO             5   */  "EIO (Input/output error)",
/*ENXIO           6   */  "ENXIO (No such device or address)",
/*E2BIG           7   */  "E2BIG (Argument list too long)",
/*ENOEXEC         8   */  "ENOEXEC (Exec format error)",
/*EBADF           9   */  "EBADF (Bad file descriptor)",
/*ECHILD          10  */  "ECHILD (No child processes)",
/*EAGAIN          11  */  "EAGAIN (Resource temporarily unavailable)",
/*ENOMEM          12  */  "ENOMEM (Cannot allocate memory)",
/*EACCES          13  */  "EACCES (Permission denied)",
/*EFAULT          14  */  "EFAULT (Bad address)",
/*ENOTBLK         15  */  "ENOTBLK (Block device required)", 
/*EBUSY           16  */  "EBUSY (Device or resource busy)",
/*EEXIST          17  */  "EEXIST (File exists)",
/*EXDEV           18  */  "EXDEV (Invalid cross-device link)",
/*ENODEV          19  */  "ENODEV (No such device)",
/*ENOTDIR         20  */  "ENOTDIR (Not a directory)",
/*EISDIR          21  */  "EISDIR (Is a directory)",
/*EINVAL          22  */  "EINVAL (Invalid argument)",
/*ENFILE          23  */  "ENFILE (Too many open files in system)",
/*EMFILE          24  */  "EMFILE (Too many open files)",
/*ENOTTY          25  */  "ENOTTY (Inappropriate ioctl for device)",
/*ETXTBSY         26  */  "ETXTBSY (Text file busy)",
/*EFBIG           27  */  "EFBIG (File too large)",
/*ENOSPC          28  */  "ENOSPC (No space left on device)",
/*ESPIPE          29  */  "ESPIPE (Illegal seek)",
/*EROFS           30  */  "EROFS (Read-only file system)",
/*EMLINK          31  */  "EMLINK (Too many links)",
/*EPIPE           32  */  "EPIPE (Broken pipe)",
/*EDOM            33  */  "EDOM (Numerical argument out of domain)",
/*ERANGE          34  */  "ERANGE (Numerical result out of range)",
/*EDEADLK         35  */  "EDEADLK (Resource deadlock avoided)",
/*ENAMETOOLONG    36  */  "ENAMETOOLONG (File name too long)",
/*ENOLCK          37  */  "ENOLCK (No locks available)",
/*ENOSYS          38  */  "ENOSYS (Function not implemented)",
/*ENOTEMPTY       39  */  "ENOTEMPTY (Directory not empty)",
/*ELOOP           40  */  "ELOOP (Too many levels of symbolic links)",
/*EWOULDBLOCK     11  */  "EWOULDBLOCK (Resource temporarily unavailable)",
/*ENOMSG          42  */  "ENOMSG (No message of desired type)",
/*EIDRM           43  */  "EIDRM (Identifier removed)",
/*ECHRNG          44  */  "ECHRNG (Channel number out of range)",
/*EL2NSYNC        45  */  "EL2NSYNC (Level 2 not synchronized)",
/*EL3HLT          46  */  "EL3HLT (Level 3 halted)",
/*EL3RST          47  */  "EL3RST (Level 3 reset)",
/*ELNRNG          48  */  "ELNRNG (Link number out of range)",
/*EUNATCH         49  */  "EUNATCH (Protocol driver not attached)",
/*ENOCSI          50  */  "ENOCSI (No CSI structure available)",
/*EL2HLT          51  */  "EL2HLT (Level 2 halted)",
/*EBADE           52  */  "EBADE (Invalid exchange)",
/*EBADR           53  */  "EBADR (Invalid request descriptor)",
/*EXFULL          54  */  "EXFULL (Exchange full)",
/*ENOANO          55  */  "ENOANO (No anode)",
/*EBADRQC         56  */  "EBADRQC (Invalid request code)",
/*EBADSLT         57  */  "EBADSLT (Invalid slot)",
/*EDEADLOCK       35  */  "EDEADLOCK (Resource deadlock avoided)",
/*EBFONT          59  */  "EBFONT (Bad font file format)",
/*ENOSTR          60  */  "ENOSTR (Device not a stream)",
/*ENODATA         61  */  "ENODATA (No data available)",
/*ETIME           62  */  "ETIME (Timer expired)",
/*ENOSR           63  */  "ENOSR (Out of streams resources)",
/*ENONET          64  */  "ENONET (Machine is not on the network)",
/*ENOPKG          65  */  "ENOPKG (Package not installed)",
/*EREMOTE         66  */  "EREMOTE (Object is remote)",
/*ENOLINK         67  */  "ENOLINK (Link has been severed)",
/*EADV            68  */  "EADV (Advertise error)",
/*ESRMNT          69  */  "ESRMNT (Srmount error)",
/*ECOMM           70  */  "ECOMM (Communication error on send)",
/*EPROTO          71  */  "EPROTO (Protocol error)",
/*EMULTIHOP       72  */  "EMULTIHOP (Multihop attempted)",
/*EDOTDOT         73  */  "EDOTDOT (RFS specific error)",
/*EBADMSG         74  */  "EBADMSG (Bad message)",
/*EOVERFLOW       75  */  "EOVERFLOW (Value too large for defined data type)",
/*ENOTUNIQ        76  */  "ENOTUNIQ (Name not unique on network)",
/*EBADFD          77  */  "EBADFD (File descriptor in bad state)",
/*EREMCHG         78  */  "EREMCHG (Remote address changed)",
/*ELIBACC         79  */  "ELIBACC (Can not access a needed shared library)",
/*ELIBBAD         80  */  "ELIBBAD (Accessing a corrupted shared library)",
/*ELIBSCN         81  */  "ELIBSCN (.lib section in a.out corrupted)",
/*ELIBMAX         82  */  "ELIBMAX (Attempting to link in too many shared libraries)",
/*ELIBEXEC        83  */  "ELIBEXEC (Cannot exec a shared library directly)",
/*EILSEQ          84  */  "EILSEQ (Invalid or incomplete multibyte or wide character)",
/*ERESTART        85  */  "ERESTART (Interrupted system call should be restarted)",
/*ESTRPIPE        86  */  "ESTRPIPE (Streams pipe error)",
/*EUSERS          87  */  "EUSERS (Too many users)",
/*ENOTSOCK        88  */  "ENOTSOCK (Socket operation on non-socket)",
/*EDESTADDRREQ    89  */  "EDESTADDRREQ (Destination address required)",
/*EMSGSIZE        90  */  "EMSGSIZE (Message too long)",
/*EPROTOTYPE      91  */  "EPROTOTYPE (Protocol wrong type for socket)",
/*ENOPROTOOPT     92  */  "ENOPROTOOPT (Protocol not available)",
/*EPROTONOSUPPORT 93  */  "EPROTONOSUPPORT (Protocol not supported)",
/*ESOCKTNOSUPPORT 94  */  "ESOCKTNOSUPPORT (Socket type not supported)",
/*EOPNOTSUPP      95  */  "EOPNOTSUPP (Operation not supported)",
/*EPFNOSUPPORT    96  */  "EPFNOSUPPORT (Protocol family not supported)",
/*EAFNOSUPPORT    97  */  "EAFNOSUPPORT (Address family not supported by protocol)",
/*EADDRINUSE      98  */  "EADDRINUSE (Address already in use)",
/*EADDRNOTAVAIL   99  */  "EADDRNOTAVAIL (Cannot assign requested address)",
/*ENETDOWN        100 */  "ENETDOWN (Network is down)",
/*ENETUNREACH     101 */  "ENETUNREACH (Network is unreachable)",
/*ENETRESET       102 */  "ENETRESET (Network dropped connection on reset)",
/*ECONNABORTED    103 */  "ECONNABORTED (Software caused connection abort)",
/*ECONNRESET      104 */  "ECONNRESET (Connection reset by peer)",
/*ENOBUFS         105 */  "ENOBUFS (No buffer space available)",
/*EISCONN         106 */  "EISCONN (Transport endpoint is already connected)",
/*ENOTCONN        107 */  "ENOTCONN (Transport endpoint is not connected)",
/*ESHUTDOWN       108 */  "ESHUTDOWN (Cannot send after transport endpoint shutdown)",
/*ETOOMANYREFS    109 */  "ETOOMANYREFS (Too many references: cannot splice)",
/*ETIMEDOUT       110 */  "ETIMEDOUT (Connection timed out)",
/*ECONNREFUSED    111 */  "ECONNREFUSED (Connection refused)",
/*EHOSTDOWN       112 */  "EHOSTDOWN (Host is down)",
/*EHOSTUNREACH    113 */  "EHOSTUNREACH (No route to host)",
/*EALREADY        114 */  "EALREADY (Operation already in progress)",
/*EINPROGRESS     115 */  "EINPROGRESS (Operation now in progress)",
/*ESTALE          116 */  "ESTALE (Stale file handle)",
/*EUCLEAN         117 */  "EUCLEAN (Structure needs cleaning)",
/*ENOTNAM         118 */  "ENOTNAM (Not a XENIX named type file)",
/*ENAVAIL         119 */  "ENAVAIL (No XENIX semaphores available)",
/*EISNAM          120 */  "EISNAM (Is a named type file)",
/*EREMOTEIO       121 */  "EREMOTEIO (Remote I/O error)",
/*EDQUOT          122 */  "EDQUOT (Disk quota exceeded)",
/*ENOMEDIUM       123 */  "ENOMEDIUM (No medium found)",
/*EMEDIUMTYPE     124 */  "EMEDIUMTYPE (Wrong medium type)",
/*ECANCELED       125 */  "ECANCELED (Operation canceled)",
/*ENOKEY          126 */  "ENOKEY (Required key not available)",
/*EKEYEXPIRED     127 */  "EKEYEXPIRED (Key has expired)",
/*EKEYREVOKED     128 */  "EKEYREVOKED (Key has been revoked)",
/*EKEYREJECTED    129 */  "EKEYREJECTED (Key was rejected by service)",
/*EOWNERDEAD      130 */  "EOWNERDEAD (Owner died)",
/*ENOTRECOVERABLE 131 */  "ENOTRECOVERABLE (State not recoverable)",
/*ERFKILL         132 */  "ERFKILL (Operation not possible due to RF-kill)",
/*EHWPOISON       133 */  "EHWPOISON (Memory page has hardware error)",
};
static const int NUM_MESSAGES_1 = sizeof(messages_1)/sizeof(messages_1[0]);

extern "C" char* errfun_1(int errnum_1)
{
   if (errnum_1 < NUM_MESSAGES_1)
      return messages_1[errnum_1];
   return "Unknown error";
}

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from SSL_get_error [https://www.openssl.org/docs/manmaster/ssl/SSL_get_error.html]             */

static char* messages_2[] = {
/*  0  */  "SSL_ERROR_NONE",
/*  1  */  "SSL_ERROR_SSL",
/*  2  */  "SSL_ERROR_WANT_READ",
/*  3  */  "SSL_ERROR_WANT_WRITE",
/*  4  */  "SSL_ERROR_WANT_X509_LOOKUP",
/*  5  */  "SSL_ERROR_SYSCALL",
/*  6  */  "SSL_ERROR_ZERO_RETURN",
/*  7  */  "SSL_ERROR_WANT_CONNECT",
/*  8  */  "SSL_ERROR_WANT_ACCEPT",
};
static const int NUM_MESSAGES_2 = sizeof(messages_2)/sizeof(messages_2[0]);

extern "C" char* errfun_2(int errnum_2)
{
   if (errnum_2 < NUM_MESSAGES_2)
      return messages_2[errnum_2];
   return "Unknown error";
}

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/

/*           Below are the error description from X509_verify_cert_error_string(verifyErrorCode) [https://www.openssl.org/docs/manmaster/crypto/X509_STORE_CTX_get_error.html]             */

static char* messages_3[] = {
/*  0  */  "X509_V_OK",
/*  1  */  "illegal error (for uninitialized values, to avoid X509_V_OK)",
/*  2  */  "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT",
/*  3  */  "X509_V_ERR_UNABLE_TO_GET_CRL",
/*  4  */  "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE",
/*  5  */  "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE",
/*  6  */  "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY",
/*  7  */  "X509_V_ERR_CERT_SIGNATURE_FAILURE",
/*  8  */  "X509_V_ERR_CRL_SIGNATURE_FAILURE",
/*  9  */  "X509_V_ERR_CERT_NOT_YET_VALID",
/*  10 */  "X509_V_ERR_CERT_HAS_EXPIRED",
/*  11 */  "X509_V_ERR_CRL_NOT_YET_VALID",
/*  12 */  "X509_V_ERR_CRL_HAS_EXPIRED",
/*  13 */  "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD",
/*  14 */  "X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD",
/*  15 */  "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD", 
/*  16 */  "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD",
/*  17 */  "X509_V_ERR_OUT_OF_MEM",
/*  18 */  "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT",
/*  19 */  "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN",
/*  20 */  "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY",
/*  21 */  "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE",
/*  22 */  "X509_V_ERR_CERT_CHAIN_TOO_LONG",
/*  23 */  "X509_V_ERR_CERT_REVOKED",
/*  24 */  "X509_V_ERR_INVALID_CA",
/*  25 */  "X509_V_ERR_PATH_LENGTH_EXCEEDED",
/*  26 */  "X509_V_ERR_INVALID_PURPOSE",
/*  27 */  "X509_V_ERR_CERT_UNTRUSTED",
/*  28 */  "X509_V_ERR_CERT_REJECTED",
/*  29 */  "X509_V_ERR_SUBJECT_ISSUER_MISMATCH",
/*  30 */  "X509_V_ERR_AKID_SKID_MISMATCH",
/*  31 */  "X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH",
/*  32 */  "X509_V_ERR_KEYUSAGE_NO_CERTSIGN",
/*  33 */  "X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER",
/*  34 */  "X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION",
/*  35 */  "X509_V_ERR_KEYUSAGE_NO_CRL_SIGN",
/*  36 */  "X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION",
/*  37 */  "X509_V_ERR_INVALID_NON_CA",
/*  38 */  "X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED",
/*  39 */  "X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE",
/*  40 */  "X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED",
/*  41 */  "X509_V_ERR_INVALID_EXTENSION",
/*  42 */  "X509_V_ERR_INVALID_POLICY_EXTENSION",
/*  43 */  "X509_V_ERR_NO_EXPLICIT_POLICY",
/*  44 */  "X509_V_ERR_DIFFERENT_CRL_SCOPE",
/*  45 */  "X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE",
/*  46 */  "X509_V_ERR_UNNESTED_RESOURCE",
/*  47 */  "X509_V_ERR_PERMITTED_VIOLATION",
/*  48 */  "X509_V_ERR_EXCLUDED_VIOLATION",
/*  49 */  "X509_V_ERR_SUBTREE_MINMAX",
/*  50 */  "X509_V_ERR_APPLICATION_VERIFICATION",
/*  51 */  "X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE",
/*  52 */  "X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX",
/*  53 */  "X509_V_ERR_UNSUPPORTED_NAME_SYNTAX",
/*  54 */  "X509_V_ERR_CRL_PATH_VALIDATION_ERROR",
};
static const int NUM_MESSAGES_3 = sizeof(messages_3)/sizeof(messages_3[0]);

extern "C" char* errfun_3(int errnum_3)
{
   if (errnum_3 < NUM_MESSAGES_3)
      return messages_3[errnum_3];
   return "Unknown error";
}

/*----------------END-------------------END-------------------END-------------------END---------------------END-------------------END----------*/
