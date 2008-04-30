#include "resip/stack/QuotedListParameter.hxx"

#include "resip/stack/Symbols.hxx"

#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/ParseException.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

namespace resip
{

QuotedListParameter::QuotedListParameter(ParameterTypes::Type type, 
                                    ParseBuffer& pb, 
                                    const char* terminators) :
   Parameter(type)
{
   pb.skipWhitespace();
   pb.skipChar('=');
   pb.skipWhitespace();
   if(*pb.position() != '\"')
   {
      throw ParseException("Expected '\"', got " + *pb.position(), 
                           "QuotedListParameter", 
                           __FILE__, 
                           __LINE__);
   }

   do
   {
      pb.skipChar(); // Skips over the '"' for the first value, and ',' after.
      const char* start = pb.position();
      pb.skipToOneOf(",\"");
      if(pb.position()==start)
      {
         throw ParseException("Zero-length entry in list", 
                              "QuotedListParameter", 
                              __FILE__, 
                              __LINE__);
      }
      // ?bwc? Do unescaping here?
      mValues.push_back( pb.data(start) );
   }
   while(*pb.position() != Symbols::DOUBLE_QUOTE[0]);

   pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
   // ?bwc? Should we fail if the next char is not a ';' or eof?
}

QuotedListParameter::QuotedListParameter(ParameterTypes::Type type) :
   Parameter(type)
{}

QuotedListParameter::QuotedListParameter(const QuotedListParameter& orig) :
   Parameter(orig),
   mValues(orig.mValues)
{}

QuotedListParameter::~QuotedListParameter()
{}

Parameter* 
QuotedListParameter::clone() const 
{
   return new QuotedListParameter(*this);
}

std::ostream& 
QuotedListParameter::encode(std::ostream& stream) const 
{
   if(mValues.empty())
   {
      ErrLog(<< "Accessing defaulted QuotedListParameter: '" 
               << getName() << "'");
      // ?bwc? Bail? Or just encode ""?
   }

   stream << getName() << Symbols::EQUALS << Symbols::DOUBLE_QUOTE;
   bool first=true;
   for(Values::const_iterator i = mValues.begin(); i!=mValues.end(); ++i)
   {
      if(!first)
      {
         stream << Symbols::COMMA;
      }
      // !bwc! Probably should do some sort of escaping here...
      stream << *i;
      first=false;
   }
   return stream << Symbols::DOUBLE_QUOTE;
}

} // of namespace resip
