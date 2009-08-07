#ifndef __RTPPORTALLOCATOR_HXX__
#define __RTPPORTALLOCATOR_HXX__

#include <deque>

namespace recon
{
   class UserAgentMasterProfile;

   class RTPPortAllocator
   {
   public:
      RTPPortAllocator(UserAgentMasterProfile& uamp);
      virtual ~RTPPortAllocator() {};

      unsigned int allocateRTPPort();
      void freeRTPPort(unsigned int port);

   private:
      UserAgentMasterProfile& mMasterProfile;
      std::deque<unsigned int> mRTPPortFreeList;
   };
}

#endif // __RTPPORTALLOCATOR_HXX__