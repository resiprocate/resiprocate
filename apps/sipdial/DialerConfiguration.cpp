
#include <iostream>
#include <string>

#include "rutil/Data.hxx"

#include "DialerConfiguration.hxx"


using namespace resip;
using namespace std;

DialerConfiguration::DialerConfiguration()
{
}

DialerConfiguration::~DialerConfiguration()
{
}

void DialerConfiguration::loadStream(std::istream& in)
{
   while(!in.eof())
   {
      string param;
      string value;
      in >> param >> value;
      cerr << "param = " << param << " and value = " << value << endl;
      if(param == string("dialerIdentity"))
         setDialerIdentity(NameAddr(Uri(Data(value))));
      else if(param == string("authRealm"))
         setAuthRealm(Data(value));
      else if(param == string("authUser"))
         setAuthUser(Data(value));
      else if(param == string("authPassword"))
         setAuthPassword(Data(value));
      else if(param == string("callerUserAgentAddress"))
         setCallerUserAgentAddress(Uri(Data(value)));
      else if(param == string("callerUserAgentVariety"))
      {
         if(value == string("LinksysSPA941"))
            setCallerUserAgentVariety(LinksysSPA941);
         else if(value == string("PolycomIP501"))
            setCallerUserAgentVariety(PolycomIP501);
         else if(value == string("Cisco7940"))
            setCallerUserAgentVariety(Cisco7940);
         else if(value == string("Generic"))
            setCallerUserAgentVariety(Generic);
         else
            assert(0);   // FIXME
      }
      else if(param == string("targetPrefix"))
         setTargetPrefix(Data(value));
      else if(param == string("targetDomain"))
         setTargetDomain(Data(value)); 
      else if(param == string(""))
         // skip empty lines
         { }
      else
         assert(0); // FIXME
   }
}


