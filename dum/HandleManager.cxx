#include <cassert>
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/HandleManager.hxx"
#include "resiprocate/dum/HandleException.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

HandleManager::HandleManager() : mLastId(0)
{
}

HandleManager::~HandleManager()
{
   // !jf! do nothing?
}

Handled::Id
HandleManager::create(Handled* handled)
{
   mHandleMap[++mLastId] = handled;
   return mLastId;
}

void
HandleManager::remove(Handled::Id id)
{
   HandleMap::iterator i = mHandleMap.find(id);
   assert (i != mHandleMap.end());

//!dcm! -- looks completely wrong   delete i->second;
   mHandleMap.erase(i);
}


bool
HandleManager::isValidHandle(Handled::Id id) const
{
   return mHandleMap.count(id);
}

Handled*
HandleManager::getHandled(Handled::Id id) const
{
   HandleMap::const_iterator i = mHandleMap.find(id);
   if (i == mHandleMap.end())
   {
      InfoLog (<< "Reference to stale handle: " << id);
      throw HandleException("Stale handle", __FILE__, __LINE__);
   }
   else
   {
      assert(i->second);
      return i->second;
   }
}

