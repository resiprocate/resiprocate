#include "p2p/s2c/s2c/s2c_native.hxx"

namespace s2c {

class NodeIdStruct : public PDU {
public:
   unsigned char                 mId[16];


   NodeIdStruct() {mName = "NodeId";}
   PDUMemberFunctions
};


class ResourceIdStruct : public PDU {
public:
   std::vector<unsigned char>    mId;


   ResourceIdStruct() {mName = "ResourceId";}
   PDUMemberFunctions
};


enum DestinationType {
   reserved = 0,
   peer = 1,
   resource = 2,
   compressed = 3
};

class DestinationDataStruct : public PDU {
public:
   int  mType;
   enum { 
          tPeer=2,
          tResource=2,
          tCompressed=3
   };

   struct {
     struct A3 {
          NodeIdStruct*                 mNodeId;
     } mPeer;
     struct {
          ResourceIdStruct*             mResourceId;
     } mResource;
     struct {
          std::vector<unsigned char>    mCompressedId;
     } mCompressed;
   } u;


   DestinationDataStruct() {mName = "DestinationData";}
   PDUMemberFunctions
};

class DestinationStruct : public PDU {
public:
   DestinationType               mType;
   UInt8                         mLength;
   DestinationDataStruct*        mDestinationData;


   DestinationStruct() {mName = "Destination";}
   PDUMemberFunctions
};


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
   std::vector<DestinationStruct*>  mViaList;
   std::vector<DestinationStruct*>  mDestinationList;
   UInt16                        mRouteLogLenDummy;


   ForwardingHdrStruct() {mName = "ForwardingHdr";}
   PDUMemberFunctions
};


}
