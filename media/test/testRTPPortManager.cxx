#include <rutil/Logger.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Socket.hxx>
#include <media/RTPPortManager.hxx>

#include <cstring>
#include <iostream>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

static int failures = 0;

static void
check(const char* what, bool ok)
{
   cerr << (ok ? "   ok: " : "   FAILED: ") << what << endl;
   if (!ok)
   {
      ++failures;
   }
}

// A UDP socket bound to an IPv4 address and port, as another application's would be: no options
class Holder
{
public:
   Holder(const char* address, unsigned short port) : mFd(::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))
   {
      sockaddr_in sa;
      memset(&sa, 0, sizeof(sa));
      sa.sin_family = AF_INET;
      sa.sin_port = htons(port);
      DnsUtil::inet_pton(address, sa.sin_addr);
      mBound = mFd != INVALID_SOCKET && ::bind(mFd, reinterpret_cast<sockaddr*>(&sa), sizeof(sa)) == 0;
   }
   ~Holder() { close(); }
   void close()
   {
      if (mFd != INVALID_SOCKET)
      {
         closeSocket(mFd);
         mFd = INVALID_SOCKET;
      }
   }
   bool mBound;

private:
   Socket mFd;
};

int
main(int argc, char* argv[])
{
   initNetwork();
   Log::initialize(Log::Cout, argc > 1 ? Log::Info : Log::Warning, argv[0]);

   // Ports 41000 to 41007: RTP ports 41000, 41002, 41004 and 41006
   cerr << "-- without an address: ports aren't checked" << endl;
   {
      RTPPortManager portManager(41000, 41007);
      Holder rtp("127.0.0.1", 41000);
      check("another socket has 41000", rtp.mBound);
      check("allocateRTPPort() still gives 41000", portManager.allocateRTPPort() == 41000);
   }

   cerr << "-- with an address: ports another socket has are skipped" << endl;
   {
      RTPPortManager portManager(41000, 41007);
      Holder rtp("127.0.0.1", 41000);   // the first pair's RTP port
      Holder rtcp("127.0.0.1", 41003);  // the second pair's RTCP port
      check("other sockets have 41000 and 41003", rtp.mBound && rtcp.mBound);
      check("41000 (RTP in use) and 41002 (RTCP in use) are skipped: 41004", portManager.allocateRTPPort("127.0.0.1") == 41004);
      check("then 41006", portManager.allocateRTPPort("127.0.0.1") == 41006);
      check("then none: 0", portManager.allocateRTPPort("127.0.0.1") == 0);
      check("the skipped ports are still free", portManager.getNumFreePortPairs() == 2);
      rtp.close();
      check("once 41000's socket is closed, it's given out", portManager.allocateRTPPort("127.0.0.1") == 41000);
      portManager.freeRTPPort(41004);
      check("a port that's freed is given out again", portManager.allocateRTPPort("127.0.0.1") == 41004);
   }

   // Whether a socket can be bound to a specific address while another has the port on all of them
   // depends on the OS - the check agrees with what the media stream's socket gets
   cerr << "-- another socket bound to the port on all addresses" << endl;
   {
      RTPPortManager portManager(41010, 41011);
      Holder any("0.0.0.0", 41010);
      check("another socket has 0.0.0.0:41010", any.mBound);
#ifdef _WIN32
      // Windows lets a socket bind a specific address (and take its traffic) - SO_EXCLUSIVEADDRUSE or not
      check("Windows: 41010 can be bound on 127.0.0.1", portManager.allocateRTPPort("127.0.0.1") == 41010);
#else
      check("41010 is skipped on 127.0.0.1: 0", portManager.allocateRTPPort("127.0.0.1") == 0);
#endif
   }

   // recon's sessionCaps connection address - which media streams bind - is 0.0.0.0 by default (a
   // decorator puts the address the SIP message is sent from in the SDP): a socket on any address
   // has the port
   cerr << "-- the address is 0.0.0.0, another socket has the port on one address" << endl;
   {
      RTPPortManager portManager(41020, 41023);
      Holder loopback("127.0.0.1", 41020);
      check("another socket has 127.0.0.1:41020", loopback.mBound);
      check("41020 is skipped on 0.0.0.0: 41022", portManager.allocateRTPPort("0.0.0.0") == 41022);
   }

   // Not a port problem: the media stream's socket reports it
   cerr << "-- an address that isn't this host's: ports aren't skipped" << endl;
   {
      RTPPortManager portManager(41000, 41007);
      check("192.0.2.1 (TEST-NET-1): 41000 is given out", portManager.allocateRTPPort("192.0.2.1") == 41000);
   }

   cerr << "-- an address that isn't an IP address: ports aren't checked" << endl;
   {
      RTPPortManager portManager(41000, 41007);
      Holder rtp("127.0.0.1", 41000);
      check("41000 is given out", portManager.allocateRTPPort("example.com") == 41000);
   }

   cerr << (failures ? "FAILED" : "all checks passed") << endl;
   return failures ? 1 : 0;
}
