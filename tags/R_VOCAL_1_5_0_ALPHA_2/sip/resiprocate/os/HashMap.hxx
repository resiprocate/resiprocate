#ifndef Vocal2_HashMap_hxx
#define Vocal2_HashMap_hxx

#if !defined(__SUNPRO_CC)
#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#include <ext/hash_map>
#else 
#include <map>
#endif

#if ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
using __gnu_cxx::hash_map;
#else
#  ifdef WIN32
     using std::map;
#  else
     using std::hash_map;
#  endif
#endif
#else
#include <map>
#endif

#endif
