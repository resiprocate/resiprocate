#if !defined(Vocal2_HashMap_hxx)
#define Vocal2_HashMap_hxx

// !cj! if all these machine have to use map then we can just delete them and use the default 

#  if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#    include <ext/hash_map>
#    define HASH_MAP_NAMESPACE __gnu_cxx
#    define HashMap __gnu_cxx::hash_map
#  elif  defined(__INTEL_COMPILER )
#    include <hash_map>
#    define HASH_MAP_NAMESPACE std
#    define HashMap std::hash_map
#  else
#    include <map>
#    define HashMap std::map
#  endif

#endif
