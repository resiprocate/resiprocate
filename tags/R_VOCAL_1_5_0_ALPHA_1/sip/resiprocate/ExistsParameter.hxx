#ifndef ExistsParameter_hxx
#define ExistsParameter_hxx

#include <sipstack/Parameter.hxx>
#include <iostream>

namespace Vocal2
{

class ParseBuffer;

class ExistsParameter : public Parameter
{
   public:
      typedef bool Type;
      
      ExistsParameter(ParameterTypes::Type, ParseBuffer& pb);
      ExistsParameter(ParameterTypes::Type type);

      bool& value();

      static Parameter* decode(ParameterTypes::Type type, ParseBuffer& pb)
      {
         return new ExistsParameter(type, pb);
      }

      virtual Parameter* clone() const;
      virtual std::ostream& encode(std::ostream& stream) const;

   private:
      bool mValue;
};

}

#endif
