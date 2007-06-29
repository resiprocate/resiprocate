#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/ConnectionManager.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Inserter.hxx"

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
   mHead(),
   mWriteHead(ConnectionWriteList::makeList(&mHead)),
   mReadHead(ConnectionReadList::makeList(&mHead)),
   mLRUHead(ConnectionLruList::makeList(&mHead)),
   mConnectionIdGenerator(1) 
{
}

ConnectionManager::~ConnectionManager()
{
   while (!mAddrMap.empty())
   {
      delete mAddrMap.begin()->second;
   }
   assert(mReadHead->empty());
   assert(mWriteHead->empty());
   assert(mLRUHead->empty());
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

void
ConnectionManager::buildFdSet(FdSet& fdset)
{
   for (ConnectionReadList::iterator i = mReadHead->begin(); 
        i != mReadHead->end(); ++i)
   {
      fdset.setRead((*i)->getSocket());
      fdset.setExcept((*i)->getSocket());
   }

   for (ConnectionWriteList::iterator i = mWriteHead->begin(); 
        i != mWriteHead->end(); ++i)
   {
      fdset.setWrite((*i)->getSocket());
      fdset.setExcept((*i)->getSocket());
   }
}

void
ConnectionManager::addToWritable(Connection* conn)
{
   mWriteHead->push_back(conn);
}

void
ConnectionManager::removeFromWritable(Connection* conn)
{
   assert(!mWriteHead->empty());
   conn->ConnectionWriteList::remove();
}

void
ConnectionManager::addConnection(Connection* connection)
{
   connection->mWho.connectionId = ++mConnectionIdGenerator;
   //DebugLog (<< "ConnectionManager::addConnection() " << connection->mWho.connectionId  << ":" << connection->mSocket);
   
   mAddrMap[connection->mWho] = connection;
   mIdMap[connection->mWho.connectionId] = connection;

   mReadHead->push_back(connection);
   mLRUHead->push_back(connection);

   assert(mAddrMap.count(connection->mWho) == 1);
}

void
ConnectionManager::removeConnection(Connection* connection)
{
   //DebugLog (<< "ConnectionManager::removeConnection()");

   assert(!mReadHead->empty());

   mIdMap.erase(connection->mWho.connectionId);
   mAddrMap.erase(connection->mWho);

   connection->ConnectionReadList::remove();
   connection->ConnectionWriteList::remove();
   connection->ConnectionLruList::remove();
}

// release excessively old connections (free up file descriptors)
void
ConnectionManager::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;
   InfoLog(<< "recycling connections older than " << relThreshhold/1000.0 << " seconds");

   for (ConnectionLruList::iterator i = mLRUHead->begin();
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
}

// move to youngest
void
ConnectionManager::touch(Connection* connection)
{
   connection->ConnectionLruList::remove();
   mLRUHead->push_back(connection);
}

void
ConnectionManager::process(FdSet& fdset, Fifo<TransactionMessage>& fifo)
{
   // process the write list
   for (ConnectionWriteList::iterator writeIter = mWriteHead->begin();
	writeIter != mWriteHead->end(); )
   {
      Connection* currConnection = *writeIter;

      // update iterator to next first so that it can traverse safely
      // even if current one is removed from the list later
      ++writeIter;

      if (!currConnection)
	 continue;

      if (fdset.readyToWrite(currConnection->getSocket()))
      {
	 currConnection->performWrite();
      }
      else if (fdset.hasException(currConnection->getSocket()))
      {
	 int errNum = 0;
	 int errNumSize = sizeof(errNum);
	 getsockopt(currConnection->getSocket(), SOL_SOCKET, SO_ERROR, (char *)&errNum, (socklen_t *)&errNumSize);
	 InfoLog(<< "Exception writing to socket " << currConnection->getSocket() << " code: " << errNum << "; closing connection");
	 delete currConnection;
      }
   }

   // process the read list
   for (ConnectionReadList::iterator readIter = mReadHead->begin();
	readIter != mReadHead->end(); )
   {
      Connection* currConnection = *readIter; 

      // update iterator to next first so that it can traverse safely
      // even if current one is removed from the list later
      ++readIter;

      if (!currConnection)
	 continue;

      if ( fdset.readyToRead(currConnection->getSocket()) ||
	   currConnection->hasDataToRead() )
      {
	 fdset.clear(currConnection->getSocket());
         
	 std::pair<char*, size_t> writePair = currConnection->getWriteBuffer();
	 size_t bytesToRead = resipMin(writePair.second, 
				       static_cast<size_t>(Connection::ChunkSize));
         
	 assert(bytesToRead > 0);
	 int bytesRead = currConnection->read(writePair.first, bytesToRead);

	 DebugLog (<< "ConnectionManager::process() " << *currConnection 
		   << " bytesToRead=" << bytesToRead << " read=" << bytesRead);            
	 if (bytesRead > 0) 
	 {
	    currConnection->performRead(bytesRead, fifo);
	 }
	 else if (bytesRead == -1)
	 {
	    DebugLog(<< "Closing connection bytesRead=" << bytesRead);
	    delete currConnection;
	 }
	 else if ( bytesRead != 0 )
	 {
	    ErrLog(<< "encounter special error at " << *currConnection
		   << " bytesToRead=" << bytesToRead << " read=" << bytesRead);
	    delete currConnection;
	 }
      }
      else if (fdset.hasException(currConnection->getSocket()))
      {
	 int errNum = 0;
	 int errNumSize = sizeof(errNum);
	 getsockopt(currConnection->getSocket(), SOL_SOCKET, SO_ERROR, (char *)&errNum, (socklen_t *)&errNumSize);
	 InfoLog (<< "Exception reading from socket " << currConnection->getSocket() << " code: " << errNum << "; closing connection");
	 delete currConnection;
      }
   }
}
