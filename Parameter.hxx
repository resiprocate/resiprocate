#ifndef Parameter_hxx
#define Parameter_hxx

#include <util/Data.hxx>
#include <iostream>
#include <sipstack/ParameterTypeEnums.hxx>


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

      virtual bool isQuoted() const { return false; } // only on DataParameter
      virtual void setQuoted(bool b) { }; // only on DataParameter
   private:
      ParameterTypes::Type mType;
};

}

#endif
