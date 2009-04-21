#ifndef _POPT_DLL_IMPEXP_H_
#define _POPT_DLL_IMPEXP_H_ 1

#ifndef __GNUC__
# define __DLL_IMPORT__	__declspec(dllimport)
# define __DLL_EXPORT__	__declspec(dllexport)
#else
# define __DLL_IMPORT__	__attribute__((dllimport)) extern
# define __DLL_EXPORT__	__attribute__((dllexport)) extern
#endif 

#if defined(__WIN32__) || defined(_WIN32)
# ifdef BUILD_POPT_DLL
#  define POPT_DLL_IMPEXP	__DLL_EXPORT__
# elif defined(POPT_STATIC)
#  define POPT_DLL_IMPEXP extern	 
# elif defined (USE_POPT_DLL)
#  define POPT_DLL_IMPEXP	__DLL_IMPORT__
# else /* assume USE_POPT_DLL */
#  define POPT_DLL_IMPEXP	__DLL_IMPORT__
# endif
#else /* __WIN32__ */
# define POPT_DLL_IMPEXP	 
#endif

#endif /* _POPTDLLIMPEXP_H_ */
