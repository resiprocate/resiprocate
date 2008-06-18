#include "p2p/s2c/s2c/s2c_native.hxx"

namespace s2c {

class NodeIdStruct : public PDU {
public:
   NodeIdStruct();
   unsigned char                 mId[16];


   PDUMemberFunctions
};


class ResourceIdStruct : public PDU {
public:
   ResourceIdStruct();
   std::vector<unsigned char>    mId;


   PDUMemberFunctions
};


typedef enum {
   reserved = 0,
   peer = 1,
   resource = 2,
   compressed = 3
} DestinationType;

class DestinationStruct : public PDU {
public:
   DestinationStruct();
   DestinationType               mType;
   UInt8                         mLength;
   struct {
        NodeIdStruct*                 mNodeId;
   } mPeer;
   struct {
        ResourceIdStruct*             mResourceId;
   } mResource;
   struct {
        std::vector<unsigned char>    mCompressedId;
   } mCompressed;


   PDUMemberFunctions
};


typedef enum {
   reserved1 = 0,
   signer_identity_peer = 1,
   signer_identity_name = 2,
   signer_identity_certificate = 3
} SignerIdentityType;

class SignerIdentityStruct : public PDU {
public:
   SignerIdentityStruct();
   SignerIdentityType            mIdentityType;
   std::vector<unsigned char>    mSignerIdentity;


   PDUMemberFunctions
};


class SignatureAndHashAlgorithmStruct : public PDU {
public:
   SignatureAndHashAlgorithmStruct();
   UInt8                         mSig;
   UInt8                         mHash;


   PDUMemberFunctions
};


class SignatureStruct : public PDU {
public:
   SignatureStruct();
   SignatureAndHashAlgorithmStruct*  mAlgorithm;
   SignerIdentityStruct*         mIdentity;
   std::vector<unsigned char>    mSignatureValue;


   PDUMemberFunctions
};


class ForwardingLayerMessageStruct : public PDU {
public:
   ForwardingLayerMessageStruct();
   UInt8                         mReloToken;
   UInt32                        mOverlay;
   UInt8                         mTtl;
   UInt8                         mReserved;
   UInt16                        mFragment;
   UInt8                         mVersion;
   UInt32                        mLength;
   UInt64                        mTransactionId;
   UInt16                        mFlags;
   std::vector<DestinationStruct*>  mViaList;
   std::vector<DestinationStruct*>  mDestinationList;
   UInt16                        mRouteLogLenDummy;
   UInt16                        mMessageCode;
   std::vector<unsigned char>    mPayload;
   SignatureStruct*              mSig;


   PDUMemberFunctions
};


}
