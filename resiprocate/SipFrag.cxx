#include "resiprocate/SipFrag.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Preparse.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

ContentsFactory<SipFrag> SipFrag::Factory;

SipFrag::SipFrag(const Mime& contentsType)
   : Contents(contentsType),
     mMessage(new SipMessage())
{}

SipFrag::SipFrag(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mMessage(0)
{
}

SipFrag::SipFrag(const SipFrag& rhs)
   : Contents(rhs),
     mMessage(rhs.mMessage ? new SipMessage(*rhs.mMessage) : 0)
{
}

SipFrag::~SipFrag()
{
   delete mMessage;
}

SipFrag&
SipFrag::operator=(const SipFrag& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      delete mMessage;
      if (rhs.mMessage)
      {
         mMessage = new SipMessage(*rhs.mMessage);
      }
      else
      {
         mMessage = 0;
      }
   }
   
   return *this;
}

Contents* 
SipFrag::clone() const
{
   return new SipFrag(*this);
}

const Mime& 
SipFrag::getStaticType() 
{
   static Mime type("message", "sipfrag");
   return type;
}

std::ostream& 
SipFrag::encodeParsed(std::ostream& str) const
{
   mMessage->encode(str);

   return str;
}

void 
SipFrag::parse(ParseBuffer& pb)
{
   DebugLog(<< "SipFrag::parse: " << pb.position());

   mMessage = new SipMessage();

   Preparse pre;

   pb.assertNotEof();
   const char* const_buffer = pb.position();
   char* buffer = const_cast<char*>(const_buffer);

   size_t size = pb.end() - pb.position();

   // !dlb! fragment not required to CRLF terminate
   // need another interface to preparse?
   // !ah! removed size check .. process() cannot process more
   // than size bytes of the message.
   if ( pre.process(*mMessage, buffer, size))
   {
     pb.fail(__FILE__, __LINE__);
   }
   else 
   {
     size_t used = pre.nBytesUsed();

     // !ah! I think this is broken .. if we are UDP then the 
     // remainder is the SigFrag, not the Content-Length... ??
     if (pre.isHeadersComplete() && 
          mMessage->exists(h_ContentLength))
      {
         assert(used == pre.nDiscardOffset());
         mMessage->setBody( buffer+used, int(size-used) );
      }
      else
      {
        // !ah! So the headers weren't complete. Why are we here?
         if (mMessage->exists(h_ContentLength))
         {
            pb.reset(buffer + used);
            pb.skipChars(Symbols::CRLF);
            mMessage->setBody(pb.position(),int(pb.end()-pb.position()) );
         }
      }
      pb.reset(pb.end());
   }
}
