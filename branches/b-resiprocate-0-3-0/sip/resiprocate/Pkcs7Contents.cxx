#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

ContentsFactory<Pkcs7Contents> Pkcs7Contents::Factory;
ContentsFactory<Pkcs7SignedContents> Pkcs7SignedContents::Factory;

Pkcs7Contents::Pkcs7Contents()
   : Contents(getStaticType()),
     mText()
{
}


Pkcs7SignedContents::Pkcs7SignedContents()
{
}

Pkcs7Contents::Pkcs7Contents(const Data& txt)
   : Contents(getStaticType()),
     mText(txt)
{
}

Pkcs7Contents::Pkcs7Contents(const Data& txt, const Mime& contentsType)
   : Contents(contentsType),
     mText(txt)
{
}
 
Pkcs7Contents::Pkcs7Contents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mText()
{
}
 
Pkcs7SignedContents::Pkcs7SignedContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Pkcs7Contents(hfv, contentsType)
{
}
 
Pkcs7Contents::Pkcs7Contents(const Pkcs7Contents& rhs)
   : Contents(rhs),
     mText(rhs.mText)
{
}

Pkcs7SignedContents::Pkcs7SignedContents(const Pkcs7SignedContents& rhs)
   : Pkcs7Contents(rhs)
{
}

Pkcs7Contents::~Pkcs7Contents()
{
}

Pkcs7SignedContents::~Pkcs7SignedContents()
{
}

Pkcs7Contents&
Pkcs7Contents::operator=(const Pkcs7Contents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mText = rhs.mText;
   }
   return *this;
}

Contents* 
Pkcs7Contents::clone() const
{
   return new Pkcs7Contents(*this);
}

Contents* 
Pkcs7SignedContents::clone() const
{
   return new Pkcs7SignedContents(*this);
}

const Mime& 
Pkcs7Contents::getStaticType() 
{
   static Mime type("application","pkcs7-mime");
   return type;
}


const Mime& 
Pkcs7SignedContents::getStaticType() 
{
   static Mime type("application","pkcs7-signature");
   return type;
}


std::ostream& 
Pkcs7Contents::encodeParsed(std::ostream& str) const
{
   //DebugLog(<< "Pkcs7Contents::encodeParsed " << mText);
   str << mText;
   return str;
}


void 
Pkcs7Contents::parse(ParseBuffer& pb)
{
   const char* anchor = pb.position();
   pb.skipToEnd();
   pb.data(mText, anchor);

   DebugLog(<< "Pkcs7Contents::parsed <" << mText.escaped() << ">" );
}


Data 
Pkcs7Contents::getBodyData() const
{
   checkParsed();
   return mText;
}
