
#include <cassert>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/ConnectionMap.hxx"


#define VOCAL_SUBSYSTEM Subsystem::TRANSPORT


using namespace std;

using namespace Vocal2;


const UInt64 ConnectionMap::MinLastUsed = 10*1000;
const UInt64 ConnectionMap::MaxLastUsed = 10*60*60*1000;


ConnectionMap::ConnectionMap()
{
   mPreYoungest.mOlder = &mPostOldest;
   mPostOldest.mYounger = &mPreYoungest;
}

ConnectionMap::~ConnectionMap()
{
   for(Map::iterator it = mConnections.begin();
       it != mConnections.end(); it++)
   {
      delete it->second;
   }
   mConnections.clear();
}

void
ConnectionMap::touch(Connection* connection)
{
   connection->mLastUsed = Timer::getTimeMs();

   connection->remove();
   connection->mOlder = mPreYoungest.mOlder;
   mPreYoungest.mOlder->mYounger = connection;
   connection->mYounger = &mPreYoungest;
   mPreYoungest.mOlder = connection;
}

Connection*
ConnectionMap::add(Transport::Tuple& who, Socket socket)
{
   assert(mConnections.find(who) == mConnections.end());

   Connection* connection = new Connection(who, socket);
   who.connection = connection;
   mConnections[who] = connection;
   touch(connection);

   DebugLog(<< "ConnectionMap::add: " << who << " fd: " << int(socket) );
      
   return connection;
}

Connection*
ConnectionMap::get(const Transport::Tuple& who)
{
   Map::const_iterator i = mConnections.find(who);
   if (i != mConnections.end())
   {
      return i->second;
   }
   
   return 0;
}

void
ConnectionMap::close(const Transport::Tuple& who)
{
   Map::iterator i = mConnections.find(who);
   if (i != mConnections.end())
   {
      i->second->remove();
      mConnections.erase(i);
      delete i->second;
   }
}

void
ConnectionMap::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;

   // start with the oldest
   Connection* i = mPostOldest.mYounger;
   while (i != &mPreYoungest)
   {
      if (i->mLastUsed < threshhold)
      {
         DebugLog( << "ConnectionMap::gc: " << *i);
         Connection* old = i;
         i = i->remove();
         mConnections.erase(old->mWho);
         delete old;
      }
      else
      {
         break;
      }
   }
}

