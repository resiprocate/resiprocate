#include "resiprocate/sipstack/ApplicationSip.hxx"
#include "resiprocate/util/Logger.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<ApplicationSip> ApplicationSip::Factory;

ApplicationSip::ApplicationSip(const Mime& contentsType)
   : SipFrag(contentsType)
{}

ApplicationSip::ApplicationSip(HeaderFieldValue* hfv, const Mime& contentsType)
   : SipFrag(hfv, contentsType)
{
}

ApplicationSip::ApplicationSip(const ApplicationSip& rhs)
   : SipFrag(rhs)
{
}

ApplicationSip&
ApplicationSip::operator=(const ApplicationSip& rhs)
{
    SipFrag::operator=(rhs);
    return *this;
}

Contents* 
ApplicationSip::clone() const
{
   return new ApplicationSip(*this);
}

const Mime& 
ApplicationSip::getStaticType() 
{
   static Mime type("application", "sip");
   return type;
}
