#include "resip/stack/ReceivedParameter.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::SIP

ReceivedParameter::ReceivedParameter(ParameterTypes::Type type,
                                     ParseBuffer& pb,
                                     const std::bitset<256>& terminators) :
   DataParameter(type)
{
   pb.skipWhitespace();

   if (!pb.eof() && *pb.position() == Symbols::EQUALS[0])
   {
      // Catch the exception gracefully to handle seeing ";received="
      try
      {
         pb.skipChar();
         pb.skipWhitespace();

         // Handle cases such as ";received="
         if(terminators[(unsigned char)(*pb.position())])
         {
            ErrLog(<< "Empty value in string-type parameter.");
            return;
         }

         const char* pos = pb.position();
         pb.skipToOneOf(terminators);
         pb.data(mValue, pos);
      }
      catch (const BaseException& e)
      {
         ErrLog(<< "Caught exception: " << e);
      }
   }

   // The value of p_received will be filled if it is not empty.
   // Otherwise, no exceptions as the empty parameter is allowed here.
}

ReceivedParameter::ReceivedParameter(ParameterTypes::Type type) :
   DataParameter(type)
{
}

Parameter* ReceivedParameter::decode(ParameterTypes::Type type,
                                     ParseBuffer& pb,
                                     const std::bitset<256>& terminators,
                                     PoolBase* pool)
{
   return new (pool) ReceivedParameter(type, pb, terminators);
}

Parameter* ReceivedParameter::clone() const
{
   return new ReceivedParameter(*this);
}

EncodeStream&
ReceivedParameter::encode(EncodeStream& stream) const
{
   if (!mValue.empty())
   {
      return stream << getName() << Symbols::EQUALS << mValue;
   }
   else
   {
      return stream << getName();
   }
}
