#include "s2c/s2c_native.hxx"

namespace s2c {

class ForwardingHdrStruct : public PDU {
public:
   uint8_t                         mReloToken;
   uint32_t                        mOverlay;
   uint8_t                         mTtl;
   uint8_t                         mReserved;
   uint16_t                        mFragment;
   uint8_t                         mVersion;
   uint32_t                        mLength;
   uint64_t                        mTransactionId;
   uint16_t                        mFlags;
   uint16_t                        mViaListLength;
   uint16_t                        mDestinationListLength;


   ForwardingHdrStruct() {mName = "ForwardingHdr";}
   PDUMemberFunctions
};


}
