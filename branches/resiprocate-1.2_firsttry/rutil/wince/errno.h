//	#include "../WceCompat.hxx"

#ifdef __cplusplus
extern "C" int errno;
extern "C" char * __cdecl strerror(int errnum );
#else
int errno;
char * __cdecl strerror(int errnum );
#endif

#define strnicmp(dest,src,size_t) _strnicmp(dest,src,size_t)
#define stricmp(dest,src) _stricmp(dest,src)
#define strdup _strdup

#if !defined(TLS_OUT_OF_INDEXES)
#define TLS_OUT_OF_INDEXES ((DWORD)0xFFFFFFFF)
#endif

#define EPERM           1
#define ENOENT          2
#define ESRCH           3
#define EINTR           4
#define EIO             5
#define ENXIO           6
#define E2BIG           7
#define ENOEXEC         8
#define EBADF           9
#define ECHILD          10
#define EAGAIN          11
#define ENOMEM          12
#define EACCES          13
#define EFAULT          14
#define EBUSY           16
#define EEXIST          17
#define EXDEV           18
#define ENODEV          19
#define ENOTDIR         20
#define EISDIR          21
#define EINVAL          22
#define ENFILE          23
#define EMFILE          24
#define ENOTTY          25
#define EFBIG           27
#define ENOSPC          28
#define ESPIPE          29
#define EROFS           30
#define EMLINK          31
#define EPIPE           32
#define EDOM            33
#define ERANGE          34
#define EDEADLK         36
#define ENAMETOOLONG    38
#define ENOLCK          39
#define ENOSYS          40
#define ENOTEMPTY       41
#define EILSEQ          42

