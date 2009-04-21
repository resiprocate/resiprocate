#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include <cassert>
#include "rutil/ParseException.hxx"
#include "resip/stack/ExistsOrDataParameter.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

namespace resip
{

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


ExistsOrDataParameter::ExistsOrDataParameter(ParameterTypes::Type type, bool) : 
   DataParameter(type)
{
}

ExistsOrDataParameter::ExistsOrDataParameter(ParameterTypes::Type type,
                                                 ParseBuffer& pb,
                                                 const char* terminators)
   : DataParameter(type, pb, terminators)
{
}

ExistsOrDataParameter::ExistsOrDataParameter(ParameterTypes::Type type)
   : DataParameter(type)
{
}

// !dcm! stole from parsebuffer.cxx -- clean up, did't want to recomplie the world
bool oneOf2(char c, const Data& cs)
{
   for (Data::size_type i = 0; i < cs.size(); i++)
   {
      if (c == cs[i])
      {
         return true;
      }
   }
   return false;
}   

Parameter* 
ExistsOrDataParameter::decode(ParameterTypes::Type type, ParseBuffer& pb, const char* terminators)
{
   pb.skipWhitespace();
   if (pb.eof() || oneOf2(*pb.position(), terminators))
   {
      return new ExistsOrDataParameter(type);
   }
   else
   {
      return new ExistsOrDataParameter(type, pb, terminators);
   }
}

Parameter* 
ExistsOrDataParameter::clone() const
{
   return new ExistsOrDataParameter(*this);
}

EncodeStream& 
ExistsOrDataParameter::encode(EncodeStream& stream) const
{
   if (mValue.empty())
   {
      return stream << getName();
   }
   else
   {
      return DataParameter::encode(stream);
   }
}

} // namespace resip
