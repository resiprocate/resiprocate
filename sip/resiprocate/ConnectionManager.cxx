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
   mWriteHead(Connection::writeList::makeList(new Connection)),
   mWriteIter(mWriteHead->begin()),
   mReadHead(Connection::readList::makeList(new Connection)),
   mReadIter(mReadHead->begin()),
   mLRUHead(Connection::lruList::makeList(new Connection)),
   mConnectionIdGenerator(1) 
{
}

ConnectionManager::~ConnectionManager()
{
   for (Connection* c = getNextRead(); c != 0; delete c );
   delete mWriteHead;
   delete mReadHead;
   delete mLRUHead;
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
   if (mReadHead->empty())
   {
      return 0;
   }
   else 
   {
      if (mReadIter == mReadHead->end())
      {
         mReadIter = mReadHead->begin();
      }

      Connection* ret = *mReadIter;
      ++mReadIter;
      return ret;
   }
}

Connection*
ConnectionManager::getNextWrite()
{
   if (mWriteHead->empty())
   {
      return 0;
   }
   else 
   {
      if (mWriteIter == mWriteHead->end())
      {
         mWriteIter = mWriteHead->begin();
      }

      Connection* ret = *mWriteIter;
      ++mWriteIter;
      return ret;
   }
}

void
ConnectionManager::buildFdSet(FdSet& fdset)
{
   for (Connection::readList::iterator i = mReadHead->begin(); 
        i != mReadHead->end(); ++i)
   {
      fdset.setRead((*i)->getSocket());
   }

   for (Connection::writeList::iterator i = mWriteHead->begin(); 
        i != mWriteHead->end(); ++i)
   {
      fdset.setWrite((*i)->getSocket());
   }
}

void
ConnectionManager::addToWritable(Connection* conn)
{
   mWriteHead->push_back(conn);
}

void
ConnectionManager::removeFromWritable()
{
   assert(!mWriteHead->empty());
   Connection* current = *mWriteIter;
   ++mWriteIter;

   current->writeList::remove();

   if (mWriteIter == mWriteHead->end())
   {
      mWriteIter = mWriteHead->begin();
   }
}

void
ConnectionManager::addConnection(Connection* connection)
{
   connection->mWho.connectionId = ++mConnectionIdGenerator;
   DebugLog (<< "ConnectionManager::addConnection() " << connection->mWho.connectionId);
   
   mAddrMap[connection->mWho] = connection;
   mIdMap[connection->mWho.connectionId] = connection;

   mReadHead->push_back(connection);
   mLRUHead->push_back(connection);
}

void
ConnectionManager::removeConnection(Connection* connection)
{
   DebugLog (<< "ConnectionManager::removeConnection()");

   assert(!mReadHead->empty());
   connection->readList::remove();
   connection->writeList::remove();
   connection->lruList::remove();

   checkIterators();
}

// release excessively old connections (free up file descriptors)
void
ConnectionManager::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;
   InfoLog(<< "recycling connections older than " << relThreshhold/1000.0 << " seconds");

   for (Connection::lruList::iterator i = mLRUHead->begin();
        i != mLRUHead->end();)
   {
      if ((*i)->mLastUsed < threshhold)
      {
         Connection* discard = *i;
         InfoLog(<< "recycling connection: " << discard << " " << discard->getSocket());
         // iterate before removing
         ++i;
         delete discard;
      }
      else
      {
         break;
      }
   }

   checkIterators();
}

// move to youngest
void
ConnectionManager::touch(Connection* connection)
{
   connection->lruList::remove();
   mLRUHead->push_back(connection);
}

void
ConnectionManager::checkIterators()
{
   if (mReadIter != mReadHead->end())
   {
      mReadIter = mReadHead->begin();
   }

   if (mWriteIter != mWriteHead->end())
   {
      mWriteIter = mWriteHead->begin();
   }
}
