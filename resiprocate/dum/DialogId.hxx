#if !defined(RESIP_DIALOG_ID_HXX)
#define RESIP_DIALOG_ID_HXX

#include "resiprocate/os/Data.hxx"
#include "resiprocate/dum/DialogSetId.hxx"

namespace resip
{

class DialogId
{
   public:
      DialogId(const SipMessage& msg );
      DialogId(const Data& callId, const Data& localTag, const Data& remoteTag );
      DialogId(const DialogSetId id, const Data& remoteTag );
      
      bool operator==(const DialogId& rhs) const;
      bool operator!=(const DialogId& rhs) const;
      bool operator<(const DialogId& rhs) const;

      const DialogSetId& getDialogSetId() const;

#if defined(HASH_MAP_NAMESPACE)
      friend struct HASH_MAP_NAMESPACE::hash<resip::DialogId>;
#endif
#if defined(__INTEL_COMPILER )
      friend size_t hash_value(const resip::DialogId& id);
#endif
   private:
      DialogSetId mDialogSetId;
      Data mRemoteTag;
};

}
  
#endif
