#include <memory>
#include <iostream>

#include "TestSupport.hxx"

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Aor.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Debug;
   Log::initialize(Log::Cerr, l, argv[0]);
   
   {
      Aor aor("sip:speedy_AT_home.com@whistler.gloo.net:5062");
      assert(aor.scheme() == "sip");
      assert(aor.user() == "speedy_AT_home.com");
      assert(aor.port() = 5062);
      assert(aor.host() == "whistler.gloo.net");
      std::cerr << "aor.value() = " << aor.value() << std::endl;
      assert(aor.value() == "sip:speedy_AT_home.com@whistler.gloo.net:5062");
   }

   {
      Uri uri("sip:speedy_AT_home.com@whistler.gloo.net:5062");
      Aor aor(uri);
      assert(aor.scheme() == "sip");
      assert(aor.user() == "speedy_AT_home.com");
      assert(aor.port() = 5062);
      assert(aor.host() == "whistler.gloo.net");
      std::cerr << "aor.value() = " << aor.value() << std::endl;
      assert(aor.value() == "sip:speedy_AT_home.com@whistler.gloo.net:5062");
   }
   
   {
      Aor aor;
   }
}
