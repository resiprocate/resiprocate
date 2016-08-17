#include <iostream>
#include <map>
#include <string>
#include <errno.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include "rutil/Errdes.hxx"

#ifdef _WIN32
#include <winsock.h>
#include <windows.h>
#endif

using namespace std;

#ifdef _WIN32
map <int, string> WinErrorMsg;
#elif __linux__
map <int, string> ErrornoErrorMsg;
#endif
map <int, string> OpenSSLErrorMsg;
map <int, string> X509ErrorMsg;


string NumericError::SearchErrorMsg(int Error, int ClassCode)
{
    string result;
    switch(ClassCode)
    {
        case OSERROR:
        {
#ifdef _WIN32
                result = WinErrorMsg[Error];
#elif __linux__
                result = ErrornoErrorMsg[Error];
#endif
            break;
        }
        case SSLERROR:
        result = OpenSSLErrorMsg[Error];
        break;
        case X509ERROR:
        result = X509ErrorMsg[Error];
        break;
    }
    if(result.length() == 0)
        result = "Unknown error";
    return result;
};

void ErrnoError::CreateMappingErrorMsg()
{
    
#ifdef _WIN32                             // creating error mapping for windows system errors
{
  WinErrorMsg[WSA_INVALID_HANDLE]         = "WSA_INVALID_HANDLE 6 Specified event object handle is invalid.";
  WinErrorMsg[WSA_NOT_ENOUGH_MEMORY]      = "WSA_NOT_ENOUGH_MEMORY 8 Insufficient memory available.";
  WinErrorMsg[WSA_INVALID_PARAMETER]      = "WSA_INVALID_PARAMETER 87 One or more parameters are invalid.";
  WinErrorMsg[WSA_OPERATION_ABORTED]      = "WSA_OPERATION_ABORTED 995 Overlapped operation aborted.";
  WinErrorMsg[WSA_IO_INCOMPLETE]          = "WSA_IO_INCOMPLETE 996 Overlapped I/O event object not in signaled state.";
  WinErrorMsg[SA_IO_PENDING]              = "SA_IO_PENDING 997 Overlapped operations will complete later.";
  WinErrorMsg[WSAEINTR]                   = "WSAEINTR 10004 Interrupted function call.";
  WinErrorMsg[WSAEBADF]                   = "WSAEBADF 10009 File handle is not valid.";
  WinErrorMsg[WSAEACCES]                  = "WSAEACCES 10013 Permission denied.";
  WinErrorMsg[WSAEFAULT]                  = "WSAEFAULT 10014 Bad address.";
  WinErrorMsg[WSAEINVAL]                  = "WSAEINVAL 10022 Invalid argument.";
  WinErrorMsg[WSAEMFILE]                  = "WSAEMFILE 10024 Too many open files.";
  WinErrorMsg[WSAEWOULDBLOCK]             = "WSAEWOULDBLOCK 10035 Resource temporarily unavailable.";
  WinErrorMsg[WSAEINPROGRESS]             = "WSAEINPROGRESS 10036 Operation now in progress.";
  WinErrorMsg[WSAEALREADY]                = "WSAEALREADY 10037 Operation already in progress.";
  WinErrorMsg[WSAENOTSOCK]                = "WSAENOTSOCK 10038 Socket operation on nonsocket.";
  WinErrorMsg[WSAEDESTADDRREQ]            = "WSAEDESTADDRREQ 10039 Destination address required.";
  WinErrorMsg[WSAEMSGSIZE]                = "WSAEMSGSIZE 10040 Message too long.";
  WinErrorMsg[WSAEPROTOTYPE]              = "WSAEPROTOTYPE 10041 Protocol wrong type for socket.";
  WinErrorMsg[WSAENOPROTOOPT]             = "WSAENOPROTOOPT 10042 Bad protocol option.";
  WinErrorMsg[WSAEPROTONOSUPPORT]         = "WSAEPROTONOSUPPORT 10043 Protocol not supported.";
  WinErrorMsg[WSAESOCKTNOSUPPORT]         = "WSAESOCKTNOSUPPORT 10044 Socket type not supported.";
  WinErrorMsg[WSAEOPNOTSUPP]              = "WSAEOPNOTSUPP 10045 Operation not supported.";
  WinErrorMsg[WSAEPFNOSUPPORT]            = "WSAEPFNOSUPPORT 10046 Protocol family not supported.";
  WinErrorMsg[WSAEAFNOSUPPORT]            = "WSAEAFNOSUPPORT 10047 Address family not supported by protocol family.";
  WinErrorMsg[WSAEADDRINUSE]              = "WSAEADDRINUSE 10048 Address already in use.";
  WinErrorMsg[WSAEADDRNOTAVAIL]           = "WSAEADDRNOTAVAIL 10049 Cannot assign requested address.";
  WinErrorMsg[WSAENETDOWN]                = "WSAENETDOWN 10050 Network is down.";
  WinErrorMsg[WSAENETUNREACH]             = "WSAENETUNREACH 10051 Network is unreachable.";
  WinErrorMsg[WSAENETRESET]               = "WSAENETRESET 10052 Network dropped connection on reset.";
  WinErrorMsg[WSAECONNABORTED]            = "WSAECONNABORTED 10053 Software caused connection abort.";
  WinErrorMsg[WSAECONNRESET]              = "WSAECONNRESET 10054 Connection reset by peer.";
  WinErrorMsg[WSAENOBUFS]                 = "WSAENOBUFS 10055 No buffer space available.";
  WinErrorMsg[WSAEISCONN]                 = "WSAEISCONN 10056 Socket is already connected.";
  WinErrorMsg[WSAENOTCONN]                = "WSAENOTCONN 10057 Socket is not connected.";
  WinErrorMsg[WSAESHUTDOWN]               = "WSAESHUTDOWN 10058 Cannot send after socket shutdown.",;
  WinErrorMsg[WSAETOOMANYREFS]            = "WSAETOOMANYREFS 10059 Too many references.";
  WinErrorMsg[WSAETIMEDOUT]               = "WSAETIMEDOUT 10060 Connection timed out.";
  WinErrorMsg[WSAECONNREFUSED]            = "WSAECONNREFUSED 10061 Connection refused.";
  WinErrorMsg[WSAELOOP]                   = "WSAELOOP 10062 Cannot translate name.";
  WinErrorMsg[WSAENAMETOOLONG]            = "WSAENAMETOOLONG 10063 Name too long.";
  WinErrorMsg[WSAEHOSTDOWN]               = "WSAEHOSTDOWN 10064 Host is down.";
  WinErrorMsg[WSAEHOSTUNREACH]            = "WSAEHOSTUNREACH 10065 No route to host.";
  WinErrorMsg[WSAENOTEMPTY]               = "WSAENOTEMPTY 10066 Directory not empty.";
  WinErrorMsg[WSAEPROCLIM]                = "WSAEPROCLIM 10067 Too many processes.";
  WinErrorMsg[WSAEUSERS]                  = "WSAEUSERS 10068 User quota exceeded.";
  WinErrorMsg[WSAEDQUOT]                  = "WSAEDQUOT 10069 Disk quota exceeded.";
  WinErrorMsg[WSAESTALE]                  = "WSAESTALE 10070 Stale file handle reference.";
  WinErrorMsg[WSAEREMOTE]                 = "WSAEREMOTE 10071 Item is remote.";
  WinErrorMsg[WSASYSNOTREADY]             = "WSASYSNOTREADY 10091 Network subsystem is unavailable.";
  WinErrorMsg[WSAVERNOTSUPPORTED]         = "WSAVERNOTSUPPORTED 10092 Winsock.dll version out of range.";
  WinErrorMsg[WSANOTINITIALISED]          = "WSANOTINITIALISED 10093 Successful WSAStartup not yet performed.";
  WinErrorMsg[WSAEDISCON]                 = "WSAEDISCON 10101 Graceful shutdown in progress.";
  WinErrorMsg[WSAENOMORE]                 = "WSAENOMORE 10102 No more results.";
  WinErrorMsg[WSAECANCELLED]              = "WSAECANCELLED 10103 Call has been canceled.";
  WinErrorMsg[WSAEINVALIDPROCTABLE]       = "WSAEINVALIDPROCTABLE 10104 Procedure call table is invalid.";
  WinErrorMsg[WSAEINVALIDPROVIDER]        = "WSAEINVALIDPROVIDER 10105 Service provider is invalid.";
  WinErrorMsg[WSAEPROVIDERFAILEDINIT]     = "WSAEPROVIDERFAILEDINIT 10106 Service provider failed to initialize.";
  WinErrorMsg[WSASYSCALLFAILURE]          = "WSASYSCALLFAILURE 10107 System call failure.";
  WinErrorMsg[WSASERVICE_NOT_FOUND]       = "WSASERVICE_NOT_FOUND 10108 Service not found.";
  WinErrorMsg[WSATYPE_NOT_FOUND]          = "WSATYPE_NOT_FOUND 10109 Class type not found.";
  WinErrorMsg[WSA_E_NO_MORE]              = "WSA_E_NO_MORE 10110 No more results.";
  WinErrorMsg[WSA_E_CANCELLED]            = "WSA_E_CANCELLED 10111 Call was canceled.";
  WinErrorMsg[WSAEREFUSED]                = "WSAEREFUSED 10112 Database query was refused.";
  WinErrorMsg[WSAHOST_NOT_FOUND]          = "WSAHOST_NOT_FOUND 11001 Host not found.";
  WinErrorMsg[WSATRY_AGAIN]               = "WSATRY_AGAIN 11002 Nonauthoritative host not found.";
  WinErrorMsg[WSANO_RECOVERY]             = "WSANO_RECOVERY 11003 This is a nonrecoverable error.";
  WinErrorMsg[WSANO_DATA]                 = "WSANO_DATA 11004 Valid name, no data record of requested type.";
  WinErrorMsg[WSA_QOS_RECEIVERS]          = "WSA_QOS_RECEIVERS 11005 QoS receivers.";
  WinErrorMsg[WSA_QOS_SENDERS]            = "WSA_QOS_SENDERS 11006 QoS senders.";
  WinErrorMsg[WSA_QOS_NO_SENDERS]         = "WSA_QOS_NO_SENDERS 11007 No QoS senders.";
  WinErrorMsg[WSA_QOS_NO_RECEIVERS]       = "WSA_QOS_NO_RECEIVERS 11008 QoS no receivers.";
  WinErrorMsg[WSA_QOS_REQUEST_CONFIRMED]  = "WSA_QOS_REQUEST_CONFIRMED 11009 QoS request confirmed.";
  WinErrorMsg[WSA_QOS_ADMISSION_FAILURE]  = "WSA_QOS_ADMISSION_FAILURE 11010 QoS admission error.";
  WinErrorMsg[WSA_QOS_POLICY_FAILURE]     = "WSA_QOS_POLICY_FAILURE 11011 QoS policy failure.";
  WinErrorMsg[WSA_QOS_BAD_STYLE]          = "WSA_QOS_BAD_STYLE 11012 QoS bad style.";
  WinErrorMsg[WSA_QOS_BAD_OBJECT]         = "WSA_QOS_BAD_OBJECT 11013 QoS bad object.";
  WinErrorMsg[WSA_QOS_TRAFFIC_CTRL_ERROR] = "WSA_QOS_TRAFFIC_CTRL_ERROR 11014 QoS traffic control error.";
  WinErrorMsg[WSA_QOS_GENERIC_ERROR]      = "WSA_QOS_GENERIC_ERROR 11015 QoS generic error.";
  WinErrorMsg[WSA_QOS_ESERVICETYPE]       = "WSA_QOS_ESERVICETYPE 11016 QoS service type error.";
  WinErrorMsg[WSA_QOS_EFLOWSPEC]          = "WSA_QOS_EFLOWSPEC 11017 QoS flowspec error.";
  WinErrorMsg[WSA_QOS_EPROVSPECBUF]       = "WSA_QOS_EPROVSPECBUF 11018 Invalid QoS provider buffer.";
  WinErrorMsg[WSA_QOS_EFILTERSTYLE]       = "WSA_QOS_EFILTERSTYLE 11019 Invalid QoS filter style.";
  WinErrorMsg[WSA_QOS_EFILTERTYPE]        = "WSA_QOS_EFILTERTYPE 11020 Invalid QoS filter type.";
  WinErrorMsg[WSA_QOS_EFILTERCOUNT]       = "WSA_QOS_EFILTERCOUNT 11021 Incorrect QoS filter count.";
  WinErrorMsg[WSA_QOS_EOBJLENGTH]         = "WSA_QOS_EOBJLENGTH 11022 Invalid QoS object length.";
  WinErrorMsg[WSA_QOS_EFLOWCOUNT]         = "WSA_QOS_EFLOWCOUNT 11023 Incorrect QoS flow count.";
  WinErrorMsg[WSA_QOS_EUNKOWNPSOBJ]       = "WSA_QOS_EUNKOWNPSOBJ 11024 Unrecognized QoS object.";
  WinErrorMsg[WSA_QOS_EPOLICYOBJ]         = "WSA_QOS_EPOLICYOBJ 11025 Invalid QoS policy object.";
  WinErrorMsg[WSA_QOS_EFLOWDESC]          = "WSA_QOS_EFLOWDESC 11026 Invalid QoS flow descriptor.";
  WinErrorMsg[WSA_QOS_EPSFLOWSPEC]        = "WSA_QOS_EPSFLOWSPEC 11027 Invalid QoS provider-specific flowspec.";
  WinErrorMsg[WSA_QOS_EPSFILTERSPEC]      = "WSA_QOS_EPSFILTERSPEC 11028 Invalid QoS provider-specific filterspec.";
  WinErrorMsg[WSA_QOS_ESDMODEOBJ]         = "WSA_QOS_ESDMODEOBJ 11029 Invalid QoS shape discard mode object.";
  WinErrorMsg[WSA_QOS_ESHAPERATEOBJ]      = "WSA_QOS_ESHAPERATEOBJ 11030 Invalid QoS shaping rate object.";
  WinErrorMsg[WSA_QOS_RESERVED_PETYPE]    = "WSA_QOS_RESERVED_PETYPE 11031 Reserved policy QoS element type.";
};

#elif __linux__                    // creating error mapping for linux system errors
{
  ErrornoErrorMsg[EPERM]           = "EPERM (Operation not permitted) 1";
  ErrornoErrorMsg[ENOENT]          = "ENOENT (No such file or directory) 2";
  ErrornoErrorMsg[ESRCH]           = "ESRCH (No such process) 3";
  ErrornoErrorMsg[EINTR]           = "EINTR (Interrupted system call) 4";
  ErrornoErrorMsg[EIO]             = "EIO (Input/output error) 5";
  ErrornoErrorMsg[ENXIO]           = "ENXIO (No such device or address) 6";
  ErrornoErrorMsg[E2BIG]           = "E2BIG (Argument list too long) 7";
  ErrornoErrorMsg[ENOEXEC]         = "ENOEXEC (Exec format error) 8";
  ErrornoErrorMsg[EBADF]           = "EBADF (Bad file descriptor) 9";
  ErrornoErrorMsg[ECHILD]          = "ECHILD (No child processes) 10";
  ErrornoErrorMsg[EAGAIN]          = "EAGAIN (Resource temporarily unavailable) 11";
  ErrornoErrorMsg[ENOMEM]          = "ENOMEM (Cannot allocate memory) 12";
  ErrornoErrorMsg[EACCES]          = "EACCES (Permission denied) 13";
  ErrornoErrorMsg[EFAULT]          = "EFAULT (Bad address) 14";
  ErrornoErrorMsg[ENOTBLK]         = "ENOTBLK (Block device required) 15";
  ErrornoErrorMsg[EBUSY]           = "EBUSY (Device or resource busy) 16";
  ErrornoErrorMsg[EEXIST]          = "EEXIST (File exists) 17";
  ErrornoErrorMsg[EXDEV]           = "EXDEV (Invalid cross-device link) 18";
  ErrornoErrorMsg[ENODEV]          = "ENODEV (No such device) 19";
  ErrornoErrorMsg[ENOTDIR]         = "ENOTDIR (Not a directory) 20";
  ErrornoErrorMsg[EISDIR]          = "EISDIR (Is a directory) 21";
  ErrornoErrorMsg[EINVAL]          = "EINVAL (Invalid argument) 22";
  ErrornoErrorMsg[ENFILE]          = "ENFILE (Too many open files in system) 23";
  ErrornoErrorMsg[EMFILE]          = "EMFILE (Too many open files) 24";
  ErrornoErrorMsg[ENOTTY]          = "ENOTTY (Inappropriate ioctl for device) 25";
  ErrornoErrorMsg[ETXTBSY]         = "ETXTBSY (Text file busy) 26";
  ErrornoErrorMsg[EFBIG]           = "EFBIG (File too large) 27";
  ErrornoErrorMsg[ENOSPC]          = "ENOSPC (No space left on device) 28";
  ErrornoErrorMsg[ESPIPE]          = "ESPIPE (Illegal seek) 29";
  ErrornoErrorMsg[EROFS]           = "EROFS (Read-only file system) 30";
  ErrornoErrorMsg[EMLINK]          = "EMLINK (Too many links) 31";
  ErrornoErrorMsg[EPIPE]           = "EPIPE (Broken pipe) 32";
  ErrornoErrorMsg[EDOM]            = "EDOM (Numerical argument out of domain) 33";
  ErrornoErrorMsg[ERANGE]          = "ERANGE (Numerical result out of range) 34";
  ErrornoErrorMsg[EDEADLK]         = "EDEADLK (Resource deadlock avoided) 35";
  ErrornoErrorMsg[ENAMETOOLONG]    = "ENAMETOOLONG (File name too long) 36";
  ErrornoErrorMsg[ENOLCK]          = "ENOLCK (No locks available) 37";
  ErrornoErrorMsg[ENOSYS]          = "ENOSYS (Function not implemented) 38";
  ErrornoErrorMsg[ENOTEMPTY]       = "ENOTEMPTY (Directory not empty) 39";
  ErrornoErrorMsg[ELOOP]           = "ELOOP (Too many levels of symbolic links) 40";
  ErrornoErrorMsg[EWOULDBLOCK]     = "EWOULDBLOCK (Resource temporarily unavailable) 41";
  ErrornoErrorMsg[ENOMSG]          = "ENOMSG (No message of desired type) 42";
  ErrornoErrorMsg[EIDRM]           = "EIDRM (Identifier removed) 43";
  ErrornoErrorMsg[ECHRNG]          = "ECHRNG (Channel number out of range) 44";
  ErrornoErrorMsg[EL2NSYNC]        = "EL2NSYNC (Level 2 not synchronized) 45";
  ErrornoErrorMsg[EL3HLT]          = "EL3HLT (Level 3 halted) 46";
  ErrornoErrorMsg[EL3RST]          = "EL3RST (Level 3 reset) 47";
  ErrornoErrorMsg[ELNRNG]          = "ELNRNG (Link number out of range) 48";
  ErrornoErrorMsg[EUNATCH]         = "EUNATCH (Protocol driver not attached) 49";
  ErrornoErrorMsg[ENOCSI]          = "ENOCSI (No CSI structure available) 50";
  ErrornoErrorMsg[EL2HLT]          = "EL2HLT (Level 2 halted) 51";
  ErrornoErrorMsg[EBADE]           = "EBADE (Invalid exchange) 52";
  ErrornoErrorMsg[EBADR]           = "EBADR (Invalid request descriptor) 53";    
  ErrornoErrorMsg[EXFULL]          = "EXFULL (Exchange full) 54";
  ErrornoErrorMsg[ENOANO]          = "ENOANO (No anode) 55";
  ErrornoErrorMsg[EBADRQC]         = "EBADRQC (Invalid request code) 56";
  ErrornoErrorMsg[EBADSLT]         = "EBADSLT (Invalid slot) 57";
  ErrornoErrorMsg[EDEADLOCK]       = "EDEADLOCK (Resource deadlock avoided) 58";
  ErrornoErrorMsg[EBFONT]          = "EBFONT (Bad font file format) 59";
  ErrornoErrorMsg[ENOSTR]          = "ENOSTR (Device not a stream) 60";
  ErrornoErrorMsg[ENODATA]         = "ENODATA (No data available) 61";
  ErrornoErrorMsg[ETIME]           = "ETIME (Timer expired) 62";
  ErrornoErrorMsg[ENOSR]           = "ENOSR (Out of streams resources) 63";
  ErrornoErrorMsg[ENONET]          = "ENONET (Machine is not on the network) 64";
  ErrornoErrorMsg[ENOPKG]          = "ENOPKG (Package not installed) 65";
  ErrornoErrorMsg[EREMOTE]         = "EREMOTE (Object is remote) 66";
  ErrornoErrorMsg[ENOLINK]         = "ENOLINK (Link has been severed) 67";
  ErrornoErrorMsg[EADV]            = "EADV (Advertise error) 68";
  ErrornoErrorMsg[ESRMNT]          = "ESRMNT (Srmount error) 69";
  ErrornoErrorMsg[ECOMM]           = "ECOMM (Communication error on send) 70";
  ErrornoErrorMsg[EPROTO]          = "EPROTO (Protocol error) 71";
  ErrornoErrorMsg[EMULTIHOP]       = "EMULTIHOP (Multihop attempted) 72";
  ErrornoErrorMsg[EDOTDOT]         = "EDOTDOT (RFS specific error) 73";
  ErrornoErrorMsg[EBADMSG]         = "EBADMSG (Bad message) 74";
  ErrornoErrorMsg[EOVERFLOW]       = "EOVERFLOW (Value too large for defined data type) 75";
  ErrornoErrorMsg[ENOTUNIQ]        = "ENOTUNIQ (Name not unique on network) 76";
  ErrornoErrorMsg[EBADFD]          = "EBADFD (File descriptor in bad state) 77";
  ErrornoErrorMsg[EREMCHG]         = "EREMCHG (Remote address changed) 78";
  ErrornoErrorMsg[ELIBACC]         = "ELIBACC (Can not access a needed shared library) 79";
  ErrornoErrorMsg[ELIBBAD]         = "ELIBBAD (Accessing a corrupted shared library) 80";
  ErrornoErrorMsg[ELIBSCN]         = "ELIBSCN (.lib section in a.out corrupted) 81";
  ErrornoErrorMsg[ELIBMAX]         = "ELIBMAX (Attempting to link in too many shared libraries) 82";
  ErrornoErrorMsg[ELIBEXEC]        = "ELIBEXEC (Cannot exec a shared library directly) 83";
  ErrornoErrorMsg[EILSEQ]          = "EILSEQ (Invalid or incomplete multibyte or wide character) 84";
  ErrornoErrorMsg[ERESTART]        = "ERESTART (Interrupted system call should be restarted) 85";
  ErrornoErrorMsg[ESTRPIPE]        = "ESTRPIPE (Streams pipe error) 86";
  ErrornoErrorMsg[EUSERS]          = "EUSERS (Too many users) 87";
  ErrornoErrorMsg[ENOTSOCK]        = "ENOTSOCK (Socket operation on non-socket) 88";
  ErrornoErrorMsg[EDESTADDRREQ]    = "EDESTADDRREQ (Destination address required) 89";
  ErrornoErrorMsg[EMSGSIZE]        = "EMSGSIZE (Message too long) 90";
  ErrornoErrorMsg[EPROTOTYPE]      = "EPROTOTYPE (Protocol wrong type for socket) 91";
  ErrornoErrorMsg[ENOPROTOOPT]     = "ENOPROTOOPT (Protocol not available) 92";
  ErrornoErrorMsg[EPROTONOSUPPORT] = "EPROTONOSUPPORT (Protocol not supported) 93";
  ErrornoErrorMsg[ESOCKTNOSUPPORT] = "ESOCKTNOSUPPORT (Socket type not supported) 94";
  ErrornoErrorMsg[EOPNOTSUPP]      = "EOPNOTSUPP (Operation not supported) 95";
  ErrornoErrorMsg[EPFNOSUPPORT]    = "EPFNOSUPPORT (Protocol family not supported) 96";
  ErrornoErrorMsg[EAFNOSUPPORT]    = "EAFNOSUPPORT (Address family not supported by protocol) 97";
  ErrornoErrorMsg[EADDRINUSE]      = "EADDRINUSE (Address already in use) 98";
  ErrornoErrorMsg[EADDRNOTAVAIL]   = "EADDRNOTAVAIL (Cannot assign requested address) 99";
  ErrornoErrorMsg[ENETDOWN]        = "ENETDOWN (Network is down) 100";
  ErrornoErrorMsg[ENETUNREACH]     = "ENETUNREACH (Network is unreachable) 101";
  ErrornoErrorMsg[ENETRESET]       = "ENETRESET (Network dropped connection on reset) 102";
  ErrornoErrorMsg[ECONNABORTED]    = "ECONNABORTED (Software caused connection abort) 103";
  ErrornoErrorMsg[ECONNRESET]      = "ECONNRESET (Connection reset by peer) 104";
  ErrornoErrorMsg[ENOBUFS]         = "ENOBUFS (No buffer space available) 105";
  ErrornoErrorMsg[EISCONN]         = "EISCONN (Transport endpoint is already connected) 106";
  ErrornoErrorMsg[ENOTCONN]        = "ENOTCONN (Transport endpoint is not connected) 107";
  ErrornoErrorMsg[ESHUTDOWN]       = "ESHUTDOWN (Cannot send after transport endpoint shutdown) 108";
  ErrornoErrorMsg[ETOOMANYREFS]    = "ETOOMANYREFS (Too many references: cannot splice) 109";
  ErrornoErrorMsg[ETIMEDOUT]       = "ETIMEDOUT (Connection timed out) 110";
  ErrornoErrorMsg[ECONNREFUSED]    = "ECONNREFUSED (Connection refused) 111";
  ErrornoErrorMsg[EHOSTDOWN]       = "EHOSTDOWN (Host is down) 112";
  ErrornoErrorMsg[EHOSTUNREACH]    = "EHOSTUNREACH (No route to host) 113";
  ErrornoErrorMsg[EALREADY]        = "EALREADY (Operation already in progress) 114";
  ErrornoErrorMsg[EINPROGRESS]     = "EINPROGRESS (Operation now in progress) 115";
  ErrornoErrorMsg[ESTALE]          = "ESTALE (Stale file handle) 116";
  ErrornoErrorMsg[EUCLEAN]         = "EUCLEAN (Structure needs cleaning) 117";
  ErrornoErrorMsg[ENOTNAM]         = "ENOTNAM (Not a XENIX named type file) 118";
  ErrornoErrorMsg[ENAVAIL]         = "ENAVAIL (No XENIX semaphores available) 119";
  ErrornoErrorMsg[EISNAM]          = "EISNAM (Is a named type file) 120";
  ErrornoErrorMsg[EREMOTEIO]       = "EREMOTEIO (Remote I/O error) 121";
  ErrornoErrorMsg[EDQUOT]          = "EDQUOT (Disk quota exceeded) 122";
  ErrornoErrorMsg[ENOMEDIUM]       = "ENOMEDIUM (No medium found) 123";
  ErrornoErrorMsg[EMEDIUMTYPE]     = "EMEDIUMTYPE (Wrong medium type) 124";
  ErrornoErrorMsg[ECANCELED]       = "ECANCELED (Operation canceled) 125";
  ErrornoErrorMsg[ENOKEY]          = "ENOKEY (Required key not available) 126";
  ErrornoErrorMsg[EKEYEXPIRED]     = "EKEYEXPIRED (Key has expired) 127";
  ErrornoErrorMsg[EKEYREVOKED]     = "EKEYREVOKED (Key has been revoked) 128";
  ErrornoErrorMsg[EKEYREJECTED]    = "EKEYREJECTED (Key was rejected by service) 129";
  ErrornoErrorMsg[EOWNERDEAD]      = "EOWNERDEAD (Owner died) 130";
  ErrornoErrorMsg[ENOTRECOVERABLE] = "ENOTRECOVERABLE (State not recoverable) 131";
  ErrornoErrorMsg[ERFKILL]         = "ERFKILL (Operation not possible due to RF-kill) 132";
  ErrornoErrorMsg[EHWPOISON]       = "EHWPOISON (Memory page has hardware error) 133";
};

#endif

};

void OpenSSLError::CreateMappingErrorMsg()    // creating error mapping for OpenSSL errors
{
  OpenSSLErrorMsg[SSL_ERROR_NONE]             = "SSL_ERROR_NONE 0";
  OpenSSLErrorMsg[SSL_ERROR_SSL]              = "SSL_ERROR_SSL 1";
  OpenSSLErrorMsg[SSL_ERROR_WANT_READ]        = "SSL_ERROR_WANT_READ 2";
  OpenSSLErrorMsg[SSL_ERROR_WANT_WRITE]       = "SSL_ERROR_WANT_WRITE 3";
  OpenSSLErrorMsg[SSL_ERROR_WANT_X509_LOOKUP] = "SSL_ERROR_WANT_X509_LOOKUP 4";
  OpenSSLErrorMsg[SSL_ERROR_SYSCALL]          = "SSL_ERROR_SYSCALL 5";
  OpenSSLErrorMsg[SSL_ERROR_ZERO_RETURN]      = "SSL_ERROR_ZERO_RETURN 6";
  OpenSSLErrorMsg[SSL_ERROR_WANT_CONNECT]     = "SSL_ERROR_WANT_CONNECT 7";
  OpenSSLErrorMsg[SSL_ERROR_WANT_ACCEPT]      = "SSL_ERROR_WANT_ACCEPT 8";
};

void X509Error::CreateMappingErrorMsg()       // creating error mapping for X509 errors
{
  X509ErrorMsg[X509_V_OK]                                     = "X509_V_OK 0";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT]          = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT 2";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_GET_CRL]                  = "X509_V_ERR_UNABLE_TO_GET_CRL 3";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE]   = "X509_V_ERR_UNABLE_TO_DECRYPT_CERT_SIGNATURE 4";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE]    = "X509_V_ERR_UNABLE_TO_DECRYPT_CRL_SIGNATURE 5";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY] = "X509_V_ERR_UNABLE_TO_DECODE_ISSUER_PUBLIC_KEY 6";
  X509ErrorMsg[X509_V_ERR_CERT_SIGNATURE_FAILURE]             = "X509_V_ERR_CERT_SIGNATURE_FAILURE 7";
  X509ErrorMsg[X509_V_ERR_CRL_SIGNATURE_FAILURE]              = "X509_V_ERR_CRL_SIGNATURE_FAILURE 8";
  X509ErrorMsg[X509_V_ERR_CERT_NOT_YET_VALID]                 = "X509_V_ERR_CERT_NOT_YET_VALID 9";
  X509ErrorMsg[X509_V_ERR_CERT_HAS_EXPIRED]                   = "X509_V_ERR_CERT_HAS_EXPIRED 10";
  X509ErrorMsg[X509_V_ERR_CRL_NOT_YET_VALID]                  = "X509_V_ERR_CRL_NOT_YET_VALID 11";
  X509ErrorMsg[X509_V_ERR_CRL_HAS_EXPIRED]                    = "X509_V_ERR_CRL_HAS_EXPIRED 12";
  X509ErrorMsg[X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD]     = "X509_V_ERR_ERROR_IN_CERT_NOT_BEFORE_FIELD 13";
  X509ErrorMsg[X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD]      = "X509_V_ERR_ERROR_IN_CERT_NOT_AFTER_FIELD 14";
  X509ErrorMsg[X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD]     = "X509_V_ERR_ERROR_IN_CRL_LAST_UPDATE_FIELD 15"; 
  X509ErrorMsg[X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD]     = "X509_V_ERR_ERROR_IN_CRL_NEXT_UPDATE_FIELD 16";
  X509ErrorMsg[X509_V_ERR_OUT_OF_MEM]                         = "X509_V_ERR_OUT_OF_MEM 17";
  X509ErrorMsg[X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT]        = "X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT 18";
  X509ErrorMsg[X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN]          = "X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN 19";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY]  = "X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY 20";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE]    = "X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE 21";
  X509ErrorMsg[X509_V_ERR_CERT_CHAIN_TOO_LONG]                = "X509_V_ERR_CERT_CHAIN_TOO_LONG 22";
  X509ErrorMsg[X509_V_ERR_CERT_REVOKED]                       = "X509_V_ERR_CERT_REVOKED 23";
  X509ErrorMsg[X509_V_ERR_INVALID_CA]                         = "X509_V_ERR_INVALID_CA 24";
  X509ErrorMsg[X509_V_ERR_PATH_LENGTH_EXCEEDED]               = "X509_V_ERR_PATH_LENGTH_EXCEEDED 25";
  X509ErrorMsg[X509_V_ERR_INVALID_PURPOSE]                    = "X509_V_ERR_INVALID_PURPOSE 26";
  X509ErrorMsg[X509_V_ERR_CERT_UNTRUSTED]                     = "X509_V_ERR_CERT_UNTRUSTED 27";
  X509ErrorMsg[X509_V_ERR_CERT_REJECTED]                      = "X509_V_ERR_CERT_REJECTED 28";
  X509ErrorMsg[X509_V_ERR_SUBJECT_ISSUER_MISMATCH]            = "X509_V_ERR_SUBJECT_ISSUER_MISMATCH 29";
  X509ErrorMsg[X509_V_ERR_AKID_SKID_MISMATCH]                 = "X509_V_ERR_AKID_SKID_MISMATCH 30";
  X509ErrorMsg[X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH]        = "X509_V_ERR_AKID_ISSUER_SERIAL_MISMATCH 31";
  X509ErrorMsg[X509_V_ERR_KEYUSAGE_NO_CERTSIGN]               = "X509_V_ERR_KEYUSAGE_NO_CERTSIGN 32";
  X509ErrorMsg[X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER]           = "X509_V_ERR_UNABLE_TO_GET_CRL_ISSUER 33";
  X509ErrorMsg[X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION]       = "X509_V_ERR_UNHANDLED_CRITICAL_EXTENSION 34";
  X509ErrorMsg[X509_V_ERR_KEYUSAGE_NO_CRL_SIGN]               = "X509_V_ERR_KEYUSAGE_NO_CRL_SIGN 35";
  X509ErrorMsg[X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION]   = "X509_V_ERR_UNHANDLED_CRITICAL_CRL_EXTENSION 36";
  X509ErrorMsg[X509_V_ERR_INVALID_NON_CA]                     = "X509_V_ERR_INVALID_NON_CA 37";
  X509ErrorMsg[X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED]         = "X509_V_ERR_PROXY_PATH_LENGTH_EXCEEDED 38";
  X509ErrorMsg[X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE]      = "X509_V_ERR_KEYUSAGE_NO_DIGITAL_SIGNATURE 39";
  X509ErrorMsg[X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED]     = "X509_V_ERR_PROXY_CERTIFICATES_NOT_ALLOWED 40";
  X509ErrorMsg[X509_V_ERR_INVALID_EXTENSION]                  = "X509_V_ERR_INVALID_EXTENSION 41";
  X509ErrorMsg[X509_V_ERR_INVALID_POLICY_EXTENSION]           = "X509_V_ERR_INVALID_POLICY_EXTENSION 42";
  X509ErrorMsg[X509_V_ERR_NO_EXPLICIT_POLICY]                 = "X509_V_ERR_NO_EXPLICIT_POLICY 43";
  X509ErrorMsg[X509_V_ERR_DIFFERENT_CRL_SCOPE]                = "X509_V_ERR_DIFFERENT_CRL_SCOPE 44";
  X509ErrorMsg[X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE]      = "X509_V_ERR_UNSUPPORTED_EXTENSION_FEATURE 45";
  X509ErrorMsg[X509_V_ERR_UNNESTED_RESOURCE]                  = "X509_V_ERR_UNNESTED_RESOURCE 46";
  X509ErrorMsg[X509_V_ERR_PERMITTED_VIOLATION]                = "X509_V_ERR_PERMITTED_VIOLATION 47";
  X509ErrorMsg[X509_V_ERR_EXCLUDED_VIOLATION]                 = "X509_V_ERR_EXCLUDED_VIOLATION 48";
  X509ErrorMsg[X509_V_ERR_SUBTREE_MINMAX]                     = "X509_V_ERR_SUBTREE_MINMAX 49";
  X509ErrorMsg[X509_V_ERR_APPLICATION_VERIFICATION]           = "X509_V_ERR_APPLICATION_VERIFICATION 50";
  X509ErrorMsg[X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE]        = "X509_V_ERR_UNSUPPORTED_CONSTRAINT_TYPE 51";
  X509ErrorMsg[X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX]      = "X509_V_ERR_UNSUPPORTED_CONSTRAINT_SYNTAX 52";
  X509ErrorMsg[X509_V_ERR_UNSUPPORTED_NAME_SYNTAX]            = "X509_V_ERR_UNSUPPORTED_NAME_SYNTAX 53";
  X509ErrorMsg[X509_V_ERR_CRL_PATH_VALIDATION_ERROR]          = "X509_V_ERR_CRL_PATH_VALIDATION_ERROR 54";
};

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
