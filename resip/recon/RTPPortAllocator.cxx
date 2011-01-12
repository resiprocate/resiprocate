#include <boost/asio.hpp>

#include <rutil/Random.hxx>

#include "UserAgentMasterProfile.hxx"
#include "RTPPortAllocator.hxx"

using namespace recon;
using boost::asio::ip::tcp;
using boost::asio::ip::udp;

#define MIN_DYNAMIC_PORT 49152
#define MAX_DYNAMIC_PORT 65535

/** @todo : We should check the operating system to see if these ports are valid */

RTPPortAllocator::RTPPortAllocator(boost::asio::io_service& ioService, unsigned int minPort, unsigned int maxPort)
: mIOService(ioService), mMinPort(minPort), mMaxPort(maxPort)
{
   assert( mMinPort >= 0 );
   assert( mMinPort <= MAX_DYNAMIC_PORT );
   assert( mMaxPort >= 0 );
   assert( mMaxPort <= MAX_DYNAMIC_PORT );
   assert( mMinPort <= mMaxPort );

   if ((mMinPort == 0 && mMaxPort == 0) || (mMinPort > mMaxPort))
   {
      // Use the IANA dynamic range instead
      mMinPort = MIN_DYNAMIC_PORT;
      mMaxPort = MAX_DYNAMIC_PORT;
   }
}

RTPPortAllocator::RTPPortAllocator(boost::asio::io_service& ioService, resip::SharedPtr<UserAgentMasterProfile> uamp)
: mIOService(ioService), mMinPort(uamp->rtpPortRangeMin()), mMaxPort(uamp->rtpPortRangeMax())
{
   assert( mMinPort >= 0 );
   assert( mMinPort <= MAX_DYNAMIC_PORT );
   assert( mMaxPort >= 0 );
   assert( mMaxPort <= MAX_DYNAMIC_PORT );
   assert( mMinPort <= mMaxPort );

   if ((mMinPort == 0 && mMaxPort == 0) || (mMinPort > mMaxPort))
   {
      // Use the IANA dynamic range instead
      mMinPort = MIN_DYNAMIC_PORT;
      mMaxPort = MAX_DYNAMIC_PORT;
   }
}

RTPPortAllocator::~RTPPortAllocator()
{
   mUDPPorts.clear();
   mTCPPorts.clear();
}

bool RTPPortAllocator::allocateUDPPort(unsigned int& ulOutPort)
{
   ulOutPort = 0;
   boost::system::error_code ec;

   for( unsigned int curPort = mMinPort ; curPort <= mMaxPort ; ++curPort )
   {
      // This is an O^2 loop, but the number of allocated ports at any time
      // should be very small (i.e. at most 10)
      if (mUDPPorts.find(curPort) != mUDPPorts.end())
         continue;

      // !ds! will have to be changed to get ipv6 going
      udp::socket testSocket(mIOService);
      udp::endpoint testEndpoint(udp::v4(), curPort);
      testSocket.open(udp::v4());
      testSocket.bind(testEndpoint, ec);
      testSocket.close();
      if (!ec)
      {
         // There was no error, so the socket must be ok. Close it and return
         // this value.
         ulOutPort = testEndpoint.port();
         mUDPPorts.insert(ulOutPort);
         return true;
      }
   }

   // Couldn't find an appropriate port
   return false;
}

bool RTPPortAllocator::allocateTCPPort(unsigned int& ulOutPort)
{
   ulOutPort = 0;
   boost::system::error_code ec;

   for( unsigned int curPort = mMinPort ; curPort <= mMaxPort ; ++curPort )
   {
      // This is an O^2 loop, but the number of allocated ports at any time
      // should be very small (i.e. at most 10)
      if (mTCPPorts.find(curPort) != mTCPPorts.end())
         continue;

      // !ds! will have to be changed to get ipv6 going
      tcp::socket testSocket(mIOService);
      tcp::endpoint testEndpoint(tcp::v4(), curPort);
      testSocket.open(tcp::v4());
      testSocket.bind(testEndpoint, ec);
      testSocket.close();
      if (!ec)
      {
         // There was no error, so the endpoint/port must be ok. Make sure
         // the socket is closed (because it will be re-opened shortly) and
         // return the current value. There is a race condition here, but it's
         // better than nothing.
         ulOutPort = testEndpoint.port();
         mTCPPorts.insert(ulOutPort);
         return true;
      }
   }

   // Couldn't find an appropriate port
   return false;
}

bool RTPPortAllocator::allocateRTPPortFromRange(unsigned int minPort, unsigned int maxPort, unsigned int& ulOutRTPPort, unsigned int& ulOutRTCPPort)
{
   // Allocate 2 UDP ports in succession (one for RTP, the other for RTCP).
   // They should be in sequence (RTCP == RTP + 1)
   ulOutRTPPort  = 0;
   ulOutRTCPPort = 0;
   boost::system::error_code ec;

   // a random number between mMinPort and mMaxPort inclusive
   unsigned int r = resip::Random::getCryptoRandom() % (maxPort - minPort + 1) + minPort;
   if ((maxPort - r) < 10) r = minPort; // safety factor

   // ensure we use an EVEN port for RTP, per RFC 3550 section 11
   if (r % 2 != 0) r++;

   for( unsigned int testRTPPort = r ; testRTPPort <= maxPort ; testRTPPort+=2 )
   {
      // This is an O^2 loop, but the number of allocated ports at any time
      // should be very small (i.e. at most 10)
      if (mUDPPorts.find(testRTPPort) != mUDPPorts.end())
         continue;

      // !ds! will have to be changed to get ipv6 going
      tcp::socket testRTPSocket(mIOService);
      tcp::endpoint testRTPEndpoint(tcp::v4(), testRTPPort);
      testRTPSocket.open(tcp::v4());
      testRTPSocket.bind(testRTPEndpoint, ec);
      testRTPSocket.close();
      if (!ec)
      {
         // There was no error, so the endpoint/port must be ok. Now check
         // the adjacent port
         unsigned int testRTCPPort = testRTPEndpoint.port() + 1;
         if ( testRTCPPort > maxPort )
            return false;

         // Also make sure the RTCP port wasn't previously allocated
         if (mUDPPorts.find(testRTCPPort) != mUDPPorts.end())
            continue;

         tcp::socket testRTCPSocket(mIOService);
         tcp::endpoint testRTCPEndpoint(tcp::v4(), testRTCPPort);
         testRTCPSocket.open(tcp::v4());
         testRTCPSocket.bind(testRTCPEndpoint, ec);
         testRTCPSocket.close();
         if (!ec)
         {
            // At this point, we were able to reserve two adjacent UDP ports,
            // so we return them as the selected RTP and RTCP.
            ulOutRTPPort  = testRTPEndpoint.port();
            ulOutRTCPPort = testRTCPEndpoint.port();
            mUDPPorts.insert(ulOutRTPPort);
            mUDPPorts.insert(ulOutRTCPPort);
            return true;
         }
      }
   }

   return false;
}

bool RTPPortAllocator::allocateRTPPort(unsigned int& ulOutRTPPort, unsigned int& ulOutRTCPPort)
{
   if (!allocateRTPPortFromRange(mMinPort, mMaxPort, ulOutRTPPort, ulOutRTCPPort))
   {
      return allocateRTPPortFromRange(MIN_DYNAMIC_PORT, MAX_DYNAMIC_PORT, ulOutRTPPort, ulOutRTCPPort);
   }
   return true;
}

void recon::RTPPortAllocator::freeUDPPort( unsigned int& port )
{
   std::set<unsigned int>::iterator iter = mUDPPorts.find(port);
   if( iter != mUDPPorts.end() )
      mUDPPorts.erase(iter);

   port = 0;
}

void recon::RTPPortAllocator::freeTCPPort( unsigned int& port )
{
   std::set<unsigned int>::iterator iter = mTCPPorts.find(port);
   if( iter != mTCPPorts.end() )
      mTCPPorts.erase(iter);

   port = 0;
}

void recon::RTPPortAllocator::freeRTPPort( unsigned int& rtpPort, unsigned int& rtcpPort )
{
   freeUDPPort( rtpPort );
   freeUDPPort( rtcpPort );
}

void recon::RTPPortAllocator::freeRTPPort( unsigned int& rtpPort )
{
   unsigned int rtcpPort = rtpPort + 1;
   freeRTPPort( rtpPort, rtcpPort );
}