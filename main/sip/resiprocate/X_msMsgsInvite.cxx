#include "resiprocate/X_msMsgsInvite.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

ContentsFactory<X_msMsgsInvite> X_msMsgsInvite::Factory;

X_msMsgsInvite::X_msMsgsInvite()
   : Contents(getStaticType()),
     mPort(0),
     mHost()
{}

X_msMsgsInvite::X_msMsgsInvite(const Data& txt)
   : Contents(getStaticType()),
     mPort(0),
     mHost()
{}

X_msMsgsInvite::X_msMsgsInvite(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mPort(0),
     mHost()
{}
 
X_msMsgsInvite::X_msMsgsInvite(const Data& txt, const Mime& contentsType)
   : Contents(contentsType),
     mPort(0),
     mHost()
{}

X_msMsgsInvite::X_msMsgsInvite(const X_msMsgsInvite& rhs)
   : Contents(rhs),
     mPort(rhs.mPort),
     mHost(rhs.mHost)
{}

X_msMsgsInvite::~X_msMsgsInvite()
{}

X_msMsgsInvite&
X_msMsgsInvite::operator=(const X_msMsgsInvite& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      mPort = rhs.mPort;
      mHost = rhs.mHost;
   }
   return *this;
}

Contents* 
X_msMsgsInvite::clone() const
{
   return new X_msMsgsInvite(*this);
}

const Mime& 
X_msMsgsInvite::getStaticType() 
{
   // set charset ?dlb?
   static Mime type("text","x-msmsgsinvite");

   return type;
}

std::ostream& 
X_msMsgsInvite::encodeParsed(std::ostream& str) const
{
   for (Attributes::const_iterator i = mAttributes.begin();
	i != mAttributes.end(); i++)
   {
      str << i->first << ": "  << i->second << Symbols::CRLF;
   }
   if (mPort)
   {
      str << "Port: " << mPort << Symbols::CRLF;
   }
   
   if (!mHost.empty())
   {
      str << "IP-Address: " << mHost << Symbols::CRLF;
   }

   return str;
}

void 
X_msMsgsInvite::parse(ParseBuffer& pb)
{
   DebugLog(<< "X_msMsgsInvite::parse: " << pb.position());

   Data attribute;
   Data value;

   while (!pb.eof())
   {
      const char* anchor = pb.skipWhitespace(); 
      pb.skipToChar(Symbols::COLON[0]);
      pb.data(attribute, anchor);
      pb.skipChar(Symbols::COLON[0]);
      pb.skipWhitespace();

      if (attribute == "Port")
      {
	 mPort = pb.integer();
      }
      else if (attribute == "IP-Address")
      {
	 pb.skipToChars(Symbols::CRLF);
	 pb.data(mHost, anchor);
      }
      else
      {
	 pb.skipToChars(Symbols::CRLF);
	 pb.data(value, anchor);
	 mAttributes.push_back(make_pair(attribute, value));
      }
   }
}
