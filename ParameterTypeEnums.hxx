#ifndef ParameterTypeEnums_hxx
#define ParameterTypeEnums_hxx

#include <util/Data.hxx>

namespace Vocal2
{

class Parameter;
class ParseBuffer;

class ParameterTypes
{
  
   public:
      // When you add something to this enum Type, you must add an entry to
      // Parameter::make
      
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

      // convert to enum from two pointers into the HFV raw buffer
      static Type getType(const char* start, unsigned int length);

      typedef Parameter* (*Factory)(ParameterTypes::Type, ParseBuffer&);

      static Factory ParameterFactories[MAX_PARAMETER];
      static Data ParameterNames[MAX_PARAMETER];
};
 
}

#endif
