#ifndef FloatParameter_hxx
#define FloatParameter_hxx

#include <sipstack/Parameter.hxx>
#include <sipstack/ParameterTypeEnums.hxx>
#include <iostream>

namespace Vocal2
{

class FloatParameter : public Parameter
{
   public:
      typedef float Type;
      
      FloatParameter(ParameterTypes::Type, const char* startData, unsigned int dataSize);
      FloatParameter(ParameterTypes::Type type, float value);

      float& value();
      virtual Parameter* clone() const;
      virtual std::ostream& encode(std::ostream& stream) const;

   private:
      float mValue;
};

}

#endif
