#include "resiprocate/AresDns.hxx"
#include "resiprocate/ExternalDnsFactory.hxx"


using namespace resip;

ExternalDns* 
ExternalDnsFactory::createExternalDns()
{
   return new AresDns();
}

