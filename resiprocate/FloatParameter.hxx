#ifndef FloatParameter_hxx
#define FloatParameter_hxx

#include <sipstack/Parameter.hxx>
#include <sipstack/ParameterTypeEnums.hxx>
#include <iostream>

namespace Vocal2
{

class ParseBuffer;

class FloatParameter : public Parameter
{
   public:
      typedef float Type;
      
      FloatParameter(ParameterTypes::Type, ParseBuffer& pb);
      FloatParameter(ParameterTypes::Type type, float value);

      float& value();

      static Parameter* decode(ParameterTypes::Type type, ParseBuffer& pb)
      {
         return new FloatParameter(type, pb);
      }

      virtual Parameter* clone() const;
      virtual std::ostream& encode(std::ostream& stream) const;

   private:
      float mValue;
};

}

#endif
