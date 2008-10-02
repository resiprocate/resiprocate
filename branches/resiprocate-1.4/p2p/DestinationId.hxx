#ifndef __P2P_DESTINATION_ID_HXX
#define __P2P_DESTINATION_ID_HXX 

#include "p2p/MessageStructsGen.hxx"
#include "p2p/NodeId.hxx"
#include "p2p/ResourceId.hxx"

namespace p2p
{

// Either a CompressedId, NodeId or ResourceId
class DestinationId : private s2c::DestinationStruct
{
   public:
      DestinationId(s2c::DestinationStruct);
      DestinationId(const NodeId& nid);
      DestinationId(const ResourceId& rid);

      bool isNodeId() const;
      NodeId asNodeId() const;
      
      bool isCompressedId() const;
      CompressedId asCompressedId() const;
      
      bool isResourceId() const;
      ResourceId asResourceId() const;
      
      bool operator==(const NodeId& nid) const;
      bool operator==(const DestinationId& nid) const;

      s2c::DestinationStruct* copyDestinationStruct() const;
};

}

#endif
