#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/ConnectionManager.hxx"
#include "resiprocate/os/Logger.hxx"
#include <vector>

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

// smallest time to reuse
const UInt64 
ConnectionManager::MinLastUsed = 1;

// largest unused time to reclaim
const UInt64 
ConnectionManager::MaxLastUsed = 1000;

ConnectionManager::ConnectionManager() : 
   mWriteMark(mWriteSet.end()),
   mReadMark(mReadSet.end()),
   mConnectionIdGenerator(1) 
{
   mPreYoungest.mOlder = &mPostOldest;
   mPostOldest.mYounger = &mPreYoungest;
}

ConnectionManager::~ConnectionManager()
{
   for (Connection* c = getNextRead(); c != 0; delete c );
}

Connection*
ConnectionManager::findConnection(const Tuple& addr)
{
   if (addr.connectionId == 0)
   {
      AddrMap::const_iterator i = mAddrMap.find(addr);
      if (i != mAddrMap.end())
      {
         return i->second;
      }
   }
   else
   {
      IdMap::const_iterator i = mIdMap.find(addr.connectionId);
      if (i != mIdMap.end())
      {
         return i->second;
      }
   }
   
   return 0;
}

Connection*
ConnectionManager::getNextRead()
{
   if (mReadSet.empty())
   {
      return 0;
   }
   else 
   {
      if (++mReadMark == mReadSet.end())
      {
         mReadMark = mReadSet.begin();
      }
      return *mReadMark;
   }
}

Connection*
ConnectionManager::getNextWrite()
{
   if (mWriteSet.empty())
   {
      return 0;
   }
   else 
   {
      if (++mWriteMark == mWriteSet.end())
      {
         mWriteMark = mWriteSet.begin();
      }
      return *mWriteMark;
   }
}

void
ConnectionManager::buildFdSet( FdSet& fdset)
{
   for (std::list<Connection*>::iterator i=mReadSet.begin(); i!=mReadSet.end(); i++)
   {
      fdset.setRead((*i)->getSocket());
   }

   for (std::list<Connection*>::iterator i=mWriteSet.begin(); i!=mWriteSet.end(); i++)
   {
      fdset.setWrite((*i)->getSocket());
   }
}


void
ConnectionManager::addToWritable(Connection* conn)
{
   mWriteSet.push_back(conn);
}

void
ConnectionManager::removeFromWritable()
{
   assert(!mWriteSet.empty());
   mWriteMark = mWriteSet.erase(mWriteMark);
   if (!mWriteSet.empty() && mWriteMark == mWriteSet.end())
   {
      mWriteMark = mWriteSet.begin();
   }
}

void
ConnectionManager::addConnection(Connection* connection)
{
   connection->mWho.connectionId = ++mConnectionIdGenerator;
   DebugLog (<< "ConnectionManager::addConnection() " << connection->mWho.connectionId);
   
   mAddrMap[connection->mWho] = connection;
   mIdMap[connection->mWho.connectionId] = connection;
   mReadSet.push_back(connection);
   
   // add to least recently used
   connection->mOlder = mPreYoungest.mOlder;
   mPreYoungest.mOlder->mYounger = connection;
   connection->mYounger = &mPreYoungest;
   mPreYoungest.mOlder = connection;
}

void
ConnectionManager::removeConnection(Connection* connection)
{
   DebugLog (<< "ConnectionManager::removeConnection()");
   
   connection->mYounger->mOlder = connection->mOlder;
   connection->mOlder->mYounger = connection->mYounger;

   assert(!mReadSet.empty());
   mReadSet.remove(connection);
   mWriteSet.remove(connection);
   if (!mReadSet.empty())
   {
      mReadMark = mReadSet.begin();
   }

   if (!mWriteSet.empty())
   {
      mWriteMark = mWriteSet.begin();
   }
}

// release excessively old connections (free up file descriptors)
void
ConnectionManager::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;

   // start with the oldest
   while (true)
   {
      Connection* i = mPostOldest.mYounger;
      if ((i != &mPreYoungest) && (i->mLastUsed < threshhold))
      {
         delete i;
      }
      else
      {
         break;
      }
   }
}

// move to youngest
void
ConnectionManager::touch(Connection* connection)
{
   connection->mLastUsed = Timer::getTimeMs();

   connection->mOlder->mYounger = connection->mYounger;
   connection->mYounger->mOlder = connection->mOlder;

   connection->mOlder = mPreYoungest.mOlder;
   mPreYoungest.mOlder->mYounger = connection;
   connection->mYounger = &mPreYoungest;
   mPreYoungest.mOlder = connection;
}

