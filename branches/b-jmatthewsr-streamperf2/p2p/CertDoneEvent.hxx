#ifndef P2P_CertDone_hxx
#define P2P_CertDone_hxx

#include "rutil/Data.hxx"
#include "Event.hxx"

namespace p2p 
{

class CertDone : public Event 
{
   public:
      enum Resolution
      {
         Succeeded = 0,
         NoNetWork,
         SingerUnknown,
      };

      CertDone(const resip::Data& id,
               Resolution resolution)
         : mId(id) 
      {}
         
      virtual resip::Data brief() const 
      {
         return "CertDone";
      }
      

   private:
      const resip::Data mId;
      Resolution mResolution;
};

} // p2p

#endif // P2P_CertDone_hxx
