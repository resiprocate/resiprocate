#include <iostream>

#include "TestSupport.hxx"
#include "resiprocate/ParseUtil.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::DEBUG;
   Log::initialize(Log::COUT, l, argv[0]);
   
   {
      Data addr("1:1");
      cerr << "!! "<< addr << endl;
      assert(ParseUtil::isIpV6Address(addr));
   }

   {
      Data addr("1:1:192.168.2.233");
      cerr << "!! "<< addr << endl;
      assert(ParseUtil::isIpV6Address(addr));
   }

   {
      Data addr("1:1:::::");
      cerr << "!! "<< addr << endl;
      assert(ParseUtil::isIpV6Address(addr));
   }

   {
      Data addr("1:1::::::168.192.2.233");
      cerr << "!! "<< addr << endl;
      assert(ParseUtil::isIpV6Address(addr));
   }

   {
      Data addr("5f1b:df00:ce3e:e200:20:800:2b37:6426");
      cerr << "!! "<< addr << endl;
      assert(ParseUtil::isIpV6Address(addr));
   }

   {
      Data addr("5f1b:df00:ce3e:e200:20:800:2b37:6426:121.12.131.12");
      cerr << "!! "<< addr << endl;
      assert(ParseUtil::isIpV6Address(addr));
   }
   
   {
      Data addr("192.168.2.233");
      cerr << "!! "<< addr << endl;
      assert(!ParseUtil::isIpV6Address(addr));      
   }

   {
      Data addr("u@a.tv:1290");
      cerr << "!! "<< addr << endl;
      assert(!ParseUtil::isIpV6Address(addr));      
   }

   cerr << "All OK" << endl;
}
