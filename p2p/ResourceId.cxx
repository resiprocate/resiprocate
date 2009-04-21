#include "p2p/ResourceId.hxx"

using namespace p2p;

ResourceId::ResourceId()
{

}

ResourceId::ResourceId(const resip::Data& rid) : 
   mResourceId(rid)
{
}

ResourceId::ResourceId(const ResourceId& rhs) : 
   mResourceId(rhs.mResourceId)
{
}

resip::Data&
ResourceId::operator=(const ResourceId& rhs)
{
   return mResourceId = rhs.mResourceId;
}

bool
ResourceId::operator==(const ResourceId& rhs) const
{
   return mResourceId == rhs.mResourceId;
}

const resip::Data& 
ResourceId::value() const
{
   return mResourceId;
}
