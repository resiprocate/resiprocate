#if !defined(RESIP_MERGEDREQUESTKEY_HXX)
#define RESIP_MERGEDREQUESTKEY_HXX

#include "resiprocate/os/Data.hxx"

namespace resip
{
class SipMessage;

class MergedRequestKey
{
   public:
      MergedRequestKey();
      MergedRequestKey(const SipMessage& request);
      bool operator==(const MergedRequestKey& other) const;
      bool operator!=(const MergedRequestKey& other) const;
      bool operator<(const MergedRequestKey& other) const;

      static const MergedRequestKey Empty;

   private:
      //MergedRequestKey(const MergedRequestKey& key);
      MergedRequestKey& operator=(const MergedRequestKey& key);
      
      Data mRequestUri;
      Data mCseq;
      Data mTag;
      Data mCallId;
};
 
}

#endif
