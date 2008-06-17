#include <cassert>
#include "p2p/ChordNodeId.hxx"

using namespace p2p;

ChordNodeId::ChordNodeId(const resip::Data& nid)  : NodeId(nid)
{
}

ChordNodeId::ChordNodeId(const NodeId& nid) : NodeId(nid)
{
}

bool
ChordNodeId::operator<(const ChordNodeId& rhs) const
{
   assert(0);
   return true;
}

bool
ChordNodeId::operator<=(const ChordNodeId& rhs) const
{
   return *this == rhs || *this < rhs;
}
