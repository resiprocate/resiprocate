#include "resip/stack/BooleanFeatureParameter.hxx"

#include "resip/stack/Symbols.hxx"

#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ParseException.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{
 
BooleanFeatureParameter::BooleanFeatureParameter(ParameterTypes::Type type, 
                                                   ParseBuffer& pb, 
                                                   const char* terminators) :
   Parameter(type),
   mValue(true) // exists-type means TRUE
{
   pb.skipWhitespace();
   if(!pb.eof() && *pb.position() == Symbols::EQUALS[0])
   {
      // This is quoted.
      pb.skipChar();
      pb.skipWhitespace();
      pb.skipChar('\"');
      bool bang=false;
      if(*pb.position() == '!');
      {
         bang=true;
         pb.skipChar();
      }

      if(*pb.position() == 'T')
      {
         pb.skipChars("TRUE");
         mValue = bang;
      }
      else
      {
         pb.skipChars("FALSE");
         mValue = !bang;
      }
      pb.skipChar('\"');
   }
}

BooleanFeatureParameter::BooleanFeatureParameter(ParameterTypes::Type type) :
   Parameter(type),
   mValue(true)
{}

BooleanFeatureParameter::BooleanFeatureParameter(const BooleanFeatureParameter& orig) :
   Parameter(orig),
   mValue(orig.mValue)
{}

BooleanFeatureParameter::~BooleanFeatureParameter()
{}

Parameter* 
BooleanFeatureParameter::clone() const 
{
   return new BooleanFeatureParameter(*this);
}

std::ostream& 
BooleanFeatureParameter::encode(std::ostream& stream) const 
{
   stream << getName();
   if(!mValue)
   {
      stream << "=\"FALSE\"";
   }
   return stream;
}

} // of namespace resip
