#include "resiprocate/OctetContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP


ContentsFactory<OctetContents> OctetContents::Factory;
const OctetContents OctetContents::Empty;

OctetContents::OctetContents()
   : Contents(getStaticType()),
     mOctets()
{}

OctetContents::OctetContents(const Data& octets)
   : Contents(getStaticType()),
     mOctets(octets)
{}

OctetContents::OctetContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mOctets()
{
}
 
OctetContents::OctetContents(const Data& octets, const Mime& contentsType)
   : Contents(contentsType),
     mOctets(octets)
{
}

OctetContents::OctetContents(const OctetContents& rhs)
   : Contents(rhs),
     mOctets(rhs.mOctets)
{
}

OctetContents::~OctetContents()
{
}

OctetContents&
OctetContents::operator=(const OctetContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mOctets = rhs.mOctets;
   }
   return *this;
}

Contents* 
OctetContents::clone() const
{
   return new OctetContents(*this);
}

const Mime& 
OctetContents::getStaticType() 
{
   static Mime type("application","octet-stream");
   return type;
}

std::ostream& 
OctetContents::encodeParsed(std::ostream& str) const
{
   //DebugLog(<< "OctetContents::encodeParsed " << mOctets);
   str << mOctets;
   return str;
}

void 
OctetContents::parse(ParseBuffer& pb)
{
   //DebugLog(<< "OctetContents::parse: " << pb.position());

   const char* anchor = pb.position();
   pb.skipToEnd();
   pb.data(mOctets, anchor);

   //DebugLog("OctetContents::parsed <" << mOctets << ">" );
}


Data
OctetContents::getBodyData() const
{
   checkParsed(); 
   return mOctets;
}
