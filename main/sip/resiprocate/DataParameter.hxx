#ifndef DataParameter_hxx
#define DataParameter_hxx

#include <sipstack/Parameter.hxx>
#include <sipstack/ParameterTypeEnums.hxx>
#include <util/Data.hxx>
#include <iostream>

namespace Vocal2
{

class ParseBuffer;

class DataParameter : public Parameter
{
   public:
      typedef Data Type;

      DataParameter(ParameterTypes::Type, ParseBuffer& pb);
      DataParameter(ParameterTypes::Type);

      Data& value();            // does not return a quoted string
      bool isQuoted() const { return mQuoted; }
      void setQuoted(bool b) { mQuoted = b; }; // this parameter will be enclosed in quotes e.g. "foo"

      static Parameter* decode(ParameterTypes::Type type, ParseBuffer& pb)
      {
         return new DataParameter(type, pb);
      }
      
      virtual Parameter* clone() const;
      virtual std::ostream& encode(std::ostream& stream) const;
      
   protected:
      DataParameter(const DataParameter& other) 
         : Parameter(other), 
           mValue(other.mValue), 
           mQuoted(other.mQuoted)
      {
      }

   private:

      Data mValue;
      bool mQuoted;
};
 
}

#endif
