#include "s2c_native.hxx"

namespace s2c {
namespace fwding {

class ForwardingHdrStruct : public PDU {
public:
   UInt8                         mReloToken;
   UInt32                        mOverlay;
   UInt8                         mTtl;
   UInt8                         mReserved;
   UInt16                        mFragment;
   UInt8                         mVersion;
   UInt32                        mLength;
   UInt64                        mTransactionId;
   UInt16                        mFlags;
   UInt16                        mViaListLength;
   UInt16                        mDestinationListLength;


   ForwardingHdrStruct() {mName = "ForwardingHdr";}
   PDUMemberFunctions
};


}
}
