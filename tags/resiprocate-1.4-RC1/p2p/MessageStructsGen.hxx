#ifndef _MessageStructs_h_
#define _MessageStructs_h_

#include "p2p/s2c/s2c/s2c_native.hxx"

namespace s2c {

typedef enum {
   bool_false = 0,
   bool_true = 1
} Boolean;

class NodeIdStruct : public PDU {
public:
   NodeIdStruct();
   NodeIdStruct(const NodeIdStruct&);
   UInt64                        mHigh;
   UInt64                        mLow;


   PDUMemberFunctions
};


class ResourceIdStruct : public PDU {
public:
   ResourceIdStruct();
   ResourceIdStruct(const ResourceIdStruct&);
   resip::Data                   mId;


   PDUMemberFunctions
};


typedef enum {
   reserved_addr = 0,
   ipv4_address = 1,
   ipv6_address = 2
} AddressType;

class IPv4AddrPortStruct : public PDU {
public:
   IPv4AddrPortStruct();
   IPv4AddrPortStruct(const IPv4AddrPortStruct&);
   UInt32                        mAddr;
   UInt16                        mPort;


   PDUMemberFunctions
};


class IPv6AddrPortStruct : public PDU {
public:
   IPv6AddrPortStruct();
   IPv6AddrPortStruct(const IPv6AddrPortStruct&);
   unsigned char                 mAddr[16];
   UInt16                        mPort;


   PDUMemberFunctions
};


class IpAddressAndPortStruct : public PDU {
public:
   IpAddressAndPortStruct();
   IpAddressAndPortStruct(const IpAddressAndPortStruct&);
   AddressType                   mType;
   UInt8                         mLength;
   struct mIpv4Address_ {
        IPv4AddrPortStruct*           mV4addrPort;
   } mIpv4Address;
   struct mIpv6Address_ {
        IPv6AddrPortStruct*           mV6addrPort;
   } mIpv6Address;


   PDUMemberFunctions
};


typedef enum {
   reserved_destination = 0,
   peer = 1,
   resource = 2,
   compressed = 3
} DestinationType;

class DestinationStruct : public PDU {
public:
   DestinationStruct();
   DestinationStruct(const DestinationStruct&);
   DestinationType               mType;
   struct mPeer_ {
        NodeIdStruct*                 mNodeId;
   } mPeer;
   struct mResource_ {
        ResourceIdStruct*             mResourceId;
   } mResource;
   struct mCompressed_ {
        resip::Data                   mCompressedId;
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
   SignerIdentityStruct(const SignerIdentityStruct&);
   SignerIdentityType            mIdentityType;
   resip::Data                   mSignerIdentity;


   PDUMemberFunctions
};


class SignatureAndHashAlgorithmStruct : public PDU {
public:
   SignatureAndHashAlgorithmStruct();
   SignatureAndHashAlgorithmStruct(const SignatureAndHashAlgorithmStruct&);
   UInt8                         mSig;
   UInt8                         mHash;


   PDUMemberFunctions
};


class SignatureStruct : public PDU {
public:
   SignatureStruct();
   SignatureStruct(const SignatureStruct&);
   SignatureAndHashAlgorithmStruct*  mAlgorithm;
   SignerIdentityStruct*         mIdentity;
   resip::Data                   mSignatureValue;


   PDUMemberFunctions
};


class ForwardingHeaderStruct : public PDU {
public:
   ForwardingHeaderStruct();
   ForwardingHeaderStruct(const ForwardingHeaderStruct&);
   UInt32                        mReloToken;
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


   PDUMemberFunctions
};


class ForwardingLayerMessageStruct : public PDU {
public:
   ForwardingLayerMessageStruct();
   ForwardingLayerMessageStruct(const ForwardingLayerMessageStruct&);
   ForwardingHeaderStruct*       mHeader;
   resip::Data                   mPayload;
   SignatureStruct*              mSig;


   PDUMemberFunctions
};


class MessagePayloadStruct : public PDU {
public:
   MessagePayloadStruct();
   MessagePayloadStruct(const MessagePayloadStruct&);
   resip::Data                   mPayload;


   PDUMemberFunctions
};


class ErrorResponseStruct : public PDU {
public:
   ErrorResponseStruct();
   ErrorResponseStruct(const ErrorResponseStruct&);
   UInt16                        mErrorCode;
   resip::Data                   mReasonPhrase;
   resip::Data                   mErrorInfo;


   PDUMemberFunctions
};


class JoinReqStruct : public PDU {
public:
   JoinReqStruct();
   JoinReqStruct(const JoinReqStruct&);
   NodeIdStruct*                 mJoiningPeerId;
   resip::Data                   mOverlaySpecificData;


   PDUMemberFunctions
};


class JoinAnsStruct : public PDU {
public:
   JoinAnsStruct();
   JoinAnsStruct(const JoinAnsStruct&);
   resip::Data                   mOverlaySpecificData;


   PDUMemberFunctions
};


class LeaveReqStruct : public PDU {
public:
   LeaveReqStruct();
   LeaveReqStruct(const LeaveReqStruct&);
   NodeIdStruct*                 mLeavingPeerId;
   resip::Data                   mOverlaySpecificData;


   PDUMemberFunctions
};


class RouteQueryReqStruct : public PDU {
public:
   RouteQueryReqStruct();
   RouteQueryReqStruct(const RouteQueryReqStruct&);
   Boolean                       mSendUpdate;
   DestinationStruct*            mDestination;
   resip::Data                   mOverlaySpecificData;


   PDUMemberFunctions
};


typedef enum {
   data = 128,
   ack = 129
} FramedMessageType;

class FramedMessageStruct : public PDU {
public:
   FramedMessageStruct();
   FramedMessageStruct(const FramedMessageStruct&);
   FramedMessageType             mType;
   struct mData_ {
        UInt32                        mSequence;
        resip::Data                   mMessage;
   } mData;
   struct mAck_ {
        UInt32                        mAckSequence;
        UInt32                        mReceived;
   } mAck;


   PDUMemberFunctions
};


class IceCandidateStruct : public PDU {
public:
   IceCandidateStruct();
   IceCandidateStruct(const IceCandidateStruct&);
   resip::Data                   mCandidate;


   PDUMemberFunctions
};


class ConnectReqAnsStruct : public PDU {
public:
   ConnectReqAnsStruct();
   ConnectReqAnsStruct(const ConnectReqAnsStruct&);
   resip::Data                   mUfrag;
   resip::Data                   mPassword;
   UInt16                        mApplication;
   resip::Data                   mRole;
   std::vector<IceCandidateStruct*>  mCandidates;


   PDUMemberFunctions
};


typedef enum {
   responsible_set = 1,
   num_resources = 2
} PingInformationType;

class PingReqStruct : public PDU {
public:
   PingReqStruct();
   PingReqStruct(const PingReqStruct&);
   std::vector<UInt8>            mRequestedInfo;


   PDUMemberFunctions
};


class PingInformationStruct : public PDU {
public:
   PingInformationStruct();
   PingInformationStruct(const PingInformationStruct&);
   PingInformationType           mType;
   struct mResponsibleSet_ {
        UInt32                        mResponsiblePpb;
   } mResponsibleSet;
   struct mNumResources_ {
        UInt32                        mNumResources;
   } mNumResources;


   PDUMemberFunctions
};


class PingAnsStruct : public PDU {
public:
   PingAnsStruct();
   PingAnsStruct(const PingAnsStruct&);
   UInt64                        mResponseId;
   std::vector<PingInformationStruct*>  mPingInfo;


   PDUMemberFunctions
};


class TunnelReqStruct : public PDU {
public:
   TunnelReqStruct();
   TunnelReqStruct(const TunnelReqStruct&);
   UInt16                        mApplication;
   resip::Data                   mDialogId;
   resip::Data                   mApplicationPdu;


   PDUMemberFunctions
};


typedef enum {
   reserved_data_model = 0,
   single_value = 1,
   array = 2,
   dictionary = 3
} DataModel;

class DataValueStruct : public PDU {
public:
   DataValueStruct();
   DataValueStruct(const DataValueStruct&);
   Boolean                       mExists;
   resip::Data                   mValue;


   PDUMemberFunctions
};


class ArrayEntryStruct : public PDU {
public:
   ArrayEntryStruct();
   ArrayEntryStruct(const ArrayEntryStruct&);
   UInt32                        mIndex;
   DataValueStruct*              mValue;


   PDUMemberFunctions
};


class DictionaryKeyStruct : public PDU {
public:
   DictionaryKeyStruct();
   DictionaryKeyStruct(const DictionaryKeyStruct&);
   resip::Data                   mKey;


   PDUMemberFunctions
};


class DictionaryEntryStruct : public PDU {
public:
   DictionaryEntryStruct();
   DictionaryEntryStruct(const DictionaryEntryStruct&);
   DictionaryKeyStruct*          mKey;
   DataValueStruct*              mValue;


   PDUMemberFunctions
};


class StoredDataValueStruct : public PDU {
public:
   StoredDataValueStruct();
   StoredDataValueStruct(const StoredDataValueStruct&);
   DataModel                     mModel;
   struct mSingleValue_ {
        DataValueStruct*              mSingleValueEntry;
   } mSingleValue;
   struct mArray_ {
        ArrayEntryStruct*             mArrayEntry;
   } mArray;
   struct mDictionary_ {
        DictionaryEntryStruct*        mDictionaryEntry;
   } mDictionary;


   PDUMemberFunctions
};


class StoredDataStruct : public PDU {
public:
   StoredDataStruct();
   StoredDataStruct(const StoredDataStruct&);
   UInt64                        mStorageTime;
   UInt32                        mLifetime;
   StoredDataValueStruct*        mValue;
   SignatureStruct*              mSignature;


   PDUMemberFunctions
};


class StoreKindDataStruct : public PDU {
public:
   StoreKindDataStruct();
   StoreKindDataStruct(const StoreKindDataStruct&);
   UInt32                        mKind;
   DataModel                     mDataModel;
   UInt64                        mGenerationCounter;
   std::vector<StoredDataStruct*>  mValues;


   PDUMemberFunctions
};


class StoreReqStruct : public PDU {
public:
   StoreReqStruct();
   StoreReqStruct(const StoreReqStruct&);
   ResourceIdStruct*             mResource;
   UInt8                         mReplicaNumber;
   std::vector<StoreKindDataStruct*>  mKindData;


   PDUMemberFunctions
};


class StoreKindResponseStruct : public PDU {
public:
   StoreKindResponseStruct();
   StoreKindResponseStruct(const StoreKindResponseStruct&);
   UInt32                        mKind;
   UInt64                        mGenerationCounter;
   std::vector<NodeIdStruct*>    mReplicas;


   PDUMemberFunctions
};


class StoreAnsStruct : public PDU {
public:
   StoreAnsStruct();
   StoreAnsStruct(const StoreAnsStruct&);
   std::vector<StoreKindResponseStruct*>  mKindResponses;


   PDUMemberFunctions
};


class ArrayRangeStruct : public PDU {
public:
   ArrayRangeStruct();
   ArrayRangeStruct(const ArrayRangeStruct&);
   UInt32                        mFirst;
   UInt32                        mLast;


   PDUMemberFunctions
};


class StoredDataSpecifierStruct : public PDU {
public:
   StoredDataSpecifierStruct();
   StoredDataSpecifierStruct(const StoredDataSpecifierStruct&);
   UInt32                        mKind;
   DataModel                     mModel;
   UInt64                        mGeneration;
   struct mSingleValue_ {
   } mSingleValue;
   struct mArray_ {
        std::vector<ArrayRangeStruct*>  mIndices;
   } mArray;
   struct mDictionary_ {
        std::vector<DictionaryKeyStruct*>  mKeys;
   } mDictionary;


   PDUMemberFunctions
};


class FetchReqStruct : public PDU {
public:
   FetchReqStruct();
   FetchReqStruct(const FetchReqStruct&);
   ResourceIdStruct*             mResource;
   std::vector<StoredDataSpecifierStruct*>  mSpecifiers;


   PDUMemberFunctions
};


class FetchKindResponseStruct : public PDU {
public:
   FetchKindResponseStruct();
   FetchKindResponseStruct(const FetchKindResponseStruct&);
   UInt32                        mKind;
   UInt64                        mGeneration;
   std::vector<StoredDataStruct*>  mValues;


   PDUMemberFunctions
};


class FetchAnsStruct : public PDU {
public:
   FetchAnsStruct();
   FetchAnsStruct(const FetchAnsStruct&);
   std::vector<FetchKindResponseStruct*>  mKindResponses;


   PDUMemberFunctions
};


class RemoveReqStruct : public PDU {
public:
   RemoveReqStruct();
   RemoveReqStruct(const RemoveReqStruct&);
   ResourceIdStruct*             mResource;
   std::vector<StoredDataSpecifierStruct*>  mSpecifiers;


   PDUMemberFunctions
};


class RemoveAnsStruct : public PDU {
public:
   RemoveAnsStruct();
   RemoveAnsStruct(const RemoveAnsStruct&);
   std::vector<StoreKindResponseStruct*>  mKindResponses;


   PDUMemberFunctions
};


class FindReqStruct : public PDU {
public:
   FindReqStruct();
   FindReqStruct(const FindReqStruct&);
   ResourceIdStruct*             mResource;
   std::vector<UInt32>           mKinds;


   PDUMemberFunctions
};


class FindKindDataStruct : public PDU {
public:
   FindKindDataStruct();
   FindKindDataStruct(const FindKindDataStruct&);
   UInt32                        mKind;
   ResourceIdStruct*             mClosest;


   PDUMemberFunctions
};


class FindAnsStruct : public PDU {
public:
   FindAnsStruct();
   FindAnsStruct(const FindAnsStruct&);
   std::vector<FindKindDataStruct*>  mResults;


   PDUMemberFunctions
};


class TurnServerStruct : public PDU {
public:
   TurnServerStruct();
   TurnServerStruct(const TurnServerStruct&);
   UInt8                         mIteration;
   IpAddressAndPortStruct*       mServerAddress;


   PDUMemberFunctions
};


typedef enum {
   sip_registration_uri = 1,
   sip_registration_route = 2
} SipRegistrationType;

class SipRegistrationStruct : public PDU {
public:
   SipRegistrationStruct();
   SipRegistrationStruct(const SipRegistrationStruct&);
   SipRegistrationType           mType;
   struct mSipRegistrationUri_ {
        resip::Data                   mUri;
   } mSipRegistrationUri;
   struct mSipRegistrationRoute_ {
        resip::Data                   mContactPrefs;
        std::vector<DestinationStruct*>  mDestinationList;
   } mSipRegistrationRoute;


   PDUMemberFunctions
};


typedef enum {
   reserved_chord = 0,
   peer_ready = 1,
   neighbors = 2,
   full = 3
} ChordUpdateType;

class ChordUpdateStruct : public PDU {
public:
   ChordUpdateStruct();
   ChordUpdateStruct(const ChordUpdateStruct&);
   ChordUpdateType               mType;
   struct mPeerReady_ {
   } mPeerReady;
   struct mNeighbors_ {
        std::vector<NodeIdStruct*>    mPredecessors;
        std::vector<NodeIdStruct*>    mSuccessors;
   } mNeighbors;
   struct mFull_ {
        std::vector<NodeIdStruct*>    mPredecessors;
        std::vector<NodeIdStruct*>    mSuccessors;
        std::vector<NodeIdStruct*>    mFingers;
   } mFull;


   PDUMemberFunctions
};


class ChordRouteQueryAnsStruct : public PDU {
public:
   ChordRouteQueryAnsStruct();
   ChordRouteQueryAnsStruct(const ChordRouteQueryAnsStruct&);
   NodeIdStruct*                 mNextId;


   PDUMemberFunctions
};


}

#endif
