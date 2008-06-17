#ifndef P2P_CertDone_hxx
#define P2P_CertDone_hxx

#include "rutil/Data.hxx"

namespace p2p 
{

class CertDone :public Event 
{
   public:
      CertDone(const resip::Data& id,
               Resolution resolution)
         : mId(id) 
      {}
      
      enum Resolution
      {
         Succeeded = 0,
         NoNetWork,
         SingerUnknown,
      }
   
   private:
      const resip:Data mId;
      Resolution mResolution;
};

} // p2p

#endif // P2P_CertDone_hxx
