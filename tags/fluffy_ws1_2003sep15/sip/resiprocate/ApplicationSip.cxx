#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/ApplicationSip.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

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
