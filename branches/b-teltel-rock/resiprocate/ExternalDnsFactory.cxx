#include "resiprocate/AresDns.hxx"
#include "resiprocate/ExternalDnsFactory.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"


using namespace resip;

ExternalDns* 
ExternalDnsFactory::createExternalDns()
{
   return new AresDns();
}

