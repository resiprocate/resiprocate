#include "resiprocate/os/Data.hxx"
#include "ResourceMgr.h"

#include <iostream>

using std::cout;
using std::endl;

ResourceMgr* ResourceMgr::mInstance = 0;

ResourceMgr& 
ResourceMgr::instance()
{
  if (!mInstance) { mInstance = new ResourceMgr(); }
  return (*mInstance);
}

bool 
ResourceMgr::exists(const Data& aor)
{
  return (mResources.find(aor)!=mResources.end());
}

bool
ResourceMgr::addResource(const Data& aor)
{
  if (exists(aor)) return 0;
  pair<Data,Resource*> mapentry = make_pair(aor, new Resource);
  pair<ResourceMap_iter,bool> inserted = mResources.insert(mapentry);
  if (!inserted.second)
  {
    delete mapentry.second;
    return 0;
  }
  else
  {
   return 1;
  }
}

bool
ResourceMgr::setPresenceDocument(const Data& aor, Contents* contents)
{
  ResourceMap_iter resIter = mResources.find(aor);
  if (resIter!=mResources.end())
  {
    (resIter->second)->setPresenceDocument(contents);
    return 1;
  }
  else
  {
    return 0;
  }
}

const Contents* 
ResourceMgr::presenceDocument(const Data& aor)
{
  ResourceMap_iter resIter = mResources.find(aor);
  if (resIter!=mResources.end())
  {
    return (resIter->second)->presenceDocument();
  }
  else
  {
    return NULL;
  }
}

void
ResourceMgr::attachToPresenceDoc(const Data& aor, SubDialog* observer)
{
  ResourceMap_iter resIter = mResources.find(aor);
  if (resIter!=mResources.end())
  {
    (resIter->second)->attachToPresenceDoc(observer);
  }
}

void
ResourceMgr::detachFromPresenceDoc(const Data& aor, SubDialog* observer)
{
  ResourceMap_iter resIter = mResources.find(aor);
  if (resIter!=mResources.end())
  {
    (resIter->second)->detachFromPresenceDoc(observer);
  }
}

