#if !defined(resip_HashMap_hxx)
#define resip_HashMap_hxx

// !cj! if all these machine have to use map then we can just delete them and use the default 

#  if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#    include <ext/hash_map>
#    define HASH_MAP_NAMESPACE __gnu_cxx
#    define HashMap __gnu_cxx::hash_map
#    define HashSet __gnu_cxx::hash_set
#  elif  defined(__INTEL_COMPILER )
#    include <hash_map>
#    define HASH_MAP_NAMESPACE std
#    define HashMap std::hash_map
#    define HashSet std::hash_set
#  else
#    include <map>
#    define HashMap std::map
#    define HashSet std::set
#  endif

#endif
