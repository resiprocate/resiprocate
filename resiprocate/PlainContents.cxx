#include "sip2/sipstack/PlainContents.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<PlainContents> PlainContents::Factory;

PlainContents::PlainContents()
   : mText()
{}

PlainContents::PlainContents(HeaderFieldValue* hfv)
   : Contents(hfv),
     mText()
{
}
 
PlainContents::PlainContents(const PlainContents& rhs)
   : Contents(rhs),
     mText( rhs.mText )
{
}

PlainContents::~PlainContents()
{
}

PlainContents&
PlainContents::operator=(const PlainContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mText = rhs.mText;
   }
   return *this;
}

Contents* 
PlainContents::clone() const
{
   return new PlainContents(*this);
}

const Mime& 
PlainContents::getType() const
{
   static Mime type("text","plain");
   return type;
}

std::ostream& 
PlainContents::encodeParsed(std::ostream& str) const
{
   DebugLog(<< "PlainContents::encode " << mText);
   str << mText;
   str << Symbols::CRLF;
   return str;
}

void 
PlainContents::parse(ParseBuffer& pb)
{
   DebugLog(<< "PlainContents::parse: " << pb.position());

   char* buffer = const_cast<char*>(pb.position());
   size_t size = pb.end() - pb.position();

   Data text(buffer,size);
   DebugLog("PlainContents::parsed <" << text << ">" );

   mText = text;
   
   pb.reset(pb.end());
}
