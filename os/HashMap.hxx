#ifndef Vocal2_HashMap_hxx
#define Vocal2_HashMap_hxx

// !cj! if all these machine have to use map then we can just delte them and use the default 

#  if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#    include <ext/hash_map>
     using __gnu_cxx::hash_map;
//#  elif  defined(__INTEL_COMPILER )
//#    include <map>
//#    define hash_map map
//     using std::map;
//#  elif  defined(__SUNPRO_CC)
//#    include <map>
//#    define hash_map map
//     using std::map;
//#  elif  defined(WIN32)
//#    include <map>
//#    define hash_map map
//     using std::map;
#  else
#    include <map>
#    define hash_map map
     using std::map;
#  endif

// !cj! - bellow is old stuff and shoudl be delted ....

//#if !defined(__SUNPRO_CC)
//#  if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
//#    include <ext/hash_map>
//#  elif  defined(__INTEL_COMPILER )
//#    include <map>
//#  else
//#    include <map>
//#  endif
//
//#  if ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
//using __gnu_cxx::hash_map;
//#  else
//#    if defined(WIN32)
//       using std::map;
//#    else
//       using std::hash_map;
//#    endif
//#  endif
//#  else
//#include <map>
//#define hash_map map
//using std::map;
//#  endif

#endif
