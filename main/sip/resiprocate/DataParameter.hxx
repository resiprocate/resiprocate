#ifndef DataParameter_hxx
#define DataParameter_hxx

#include <sipstack/Parameter.hxx>
#include <sipstack/ParameterTypeEnums.hxx>
#include <util/Data.hxx>
#include <iostream>

namespace Vocal2
{

class DataParameter : public Parameter
{
   public:
      typedef Data Type;
      
      DataParameter(ParameterTypes::Type, const char* startData, unsigned int dataSize);
      DataParameter(ParameterTypes::Type);
      
      Data& value();            // does not return a quoted string
      bool isQuoted() const { return mQuoted; }
      void setQuoted(bool b) { mQuoted = b; }; // this parameter will be enclosed in quotes e.g. "foo"

      virtual Parameter* clone() const;
      virtual std::ostream& encode(std::ostream& stream) const;
      
   private:
      Data mData;
      bool mQuoted;
};
 
}

#endif
