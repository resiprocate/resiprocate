#include <iostream>
#include <map>
#include <string>
#include <errno.h>
#include <openssl/ssl.h>
#include "rutil/Errdes.hxx"

#ifdef _WIN32
#include <winsock.h>
#include <windows.h>
#endif

using namespace std;

#define _GET_CODE(x) #x
#define ERR_WITH_CODE(m, s) #m ": " s " (" _GET_CODE(m) ")"

#ifdef _WIN32
map <int, string> WinErrorMsg;
#elif __linux__
map <int, string> ErrornoErrorMsg;
#endif
map <int, string> OpenSSLErrorMsg;


string ErrnoError::SearchErrorMsg(int Error)
{
  string result;

#ifdef _WIN32

    if(WinErrorMsg.find(Error) != WinErrorMsg.end())
      result = WinErrorMsg[Error];
    else
      result = "Unknown error";

#elif __linux__

    if(ErrornoErrorMsg.find(Error) != ErrornoErrorMsg.end())
      result = ErrornoErrorMsg[Error];
    else
      result = "Unknown error";

#endif

  return result;
}

string OpenSSLError::SearchErrorMsg(int Error)
{
  string result;

  if(OpenSSLErrorMsg.find(Error) != OpenSSLErrorMsg.end())
      result = OpenSSLErrorMsg[Error];
  else
    result = "Unknown error";

  return result;
}

void ErrnoError::CreateMappingErrorMsg()
{
#ifdef _WIN32
{
  WinErrorMsg[WSA_INVALID_HANDLE]         = ERR_WITH_CODE(WSA_INVALID_HANDLE, "Specified event object handle is invalid.");
  WinErrorMsg[WSA_NOT_ENOUGH_MEMORY]      = ERR_WITH_CODE(WSA_NOT_ENOUGH_MEMORY, "Insufficient memory available.");
  WinErrorMsg[WSA_INVALID_PARAMETER]      = ERR_WITH_CODE(WSA_INVALID_PARAMETER, "One or more parameters are invalid.");
  WinErrorMsg[WSA_OPERATION_ABORTED]      = ERR_WITH_CODE(WSA_OPERATION_ABORTED, "Overlapped operation aborted.");
  WinErrorMsg[WSA_IO_INCOMPLETE]          = ERR_WITH_CODE(WSA_IO_INCOMPLETE, "Overlapped I/O event object not in signaled state.");
  WinErrorMsg[SA_IO_PENDING]              = ERR_WITH_CODE(SA_IO_PENDING, "Overlapped operations will complete later.");
  WinErrorMsg[WSAEINTR]                   = ERR_WITH_CODE(WSAEINTR, "Interrupted function call.");
  WinErrorMsg[WSAEBADF]                   = ERR_WITH_CODE(WSAEBADF, "File handle is not valid.");
  WinErrorMsg[WSAEACCES]                  = ERR_WITH_CODE(WSAEACCES, "Permission denied.");
  WinErrorMsg[WSAEFAULT]                  = ERR_WITH_CODE(WSAEFAULT, "Bad address.");
  WinErrorMsg[WSAEINVAL]                  = ERR_WITH_CODE(WSAEINVAL, "Invalid argument.");
  WinErrorMsg[WSAEMFILE]                  = ERR_WITH_CODE(WSAEMFILE, "Too many open files.");
  WinErrorMsg[WSAEWOULDBLOCK]             = ERR_WITH_CODE(WSAEWOULDBLOCK, "Resource temporarily unavailable.");
  WinErrorMsg[WSAEINPROGRESS]             = ERR_WITH_CODE(WSAEINPROGRESS, "Operation now in progress.");
  WinErrorMsg[WSAEALREADY]                = ERR_WITH_CODE(WSAEALREADY, "Operation already in progress.");
  WinErrorMsg[WSAENOTSOCK]                = ERR_WITH_CODE(WSAENOTSOCK, "Socket operation on nonsocket.");
  WinErrorMsg[WSAEDESTADDRREQ]            = ERR_WITH_CODE(WSAEDESTADDRREQ, "Destination address required.");
  WinErrorMsg[WSAEMSGSIZE]                = ERR_WITH_CODE(WSAEMSGSIZE, "Message too long.");
  WinErrorMsg[WSAEPROTOTYPE]              = ERR_WITH_CODE(WSAEPROTOTYPE, "Protocol wrong type for socket.");
  WinErrorMsg[WSAENOPROTOOPT]             = ERR_WITH_CODE(WSAENOPROTOOPT, "Bad protocol option.");
  WinErrorMsg[WSAEPROTONOSUPPORT]         = ERR_WITH_CODE(WSAEPROTONOSUPPORT, "Protocol not supported.");
  WinErrorMsg[WSAESOCKTNOSUPPORT]         = ERR_WITH_CODE(WSAESOCKTNOSUPPORT, "Socket type not supported.");
  WinErrorMsg[WSAEOPNOTSUPP]              = ERR_WITH_CODE(WSAEOPNOTSUPP, "Operation not supported.");
  WinErrorMsg[WSAEPFNOSUPPORT]            = ERR_WITH_CODE(WSAEPFNOSUPPORT, "Protocol family not supported.");
  WinErrorMsg[WSAEAFNOSUPPORT]            = ERR_WITH_CODE(WSAEAFNOSUPPORT, "Address family not supported by protocol family.");
  WinErrorMsg[WSAEADDRINUSE]              = ERR_WITH_CODE(WSAEADDRINUSE, "Address already in use.");
  WinErrorMsg[WSAEADDRNOTAVAIL]           = ERR_WITH_CODE(WSAEADDRNOTAVAIL, "Cannot assign requested address.");
  WinErrorMsg[WSAENETDOWN]                = ERR_WITH_CODE(WSAENETDOWN, "Network is down.");
  WinErrorMsg[WSAENETUNREACH]             = ERR_WITH_CODE(WSAENETUNREACH, "Network is unreachable.");
  WinErrorMsg[WSAENETRESET]               = ERR_WITH_CODE(WSAENETRESET, "Network dropped connection on reset.");
  WinErrorMsg[WSAECONNABORTED]            = ERR_WITH_CODE(WSAECONNABORTED, "Software caused connection abort.");
  WinErrorMsg[WSAECONNRESET]              = ERR_WITH_CODE(WSAECONNRESET, "Connection reset by peer.");
  WinErrorMsg[WSAENOBUFS]                 = ERR_WITH_CODE(WSAENOBUFS, "No buffer space available.");
  WinErrorMsg[WSAEISCONN]                 = ERR_WITH_CODE(WSAEISCONN, "Socket is already connected.");
  WinErrorMsg[WSAENOTCONN]                = ERR_WITH_CODE(WSAENOTCONN, "Socket is not connected.");
  WinErrorMsg[WSAESHUTDOWN]               = ERR_WITH_CODE(WSAESHUTDOWN, "Cannot send after socket shutdown.");
  WinErrorMsg[WSAETOOMANYREFS]            = ERR_WITH_CODE(WSAETOOMANYREFS, "Too many references.");
  WinErrorMsg[WSAETIMEDOUT]               = ERR_WITH_CODE(WSAETIMEDOUT, "Connection timed out.");
  WinErrorMsg[WSAECONNREFUSED]            = ERR_WITH_CODE(WSAECONNREFUSED, "Connection refused.");
  WinErrorMsg[WSAELOOP]                   = ERR_WITH_CODE(WSAELOOP, "Cannot translate name.");
  WinErrorMsg[WSAENAMETOOLONG]            = ERR_WITH_CODE(WSAENAMETOOLONG, "Name too long.");
  WinErrorMsg[WSAEHOSTDOWN]               = ERR_WITH_CODE(WSAEHOSTDOWN, "Host is down.");
  WinErrorMsg[WSAEHOSTUNREACH]            = ERR_WITH_CODE(WSAEHOSTUNREACH, "No route to host.");
  WinErrorMsg[WSAENOTEMPTY]               = ERR_WITH_CODE(WSAENOTEMPTY, "Directory not empty.");
  WinErrorMsg[WSAEPROCLIM]                = ERR_WITH_CODE(WSAEPROCLIM, "Too many processes.");
  WinErrorMsg[WSAEUSERS]                  = ERR_WITH_CODE(WSAEUSERS, "User quota exceeded.");
  WinErrorMsg[WSAEDQUOT]                  = ERR_WITH_CODE(WSAEDQUOT, "Disk quota exceeded.");
  WinErrorMsg[WSAESTALE]                  = ERR_WITH_CODE(WSAESTALE, "Stale file handle reference.");
  WinErrorMsg[WSAEREMOTE]                 = ERR_WITH_CODE(WSAEREMOTE, "Item is remote.");
  WinErrorMsg[WSASYSNOTREADY]             = ERR_WITH_CODE(WSASYSNOTREADY, "Network subsystem is unavailable.");
  WinErrorMsg[WSAVERNOTSUPPORTED]         = ERR_WITH_CODE(WSAVERNOTSUPPORTED, "Winsock.dll version out of range.");
  WinErrorMsg[WSANOTINITIALISED]          = ERR_WITH_CODE(WSANOTINITIALISED, "Successful WSAStartup not yet performed.");
  WinErrorMsg[WSAEDISCON]                 = ERR_WITH_CODE(WSAEDISCON, "Graceful shutdown in progress.");
  WinErrorMsg[WSAENOMORE]                 = ERR_WITH_CODE(WSAENOMORE, "No more results.");
  WinErrorMsg[WSAECANCELLED]              = ERR_WITH_CODE(WSAECANCELLED, "Call has been canceled.");
  WinErrorMsg[WSAEINVALIDPROCTABLE]       = ERR_WITH_CODE(WSAEINVALIDPROCTABLE, "Procedure call table is invalid.");
  WinErrorMsg[WSAEINVALIDPROVIDER]        = ERR_WITH_CODE(WSAEINVALIDPROVIDER, "Service provider is invalid.");
  WinErrorMsg[WSAEPROVIDERFAILEDINIT]     = ERR_WITH_CODE(WSAEPROVIDERFAILEDINIT, "Service provider failed to initialize.");
  WinErrorMsg[WSASYSCALLFAILURE]          = ERR_WITH_CODE(WSASYSCALLFAILURE, "System call failure.");
  WinErrorMsg[WSASERVICE_NOT_FOUND]       = ERR_WITH_CODE(WSASERVICE_NOT_FOUND, "Service not found.");
  WinErrorMsg[WSATYPE_NOT_FOUND]          = ERR_WITH_CODE(WSATYPE_NOT_FOUND, "Class type not found.");
  WinErrorMsg[WSA_E_NO_MORE]              = ERR_WITH_CODE(WSA_E_NO_MORE, "No more results.");
  WinErrorMsg[WSA_E_CANCELLED]            = ERR_WITH_CODE(WSA_E_CANCELLED, "Call was canceled.");
  WinErrorMsg[WSAEREFUSED]                = ERR_WITH_CODE(WSAEREFUSED, "Database query was refused.");
  WinErrorMsg[WSAHOST_NOT_FOUND]          = ERR_WITH_CODE(WSAHOST_NOT_FOUND, "Host not found.");
  WinErrorMsg[WSATRY_AGAIN]               = ERR_WITH_CODE(WSATRY_AGAIN, "Nonauthoritative host not found.");
  WinErrorMsg[WSANO_RECOVERY]             = ERR_WITH_CODE(WSANO_RECOVERY, "This is a nonrecoverable error.");
  WinErrorMsg[WSANO_DATA]                 = ERR_WITH_CODE(WSANO_DATA, "Valid name, no data record of requested type.");
  WinErrorMsg[WSA_QOS_RECEIVERS]          = ERR_WITH_CODE(WSA_QOS_RECEIVERS, "QoS receivers.");
  WinErrorMsg[WSA_QOS_SENDERS]            = ERR_WITH_CODE(WSA_QOS_SENDERS, "QoS senders.");
  WinErrorMsg[WSA_QOS_NO_SENDERS]         = ERR_WITH_CODE(WSA_QOS_NO_SENDERS, "No QoS senders.");
  WinErrorMsg[WSA_QOS_NO_RECEIVERS]       = ERR_WITH_CODE(WSA_QOS_NO_RECEIVERS, "QoS no receivers.");
  WinErrorMsg[WSA_QOS_REQUEST_CONFIRMED]  = ERR_WITH_CODE(WSA_QOS_REQUEST_CONFIRMED, "QoS request confirmed.");
  WinErrorMsg[WSA_QOS_ADMISSION_FAILURE]  = ERR_WITH_CODE(WSA_QOS_ADMISSION_FAILURE, "QoS admission error.");
  WinErrorMsg[WSA_QOS_POLICY_FAILURE]     = ERR_WITH_CODE(WSA_QOS_POLICY_FAILURE, "QoS policy failure.");
  WinErrorMsg[WSA_QOS_BAD_STYLE]          = ERR_WITH_CODE(WSA_QOS_BAD_STYLE, "QoS bad style.");
  WinErrorMsg[WSA_QOS_BAD_OBJECT]         = ERR_WITH_CODE(WSA_QOS_BAD_OBJECT, "QoS bad object.");
  WinErrorMsg[WSA_QOS_TRAFFIC_CTRL_ERROR] = ERR_WITH_CODE(WSA_QOS_TRAFFIC_CTRL_ERROR, "QoS traffic control error.");
  WinErrorMsg[WSA_QOS_GENERIC_ERROR]      = ERR_WITH_CODE(WSA_QOS_GENERIC_ERROR, "QoS generic error.");
  WinErrorMsg[WSA_QOS_ESERVICETYPE]       = ERR_WITH_CODE(WSA_QOS_ESERVICETYPE, "QoS service type error.");
  WinErrorMsg[WSA_QOS_EFLOWSPEC]          = ERR_WITH_CODE(WSA_QOS_EFLOWSPEC, "QoS flowspec error.");
  WinErrorMsg[WSA_QOS_EPROVSPECBUF]       = ERR_WITH_CODE(WSA_QOS_EPROVSPECBUF, "Invalid QoS provider buffer.");
  WinErrorMsg[WSA_QOS_EFILTERSTYLE]       = ERR_WITH_CODE(WSA_QOS_EFILTERSTYLE, "Invalid QoS filter style.");
  WinErrorMsg[WSA_QOS_EFILTERTYPE]        = ERR_WITH_CODE(WSA_QOS_EFILTERTYPE, "Invalid QoS filter type.");
  WinErrorMsg[WSA_QOS_EFILTERCOUNT]       = ERR_WITH_CODE(WSA_QOS_EFILTERCOUNT, "Incorrect QoS filter count.");
  WinErrorMsg[WSA_QOS_EOBJLENGTH]         = ERR_WITH_CODE(WSA_QOS_EOBJLENGTH, "Invalid QoS object length.");
  WinErrorMsg[WSA_QOS_EFLOWCOUNT]         = ERR_WITH_CODE(WSA_QOS_EFLOWCOUNT, "Incorrect QoS flow count.");
  WinErrorMsg[WSA_QOS_EUNKOWNPSOBJ]       = ERR_WITH_CODE(WSA_QOS_EUNKOWNPSOBJ, "Unrecognized QoS object.");
  WinErrorMsg[WSA_QOS_EPOLICYOBJ]         = ERR_WITH_CODE(WSA_QOS_EPOLICYOBJ, "Invalid QoS policy object.");
  WinErrorMsg[WSA_QOS_EFLOWDESC]          = ERR_WITH_CODE(WSA_QOS_EFLOWDESC, "Invalid QoS flow descriptor.");
  WinErrorMsg[WSA_QOS_EPSFLOWSPEC]        = ERR_WITH_CODE(WSA_QOS_EPSFLOWSPEC, "Invalid QoS provider-specific flowspec.");
  WinErrorMsg[WSA_QOS_EPSFILTERSPEC]      = ERR_WITH_CODE(WSA_QOS_EPSFILTERSPEC, "Invalid QoS provider-specific filterspec.");
  WinErrorMsg[WSA_QOS_ESDMODEOBJ]         = ERR_WITH_CODE(WSA_QOS_ESDMODEOBJ, "Invalid QoS shape discard mode object.");
  WinErrorMsg[WSA_QOS_ESHAPERATEOBJ]      = ERR_WITH_CODE(WSA_QOS_ESHAPERATEOBJ, "Invalid QoS shaping rate object.");
  WinErrorMsg[WSA_QOS_RESERVED_PETYPE]    = ERR_WITH_CODE(WSA_QOS_RESERVED_PETYPE, "Reserved policy QoS element type.");
};

#elif __linux__
{
  ErrornoErrorMsg[EPERM]           = ERR_WITH_CODE(EPERM, "Operation not permitted");
  ErrornoErrorMsg[ENOENT]          = ERR_WITH_CODE(ENOENT, "No such file or directory");
  ErrornoErrorMsg[ESRCH]           = ERR_WITH_CODE(ESRCH, "No such process");
  ErrornoErrorMsg[EINTR]           = ERR_WITH_CODE(EINTR, "Interrupted system call");
  ErrornoErrorMsg[EIO]             = ERR_WITH_CODE(EIO, "Input/output error");
  ErrornoErrorMsg[ENXIO]           = ERR_WITH_CODE(ENXIO, "No such device or address");
  ErrornoErrorMsg[E2BIG]           = ERR_WITH_CODE(E2BIG, "Argument list too long");
  ErrornoErrorMsg[ENOEXEC]         = ERR_WITH_CODE(ENOEXEC, "Exec format error");
  ErrornoErrorMsg[EBADF]           = ERR_WITH_CODE(EBADF, "Bad file descriptor");
  ErrornoErrorMsg[ECHILD]          = ERR_WITH_CODE(ECHILD, "No child processes");
  ErrornoErrorMsg[EAGAIN]          = ERR_WITH_CODE(EAGAIN, "Resource temporarily unavailable");
  ErrornoErrorMsg[ENOMEM]          = ERR_WITH_CODE(ENOMEM, "Cannot allocate memory");
  ErrornoErrorMsg[EACCES]          = ERR_WITH_CODE(EACCES, "Permission denied");
  ErrornoErrorMsg[EFAULT]          = ERR_WITH_CODE(EFAULT, "Bad address");
  ErrornoErrorMsg[ENOTBLK]         = ERR_WITH_CODE(ENOTBLK, "Block device required");
  ErrornoErrorMsg[EBUSY]           = ERR_WITH_CODE(EBUSY, "Device or resource busy");
  ErrornoErrorMsg[EEXIST]          = ERR_WITH_CODE(EEXIST, "File exists");
  ErrornoErrorMsg[EXDEV]           = ERR_WITH_CODE(EXDEV, "Invalid cross-device link");
  ErrornoErrorMsg[ENODEV]          = ERR_WITH_CODE(ENODEV, "No such device");
  ErrornoErrorMsg[ENOTDIR]         = ERR_WITH_CODE(ENOTDIR, "Not a directory");
  ErrornoErrorMsg[EISDIR]          = ERR_WITH_CODE(EISDIR, "Is a directory");
  ErrornoErrorMsg[EINVAL]          = ERR_WITH_CODE(EINVAL, "Invalid argument");
  ErrornoErrorMsg[ENFILE]          = ERR_WITH_CODE(ENFILE, "Too many open files in system");
  ErrornoErrorMsg[EMFILE]          = ERR_WITH_CODE(EMFILE, "Too many open files");
  ErrornoErrorMsg[ENOTTY]          = ERR_WITH_CODE(ENOTTY, "Inappropriate ioctl for device");
  ErrornoErrorMsg[ETXTBSY]         = ERR_WITH_CODE(ETXTBSY, "Text file busy");
  ErrornoErrorMsg[EFBIG]           = ERR_WITH_CODE(EFBIG, "File too large");
  ErrornoErrorMsg[ENOSPC]          = ERR_WITH_CODE(ENOSPC, "No space left on device");
  ErrornoErrorMsg[ESPIPE]          = ERR_WITH_CODE(ESPIPE, "Illegal seek");
  ErrornoErrorMsg[EROFS]           = ERR_WITH_CODE(EROFS, "Read-only file system");
  ErrornoErrorMsg[EMLINK]          = ERR_WITH_CODE(EMLINK, "Too many links");
  ErrornoErrorMsg[EPIPE]           = ERR_WITH_CODE(EPIPE, "Broken pipe");
  ErrornoErrorMsg[EDOM]            = ERR_WITH_CODE(EDOM, "Numerical argument out of domain");
  ErrornoErrorMsg[ERANGE]          = ERR_WITH_CODE(ERANGE, "Numerical result out of range");
  ErrornoErrorMsg[EDEADLK]         = ERR_WITH_CODE(EDEADLK, "Resource deadlock avoided");
  ErrornoErrorMsg[ENAMETOOLONG]    = ERR_WITH_CODE(ENAMETOOLONG, "File name too long");
  ErrornoErrorMsg[ENOLCK]          = ERR_WITH_CODE(ENOLCK, "No locks available");
  ErrornoErrorMsg[ENOSYS]          = ERR_WITH_CODE(ENOSYS, "Function not implemented");
  ErrornoErrorMsg[ENOTEMPTY]       = ERR_WITH_CODE(ENOTEMPTY, "Directory not empty");
  ErrornoErrorMsg[ELOOP]           = ERR_WITH_CODE(ELOOP, "Too many levels of symbolic links");
  ErrornoErrorMsg[EWOULDBLOCK]     = ERR_WITH_CODE(EWOULDBLOCK, "Resource temporarily unavailable");
  ErrornoErrorMsg[ENOMSG]          = ERR_WITH_CODE(ENOMSG, "No message of desired type");
  ErrornoErrorMsg[EIDRM]           = ERR_WITH_CODE(EIDRM, "Identifier removed");
  ErrornoErrorMsg[ECHRNG]          = ERR_WITH_CODE(ECHRNG, "Channel number out of range");
  ErrornoErrorMsg[EL2NSYNC]        = ERR_WITH_CODE(EL2NSYNC, "Level 2 not synchronized");
  ErrornoErrorMsg[EL3HLT]          = ERR_WITH_CODE(EL3HLT, "Level 3 halted");
  ErrornoErrorMsg[EL3RST]          = ERR_WITH_CODE(EL3RST, "Level 3 reset");
  ErrornoErrorMsg[ELNRNG]          = ERR_WITH_CODE(ELNRNG, "Link number out of range");
  ErrornoErrorMsg[EUNATCH]         = ERR_WITH_CODE(EUNATCH, "Protocol driver not attached");
  ErrornoErrorMsg[ENOCSI]          = ERR_WITH_CODE(ENOCSI, "No CSI structure available");
  ErrornoErrorMsg[EL2HLT]          = ERR_WITH_CODE(EL2HLT, "Level 2 halted");
  ErrornoErrorMsg[EBADE]           = ERR_WITH_CODE(EBADE, "Invalid exchange");
  ErrornoErrorMsg[EBADR]           = ERR_WITH_CODE(EBADR, "Invalid request descriptor");    
  ErrornoErrorMsg[EXFULL]          = ERR_WITH_CODE(EXFULL, "Exchange full");
  ErrornoErrorMsg[ENOANO]          = ERR_WITH_CODE(ENOANO, "No anode");
  ErrornoErrorMsg[EBADRQC]         = ERR_WITH_CODE(EBADRQC, "Invalid request code");
  ErrornoErrorMsg[EBADSLT]         = ERR_WITH_CODE(EBADSLT, "Invalid slot");
  ErrornoErrorMsg[EDEADLOCK]       = ERR_WITH_CODE(EDEADLOCK, "Resource deadlock avoided");
  ErrornoErrorMsg[EBFONT]          = ERR_WITH_CODE(EBFONT, "Bad font file format");
  ErrornoErrorMsg[ENOSTR]          = ERR_WITH_CODE(ENOSTR, "Device not a stream");
  ErrornoErrorMsg[ENODATA]         = ERR_WITH_CODE(ENODATA, "No data available");
  ErrornoErrorMsg[ETIME]           = ERR_WITH_CODE(ETIME, "Timer expired");
  ErrornoErrorMsg[ENOSR]           = ERR_WITH_CODE(ENOSR, "Out of streams resources");
  ErrornoErrorMsg[ENONET]          = ERR_WITH_CODE(ENONET, "Machine is not on the network");
  ErrornoErrorMsg[ENOPKG]          = ERR_WITH_CODE(ENOPKG, "Package not installed");
  ErrornoErrorMsg[EREMOTE]         = ERR_WITH_CODE(EREMOTE, "Object is remote");
  ErrornoErrorMsg[ENOLINK]         = ERR_WITH_CODE(ENOLINK, "Link has been severed");
  ErrornoErrorMsg[EADV]            = ERR_WITH_CODE(EADV, "Advertise error");
  ErrornoErrorMsg[ESRMNT]          = ERR_WITH_CODE(ESRMNT, "Srmount error");
  ErrornoErrorMsg[ECOMM]           = ERR_WITH_CODE(ECOMM, "Communication error on send");
  ErrornoErrorMsg[EPROTO]          = ERR_WITH_CODE(EPROTO, "Protocol error");
  ErrornoErrorMsg[EMULTIHOP]       = ERR_WITH_CODE(EMULTIHOP, "Multihop attempted");
  ErrornoErrorMsg[EDOTDOT]         = ERR_WITH_CODE(EDOTDOT, "RFS specific error");
  ErrornoErrorMsg[EBADMSG]         = ERR_WITH_CODE(EBADMSG, "Bad message");
  ErrornoErrorMsg[EOVERFLOW]       = ERR_WITH_CODE(EOVERFLOW, "Value too large for defined data type");
  ErrornoErrorMsg[ENOTUNIQ]        = ERR_WITH_CODE(ENOTUNIQ, "Name not unique on network");
  ErrornoErrorMsg[EBADFD]          = ERR_WITH_CODE(EBADFD, "File descriptor in bad state");
  ErrornoErrorMsg[EREMCHG]         = ERR_WITH_CODE(EREMCHG, "Remote address changed");
  ErrornoErrorMsg[ELIBACC]         = ERR_WITH_CODE(ELIBACC, "Can not access a needed shared library");
  ErrornoErrorMsg[ELIBBAD]         = ERR_WITH_CODE(ELIBBAD, "Accessing a corrupted shared library");
  ErrornoErrorMsg[ELIBSCN]         = ERR_WITH_CODE(ELIBSCN, ".lib section in a.out corrupted");
  ErrornoErrorMsg[ELIBMAX]         = ERR_WITH_CODE(ELIBMAX, "(Attempting to link in too many shared libraries");
  ErrornoErrorMsg[ELIBEXEC]        = ERR_WITH_CODE(ELIBEXEC, "(Cannot exec a shared library directly");
  ErrornoErrorMsg[EILSEQ]          = ERR_WITH_CODE(EILSEQ, "Invalid or incomplete multibyte or wide character");
  ErrornoErrorMsg[ERESTART]        = ERR_WITH_CODE(ERESTART, "Interrupted system call should be restarted");
  ErrornoErrorMsg[ESTRPIPE]        = ERR_WITH_CODE(ESTRPIPE, "Streams pipe error");
  ErrornoErrorMsg[EUSERS]          = ERR_WITH_CODE(EUSERS, "Too many users");
  ErrornoErrorMsg[ENOTSOCK]        = ERR_WITH_CODE(ENOTSOCK, "Socket operation on non-socket");
  ErrornoErrorMsg[EDESTADDRREQ]    = ERR_WITH_CODE(EDESTADDRREQ, "Destination address required");
  ErrornoErrorMsg[EMSGSIZE]        = ERR_WITH_CODE(EMSGSIZE, "Message too long");
  ErrornoErrorMsg[EPROTOTYPE]      = ERR_WITH_CODE(EPROTOTYPE, "Protocol wrong type for socket");
  ErrornoErrorMsg[ENOPROTOOPT]     = ERR_WITH_CODE(ENOPROTOOPT, "Protocol not available");
  ErrornoErrorMsg[EPROTONOSUPPORT] = ERR_WITH_CODE(EPROTONOSUPPORT, "Protocol not supported");
  ErrornoErrorMsg[ESOCKTNOSUPPORT] = ERR_WITH_CODE(ESOCKTNOSUPPORT, "Socket type not supported");
  ErrornoErrorMsg[EOPNOTSUPP]      = ERR_WITH_CODE(EOPNOTSUPP, "Operation not supported");
  ErrornoErrorMsg[EPFNOSUPPORT]    = ERR_WITH_CODE(EPFNOSUPPORT, "Protocol family not supported");
  ErrornoErrorMsg[EAFNOSUPPORT]    = ERR_WITH_CODE(EAFNOSUPPORT, "Address family not supported by protocol");
  ErrornoErrorMsg[EADDRINUSE]      = ERR_WITH_CODE(EADDRINUSE, "Address already in use");
  ErrornoErrorMsg[EADDRNOTAVAIL]   = ERR_WITH_CODE(EADDRNOTAVAIL, "Cannot assign requested address");
  ErrornoErrorMsg[ENETDOWN]        = ERR_WITH_CODE(ENETDOWN, "Network is down");
  ErrornoErrorMsg[ENETUNREACH]     = ERR_WITH_CODE(ENETUNREACH, "Network is unreachable");
  ErrornoErrorMsg[ENETRESET]       = ERR_WITH_CODE(ENETRESET, "Network dropped connection on reset");
  ErrornoErrorMsg[ECONNABORTED]    = ERR_WITH_CODE(ECONNABORTED, "Software caused connection abort");
  ErrornoErrorMsg[ECONNRESET]      = ERR_WITH_CODE(ECONNRESET, "Connection reset by peer");
  ErrornoErrorMsg[ENOBUFS]         = ERR_WITH_CODE(ENOBUFS, "No buffer space available");
  ErrornoErrorMsg[EISCONN]         = ERR_WITH_CODE(EISCONN, "Transport endpoint is already connected");
  ErrornoErrorMsg[ENOTCONN]        = ERR_WITH_CODE(ENOTCONN, "Transport endpoint is not connected");
  ErrornoErrorMsg[ESHUTDOWN]       = ERR_WITH_CODE(ESHUTDOWN, "Cannot send after transport endpoint shutdown");
  ErrornoErrorMsg[ETOOMANYREFS]    = ERR_WITH_CODE(ETOOMANYREFS, "Too many references: cannot splice");
  ErrornoErrorMsg[ETIMEDOUT]       = ERR_WITH_CODE(ETIMEDOUT, "Connection timed out");
  ErrornoErrorMsg[ECONNREFUSED]    = ERR_WITH_CODE(ECONNREFUSED, "Connection refused");
  ErrornoErrorMsg[EHOSTDOWN]       = ERR_WITH_CODE(EHOSTDOWN, "Host is down");
  ErrornoErrorMsg[EHOSTUNREACH]    = ERR_WITH_CODE(EHOSTUNREACH, "No route to host");
  ErrornoErrorMsg[EALREADY]        = ERR_WITH_CODE(EALREADY, "Operation already in progress");
  ErrornoErrorMsg[EINPROGRESS]     = ERR_WITH_CODE(EINPROGRESS, "Operation now in progress");
  ErrornoErrorMsg[ESTALE]          = ERR_WITH_CODE(ESTALE, "Stale file handle");
  ErrornoErrorMsg[EUCLEAN]         = ERR_WITH_CODE(EUCLEAN, "Structure needs cleaning");
  ErrornoErrorMsg[ENOTNAM]         = ERR_WITH_CODE(ENOTNAM, "Not a XENIX named type file");
  ErrornoErrorMsg[ENAVAIL]         = ERR_WITH_CODE(ENAVAIL, "No XENIX semaphores available");
  ErrornoErrorMsg[EISNAM]          = ERR_WITH_CODE(EISNAM, "Is a named type file");
  ErrornoErrorMsg[EREMOTEIO]       = ERR_WITH_CODE(EREMOTEIO, "Remote I/O error");
  ErrornoErrorMsg[EDQUOT]          = ERR_WITH_CODE(EDQUOT, "Disk quota exceeded");
  ErrornoErrorMsg[ENOMEDIUM]       = ERR_WITH_CODE(ENOMEDIUM, "No medium found");
  ErrornoErrorMsg[EMEDIUMTYPE]     = ERR_WITH_CODE(EMEDIUMTYPE, "Wrong medium type");
  ErrornoErrorMsg[ECANCELED]       = ERR_WITH_CODE(ECANCELED, "Operation canceled");
  ErrornoErrorMsg[ENOKEY]          = ERR_WITH_CODE(ENOKEY, "Required key not available");
  ErrornoErrorMsg[EKEYEXPIRED]     = ERR_WITH_CODE(EKEYEXPIRED, "Key has expired");
  ErrornoErrorMsg[EKEYREVOKED]     = ERR_WITH_CODE(EKEYREVOKED, "Key has been revoked");
  ErrornoErrorMsg[EKEYREJECTED]    = ERR_WITH_CODE(EKEYREJECTED, "Key was rejected by service");
  ErrornoErrorMsg[EOWNERDEAD]      = ERR_WITH_CODE(EOWNERDEAD, "Owner died");
  ErrornoErrorMsg[ENOTRECOVERABLE] = ERR_WITH_CODE(ENOTRECOVERABLE, "State not recoverable");
  ErrornoErrorMsg[ERFKILL]         = ERR_WITH_CODE(ERFKILL, "Operation not possible due to RF-kill");
  ErrornoErrorMsg[EHWPOISON]       = ERR_WITH_CODE(EHWPOISON, "Memory page has hardware error");
};
#endif
};

void OpenSSLError::CreateMappingErrorMsg()
{
  OpenSSLErrorMsg[SSL_ERROR_NONE]             = ERR_WITH_CODE(SSL_ERROR_NONE, "The TLS/SSL I/O operation is completed.");
  OpenSSLErrorMsg[SSL_ERROR_SSL]              = ERR_WITH_CODE(SSL_ERROR_SSL, "A failure in the SSL library occurred.");
  OpenSSLErrorMsg[SSL_ERROR_WANT_READ]        = ERR_WITH_CODE(SSL_ERROR_WANT_READ, "Wait for the socket to be readable.");
  OpenSSLErrorMsg[SSL_ERROR_WANT_WRITE]       = ERR_WITH_CODE(SSL_ERROR_WANT_WRITE, "Wait for the socket to be writeable.");
  OpenSSLErrorMsg[SSL_ERROR_WANT_X509_LOOKUP] = ERR_WITH_CODE(SSL_ERROR_WANT_X509_LOOKUP, "Try later / SSL_ERROR_WANT_X509_LOOKUP");
  OpenSSLErrorMsg[SSL_ERROR_SYSCALL]          = ERR_WITH_CODE(SSL_ERROR_SYSCALL, "Some I/O error occurred.");
  OpenSSLErrorMsg[SSL_ERROR_ZERO_RETURN]      = ERR_WITH_CODE(SSL_ERROR_ZERO_RETURN, "The TLS/SSL connection has been closed.");
  OpenSSLErrorMsg[SSL_ERROR_WANT_CONNECT]     = ERR_WITH_CODE(SSL_ERROR_WANT_CONNECT, "BIO not connected, try later.");
  OpenSSLErrorMsg[SSL_ERROR_WANT_ACCEPT]      = ERR_WITH_CODE(SSL_ERROR_WANT_ACCEPT, "TLS/SSL connection want accept");
};


/* ====================================================================
 *
 * Copyright (C) 2016, Udit Raikwar <udit043.ur@gmail.com>  All rights reserved.
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
