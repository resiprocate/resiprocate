#ifndef IntegerParameter_hxx
#define IntegerParameter_hxx

#include <sipstack/ParameterTypeEnums.hxx>
#include <sipstack/Parameter.hxx>
#include <iostream>

namespace Vocal2
{

class ParseBuffer;

class IntegerParameter : public Parameter
{
   public:
      typedef int Type;

      IntegerParameter(ParameterTypes::Type, ParseBuffer& pb);
      IntegerParameter(ParameterTypes::Type type, int value = -666999666);
      
      static Parameter* decode(ParameterTypes::Type type, ParseBuffer& pb)
      {
         return new IntegerParameter(type, pb);
      }

      int& value();
      virtual std::ostream& encode(std::ostream& stream) const;

      virtual Parameter* clone() const;
   private:
      int mValue;
};
 
}




#endif
