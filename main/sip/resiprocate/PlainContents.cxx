#include "resiprocate/PlainContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<PlainContents> PlainContents::Factory;
const PlainContents PlainContents::Empty;

PlainContents::PlainContents()
   : Contents(getStaticType()),
     mText()
{}

PlainContents::PlainContents(const Data& txt)
   : Contents(getStaticType()),
     mText(txt)
{}

PlainContents::PlainContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mText()
{
}
 
PlainContents::PlainContents(const Data& txt, const Mime& contentsType)
   : Contents(contentsType),
     mText(txt)
{
}

PlainContents::PlainContents(const PlainContents& rhs)
   : Contents(rhs),
     mText(rhs.mText)
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
PlainContents::getStaticType() 
{
   static Mime type("text","plain");
   return type;
}

std::ostream& 
PlainContents::encodeParsed(std::ostream& str) const
{
   //DebugLog(<< "PlainContents::encodeParsed " << mText);
   str << mText;
   return str;
}

void 
PlainContents::parse(ParseBuffer& pb)
{
   //DebugLog(<< "PlainContents::parse: " << pb.position());

   const char* anchor = pb.position();
   pb.skipToEnd();
   pb.data(mText, anchor);

   //DebugLog("PlainContents::parsed <" << mText << ">" );
}


Data
PlainContents::getBodyData() const
{
   checkParsed(); 
   return mText;
}
