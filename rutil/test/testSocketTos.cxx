#include <cstdlib>
#include <iostream>

#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "rutil/Logger.hxx"
#include "rutil/Socket.hxx"

using namespace std;
using namespace resip;

static int failures = 0;

static void
check(bool condition, const char* what)
{
   cerr << (condition ? "ok   " : "FAIL ") << what << endl;
   if (!condition)
   {
      ++failures;
   }
}

// Reports what the socket did, not only that it disagreed: -1 is a refused or
// failed call, and any other value is one the operating system substituted.
static void
check(int actual, int expected, const char* what)
{
   const bool ok = (actual == expected);
   cerr << (ok ? "ok   " : "FAIL ") << what << " (got " << actual << ", wanted " << expected << ")" << endl;
   if (!ok)
   {
      ++failures;
   }
}

// CS5, and the byte it occupies once the two ECN bits are left clear.
static const int CS5 = 40;
static const int CS5_BYTE = 160;

int
main()
{
   // At Stack, so that the per-socket line setSocketDscp() writes on a failure
   // reaches the test output with the error number in it. Without it a refused
   // option is reported only as a value that does not match.
   Log::initialize(Log::Cout, Log::Stack, Data::Empty);

   // Windows opens no socket before WSAStartup, and nothing here has run the
   // stack that would call it.
   initNetwork();

   Socket v4 = ::socket(AF_INET, SOCK_DGRAM, 0);
   check(v4 != INVALID_SOCKET, "opened an AF_INET socket");

   check(setSocketDscp(v4, CS5, V4), CS5, "V4: sets the class and reads it back");
   check(setSocketDscp(v4, 0, V4), 0, "V4: clears the class to 0");

   // The class is not the byte. A helper that stored its argument unshifted
   // would round-trip correctly and still mark the wrong packets, so read the
   // byte the socket actually carries rather than trusting the return alone.
   check(setSocketDscp(v4, CS5, V4), CS5, "V4: sets the class again");

   int carried = 0;
   socklen_t carriedLen = sizeof(carried);
   ::getsockopt(v4, IPPROTO_IP, IP_TOS, (char *)&carried, &carriedLen);
   check(carried, CS5_BYTE, "V4: the class reaches the socket as the byte it shifts to");

   check(getSocketTos(v4, V4), CS5_BYTE, "V4: the byte view reports the whole byte");
   check(getSocketDscp(v4, V4), CS5, "V4: the class view reports the class");

   Socket v6 = ::socket(AF_INET6, SOCK_DGRAM, 0);
   check(v6 != INVALID_SOCKET, "opened an AF_INET6 socket");

   // Windows has no IPV6_TCLASS -- setsockopt refuses it with WSAENOPROTOOPT
   // (10042) -- so none of this can be asked of a socket there. It refuses
   // IP_TOS on an AF_INET6 socket too, which is what the decoy needs.
#ifndef WIN32
   // Seed the wrong level with a different value first. setsockopt(IPPROTO_IP,
   // IP_TOS) succeeds on an AF_INET6 socket and reads back, so an
   // implementation using the IPv4 level here would return the class and look
   // correct while marking nothing on the wire.
   int decoy = 96;
   ::setsockopt(v6, IPPROTO_IP, IP_TOS, (const char *)&decoy, sizeof(decoy));

   check(setSocketDscp(v6, CS5, V6), CS5, "V6: sets the traffic class and reads it back");

   int tclass = 0;
   socklen_t tclassLen = sizeof(tclass);
   ::getsockopt(v6, IPPROTO_IPV6, IPV6_TCLASS, (char *)&tclass, &tclassLen);
   check(tclass, CS5_BYTE, "V6: wrote IPV6_TCLASS");

   int strayTos = 0;
   socklen_t strayTosLen = sizeof(strayTos);
   ::getsockopt(v6, IPPROTO_IP, IP_TOS, (char *)&strayTos, &strayTosLen);
   check(strayTos == decoy, "V6: left the IPv4 level untouched");
#endif

   // Out of range is refused rather than shifted into a neighbouring class.
   check(setSocketDscp(v4, 64, V4) == -1, "V4: a class above 63 is refused");
   check(setSocketDscp(v6, 64, V6) == -1, "V6: a class above 63 is refused");
   check(setSocketDscp(v4, -2, V4) == -1, "a negative class is refused");

   // Refusing writes nothing: the class set before it is still on the socket.
   check(getSocketDscp(v4, V4), CS5, "a refused class leaves the socket as it was");

   // A socket that cannot carry the option must be reported, not thrown: the
   // caller keeps serving traffic.
   bool threw = false;
   int unusable = 0;
   try
   {
      unusable = setSocketDscp(INVALID_SOCKET, CS5, V4);
   }
   catch (...)
   {
      threw = true;
   }
   check(!threw, "an unusable descriptor does not throw");
   check(unusable == -1, "an unusable descriptor reports failure");

   closeSocket(v4);
   closeSocket(v6);

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
}
