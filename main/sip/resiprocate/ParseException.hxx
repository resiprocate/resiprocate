#ifndef ParseException_hxx
#define ParseException_hxx

#include <util/VException.hxx>

namespace Vocal2
{

class ParseException : public VException
{
   public:
      ParseException(const Data& msg, const Data& file, const int line)
         : VException(msg, file, line) {}
      Data getName() const { return "ParseException"; }
};
 
}

#endif
