#include "p2p/ResourceId.hxx"

using namespace p2p;


ResourceId::ResourceId(const resip::Data& rid) : 
   mResourceId(rid)
{
}

ResourceId::ResourceId(const resip::ResourceId& rhs) : 
   mResourceId(rhs.mResourceId)
{
}

const resip::Data&
ResourceId::operator=(const ResourceId& rhs)
{
   mResourceId = rhs.mResourceId;
}

const resip::Data& 
ResourceId::value() const
{
   return mResourceId;
}
