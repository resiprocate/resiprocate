#include "UserAgentMasterProfile.hxx"
#include "RTPPortAllocator.hxx"

using namespace recon;

/** @todo : We should check the operating system to see if these ports are valid */

RTPPortAllocator::RTPPortAllocator( UserAgentMasterProfile& uamp )
   : mMasterProfile( uamp )
{
   mRTPPortFreeList.clear();
   for(unsigned int i = mMasterProfile.rtpPortRangeMin(); i <= mMasterProfile.rtpPortRangeMax();)
   {
      mRTPPortFreeList.push_back(i);
      i=i+2;  // only add even ports - note we are assuming rtpPortRangeMin is even
   }
}

unsigned int 
RTPPortAllocator::allocateRTPPort()
{
   unsigned int port = 0;
   if(!mRTPPortFreeList.empty())
   {
      port = mRTPPortFreeList.front();
      mRTPPortFreeList.pop_front();
   }
   return port;
}
 
void
RTPPortAllocator::freeRTPPort(unsigned int port)
{
   if (port >= mMasterProfile.rtpPortRangeMin() && port <= mMasterProfile.rtpPortRangeMax())
      mRTPPortFreeList.push_back(port);
}
