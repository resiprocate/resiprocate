#ifndef Parameter_hxx
#define Parameter_hxx

#include <sipstack/ParameterTypeEnums.hxx>
#include <sipstack/Data.hxx>
#include <iostream>

namespace Vocal2
{

class Parameter
{
   public:
      Parameter(ParameterTypes::Type type);
      virtual ~Parameter() {}
      
      ParameterTypes::Type getType() const;

      virtual const Data& getName() const;

      virtual Parameter* clone() const = 0;
      virtual std::ostream& encode(std::ostream& stream) const = 0;

      Parameter* next;
   private:
      ParameterTypes::Type mType;

};

}

#endif
