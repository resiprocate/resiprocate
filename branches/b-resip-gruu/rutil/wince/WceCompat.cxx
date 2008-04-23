
#include <windows.h>
#include "WceCompat.hxx"

extern "C" int errno=0;

static char* messages[] = {
/*0           */    "No error",
/*EPERM       */    "Operation not permitted",
/*ENOENT      */    "No such file or directory",
/*ESRCH       */    "No such process",
/*EINTR       */    "Interrupted system call",
/*EIO         */    "I/O error",
/*ENXIO       */    "No such device or address",
/*E2BIG       */    "Arg list too long",
/*ENOEXEC     */    "Exec format error",
/*EBADF       */    "Bad file descriptor",
/*ECHILD      */    "No child processes",
/*EAGAIN      */    "Resource temporarily unavailable",
/*ENOMEM      */    "Not enough memory",
/*EACCES      */    "Permission denied",
/*EFAULT      */    "Bad address",
/*15          */    "Unknown error",						// ENOTBLK "Block device required"
/*EBUSY       */    "Device or resource busy",
/*EEXIST      */    "File exists",
/*EXDEV       */    "Improper link",						//         "Cross-device link"
/*ENODEV      */    "No such device",
/*ENOTDIR     */    "Not a directory",
/*EISDIR      */    "Is a directory",
/*EINVAL      */    "Invalid argument",
/*ENFILE      */    "Too many open files in system",
/*EMFILE      */    "Too many open files",
/*ENOTTY      */    "Inappropriate I/O control operation",	//         "Not a character device"
/*26          */    "Unknown error",						// ETXTBSY "Text file busy"
/*EFBIG       */    "File too large",
/*ENOSPC      */    "No space left on device",
/*ESPIPE      */    "Invalid seek",							//         "Illegal seek"
/*EROFS       */    "Read-only file system",
/*EMLINK      */    "Too many links",
/*EPIPE       */    "Broken pipe",
/*EDOM        */    "Domain error",							//         "Math arg out of domain of func"
/*ERANGE      */    "Result too large",						//         "Math result out of range"
/*35          */    "Unknown error",						// ENOMSG  "No message of desired type"
/*EDEADLK     */    "Resource deadlock avoided",			// EIDRM   "Identifier removed"
/*37          */    "Unknown error",						// ECHRNG  "Channel number out of range"
/*ENAMETOOLONG*/    "Filename too long",
/*ENOLCK      */    "No locks available",
/*ENOSYS      */    "Function not implemented",
/*ENOTEMPTY   */    "Directory not empty",
/*EILSEQ      */    "Illegal byte sequence"
};
static const int NUM_MESSAGES = sizeof(messages)/sizeof(messages[0]);

extern "C" char* __cdecl strerror(int errnum)
{
	if (errnum < NUM_MESSAGES)
		return messages[errnum];
	return "Unknown error";
}

//.dcm. wincece lacks this function(actually no time.h)--could port from BSD
// size_t __cdecl strftime (
//         char *string,
//         size_t maxsize,
//         const char *format,
//         const struct tm *timeptr
//         )
// {
//         return (_Strftime(string, maxsize, format, timeptr, 0));
// }
  
namespace resip
{
//	*******************************************************************************

wchar_t* ToWString(const char *str)
{
	if (!str) return 0;

	int dCharacters = MultiByteToWideChar( CP_UTF8, 0, str,strlen(str)+1, 0,0);
	wchar_t *wszStr = new wchar_t[dCharacters+1];          
	MultiByteToWideChar( CP_UTF8, 0, str,-1, wszStr,dCharacters);
	return wszStr;
};

char* FromWString(const wchar_t *wstr)
{
	if (!wstr) return 0;

	int dCharacters = WideCharToMultiByte( CP_UTF8, 0, wstr,lstrlen(wstr)+1, 0,0,0,0 );
	char *str = new char[dCharacters +1];
	WideCharToMultiByte( CP_UTF8, 0, wstr,-1, str,dCharacters ,0,0 );
	return str;
};

void FreeWString(wchar_t* wstr)
{
	if (wstr)
		delete wstr;
}

void FreeString(char* str)
{
	if (str)
		delete str;
}

}
