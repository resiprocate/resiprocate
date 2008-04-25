#ifndef BooleanFeatureParameter_Include_Guard
#define BooleanFeatureParameter_Include_Guard

#include "resip/stack/Parameter.hxx"

#include "resip/stack/ParameterTypeEnums.hxx"

#include <ostream>

namespace resip
{
class BooleanFeatureParameter : public Parameter
{
   public:
      typedef bool Type;
      BooleanFeatureParameter(ParameterTypes::Type, 
                              ParseBuffer& pb, 
                              const char* terminators);
      explicit BooleanFeatureParameter(ParameterTypes::Type);
      BooleanFeatureParameter(const BooleanFeatureParameter& orig);
      virtual ~BooleanFeatureParameter();

      static Parameter* decode(ParameterTypes::Type type, 
                                 ParseBuffer& pb, 
                                 const char* terminators)
      {
         return new BooleanFeatureParameter(type, pb, terminators);
      }

/////////////////// Must implement unless abstract ///

      virtual Parameter* clone() const ;
      virtual std::ostream& encode(std::ostream& stream) const ;


/////////////////// May override ///

      virtual bool isQuoted() const {return !mValue;}
      Type& value() {return mValue;}

   private:
      bool mValue;
}; // class BooleanFeatureParameter

} // namespace resip

#endif // include guard
