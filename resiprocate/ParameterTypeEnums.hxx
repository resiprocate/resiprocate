#ifndef ParameterTypeEnums_hxx
#define ParameterTypeEnums_hxx

#include <sipstack/Data.hxx>

namespace Vocal2
{

class ParameterTypes
{
  
   public:
      enum Type
      {
         transport,
         user,
         method,
         ttl,
         maddr,
         lr,
         q,
         purpose,
         expires,
         handling,
         tag,
         toTag,
         fromTag,
         duration,
         branch,
         received,

         comp,
         rport,
         UNKNOWN,
         MAX_PARAMETER
      };

      static Data ParameterNames[MAX_PARAMETER];
};
 
}

#endif
