#ifndef Message_hxx
#define Message_hxx

#include <sipstack/Data.hxx>

namespace Vocal2
{

class Message 
{
   public:
      virtual ~Message(){}
      virtual const Data& getTransactionId() const=0;
      virtual Data brief() const=0;
      virtual ostream& dump(ostream& strm) const=0;
};


}

#endif
