#ifndef HashMap_hxx
#define HashMap_hxx


#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#include <ext/hash_map>
#else 
#include <map>
#endif

#if ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
using __gnu_cxx::hash_map;
#else
using std::hash_map;
#endif


#endif
