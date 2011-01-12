#ifndef __RTPPORTALLOCATOR_HXX__
#define __RTPPORTALLOCATOR_HXX__

#include <set>

namespace boost
{
namespace asio
{
   class io_service;
}
}
namespace recon
{
   class UserAgentMasterProfile;

   class RTPPortAllocator
   {
   public:
      /**
       * Create a new port allocator object. The default values for the min
       * and max ports come from the IANA definition of "dynamic", or "private"
       * ports, so they will be used when nothing is specified.
       *
       * Note that specifying zero for both minPoart and maxPort is the same
       * as using the default values, i.e. the dynamic range will be used.
       */
      RTPPortAllocator(boost::asio::io_service& ioService, unsigned int minPort = 49152, unsigned int maxPort = 65535);
      RTPPortAllocator(boost::asio::io_service& ioService, resip::SharedPtr<UserAgentMasterProfile> uamp);
      virtual ~RTPPortAllocator();

      virtual bool allocateUDPPort(unsigned int& port);
      virtual void freeUDPPort(unsigned int& port);

      virtual bool allocateTCPPort(unsigned int& port);
      virtual void freeTCPPort(unsigned int& port);

      virtual bool allocateRTPPort(unsigned int& rtpPort, unsigned int& rtcpPort);
      virtual void freeRTPPort(unsigned int& rtpPort, unsigned int& rtcpPort);

      /**
       * This version of freeRTPPort assumes that the rtcpPort was allocated
       * one higher than the rtp port
       */
      virtual void freeRTPPort(unsigned int& rtpPort);

   private:
      bool allocateRTPPortFromRange(unsigned int minPort, unsigned int maxPort, unsigned int& ulOutRTPPort, unsigned int& ulOutRTCPPort);

   private:
      boost::asio::io_service& mIOService;
      unsigned int mMinPort;
      unsigned int mMaxPort;

      // port tracking
      std::set<unsigned int> mUDPPorts;
      std::set<unsigned int> mTCPPorts;
   };
}

#endif // __RTPPORTALLOCATOR_HXX__
