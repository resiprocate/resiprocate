#include "sip2/sipstack/SipFrag.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<SipFrag> SipFrag::Factory;

SipFrag::SipFrag()
   : mMessage(new SipMessage())
{}

SipFrag::SipFrag(HeaderFieldValue* hfv)
   : Contents(hfv),
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
SipFrag::getType() const
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

   using namespace PreparseConst;
    
   mMessage = new SipMessage();

   Preparse::Status status = stNone;
   
   Preparse pre;

   char* buffer = const_cast<char*>(pb.position());
   size_t size = pb.end() - pb.position();

   size_t used = 0;
   size_t discard = 0;
   
   // !dlb! fragment not required to CRLF terminate
   // need another interface to preparse?
   pre.process(*mMessage, buffer, size, 0, used, discard, status);
   DebugLog("SipFrag::parse status: " << status);
   if (status & stPreparseError ||
       used > size)
   {
      DebugLog("SipFrag::parse status: " << status);
      pb.fail("SipFrag parse failure");
   }
   else 
   {
      if (status & stHeadersComplete &&
          mMessage->exists(h_ContentLength))
      {
         assert(used == discard);
         mMessage->setBody(buffer+used,size-used);
      }
      else
      {
         if (mMessage->exists(h_ContentLength))
         {
            pb.reset(buffer + used);
            pb.skipChars(Symbols::CRLF);
            mMessage->setBody(pb.position(),pb.end()-pb.position());
         }
      }
      pb.reset(pb.end());
   }
}
