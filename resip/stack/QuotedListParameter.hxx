#ifndef QuotedListParameter_Include_Guard
#define QuotedListParameter_Include_Guard

#include "resip/stack/Parameter.hxx"

#include "resip/stack/ParameterTypeEnums.hxx"

#include "rutil/Data.hxx"

#include <list>
#include <ostream>

namespace resip
{
class ParseBuffer;

class QuotedListParameter : public Parameter
{
   public:
      typedef std::vector<Data> Values;
      typedef Values Type;

      QuotedListParameter(ParameterTypes::Type, 
                        ParseBuffer& pb, 
                        const char* terminators);
      explicit QuotedListParameter(ParameterTypes::Type);
      QuotedListParameter(const QuotedListParameter& orig);

      virtual ~QuotedListParameter();

      static Parameter* decode(ParameterTypes::Type type, 
                                 ParseBuffer& pb, 
                                 const char* terminators)
      {
         return new QuotedListParameter(type, pb, terminators);
      }

/////////////////// Must implement unless abstract ///
      virtual Parameter* clone() const ;
      virtual std::ostream& encode(std::ostream& stream) const ;


/////////////////// May override ///
      virtual bool isQuoted() const {return true;}
      Type& value() {return mValues;}

   private:
      Values mValues;

}; // class QuotedListParameter

} // namespace resip

#endif // include guard
