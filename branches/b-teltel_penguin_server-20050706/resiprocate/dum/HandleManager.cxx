#include <cassert>
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"
#include "resiprocate/dum/HandleManager.hxx"
#include "resiprocate/dum/HandleException.hxx"

using namespace resip;
#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

HandleManager::HandleManager() : 
   mShuttingDown(false),
   mLastId(0)
{
}

HandleManager::~HandleManager()
{
   // !jf! do nothing?
   // !dcm! -- this is the best we can do w/out a back-ptr to each handle
   // DUM currently cleans up properly, so not an issue unless users make their
   // own handled objects, could clean up memeory, but the app will crash first
   // handle deference regardless.
   if (!mHandleMap.empty())
   {
      DebugLog ( << "&&&&&& HandleManager::~HandleManager " );
      DebugLog ( << Inserter(mHandleMap) );   
      //throw HandleException("Deleting handlemanager that still has Handled objects", __FILE__, __LINE__);
   }
}

Handled::Id
HandleManager::create(Handled* handled)
{
   mHandleMap[++mLastId] = handled;
   return mLastId;
}

void HandleManager::shutdownWhenEmpty()
{
   mShuttingDown = true;
   if (mHandleMap.empty())
   {
      shutdown();      
   }
   else
   {
      DebugLog (<< "Shutdown waiting for all usages to be deleted (" << mHandleMap.size() << ")");
   }
}

// !jf! this will leak if there are active usages
void HandleManager::shutdown()
{
   WarningLog (<< "Forcing shutdown " << mHandleMap.size() << " active usages");
   mHandleMap.clear();
}

void
HandleManager::remove(Handled::Id id)
{
   HandleMap::iterator i = mHandleMap.find(id);
   assert (i != mHandleMap.end());
   mHandleMap.erase(i);
   if (mShuttingDown && mHandleMap.empty())
   {
      shutdown();      
   }
   else
   {
      DebugLog (<< "Waiting for usages to be deleted (" << mHandleMap.size() << ")");
   }
}

bool
HandleManager::isValidHandle(Handled::Id id) const
{
   //!dcm! -- fix; use find
   return mHandleMap.count(id) != 0;
}

Handled*
HandleManager::getHandled(Handled::Id id) const
{
   HandleMap::const_iterator i = mHandleMap.find(id);
   if (i == mHandleMap.end())
   {
      InfoLog (<< "Reference to stale handle: " << id);
      assert(0);
      throw HandleException("Stale handle", __FILE__, __LINE__);
   }
   else
   {
      assert(i->second);
      return i->second;
   }
}

