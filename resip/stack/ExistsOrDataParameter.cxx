#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include "rutil/ParseException.hxx"
#include "resip/stack/ExistsOrDataParameter.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

namespace resip
{

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


ExistsOrDataParameter::ExistsOrDataParameter(ParameterTypes::Type type, bool) : 
   DataParameter(type)
{
}

ExistsOrDataParameter::ExistsOrDataParameter(ParameterTypes::Type type,
                                              ParseBuffer& pb, 
                                              const std::bitset<256>& terminators)
   : DataParameter(type, pb, terminators)
{
}

ExistsOrDataParameter::ExistsOrDataParameter(ParameterTypes::Type type)
   : DataParameter(type)
{
}

Parameter* 
ExistsOrDataParameter::decode(ParameterTypes::Type type, 
                              ParseBuffer& pb, 
                              const std::bitset<256>& terminators,
                              PoolBase* pool)
{
   //pb.skipWhitespace();  // whitespace may be a terminator - don't skip it
   if (pb.eof() || terminators[*pb.position()])
   {
      return new (pool) ExistsOrDataParameter(type);
   }
   else
   {
      return new (pool) ExistsOrDataParameter(type, pb, terminators);
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
