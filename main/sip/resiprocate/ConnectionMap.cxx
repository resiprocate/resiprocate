#include <sipstack/ConnectionMap.hxx>
#include <util/Socket.hxx>

#ifndef WIN32
#include <errno.h>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <arpa/inet.h>
#endif

using namespace Vocal2;

const UInt64 ConnectionMap::MinLastUsed = 10*1000;
const UInt64 ConnectionMap::MaxLastUsed = 10*60*60*1000;

ConnectionMap::ConnectionMap()
{
   mPreYoungest.mOlder = &mPostOldest;
   mPostOldest.mYounger = &mPreYoungest;
}

void
ConnectionMap::touch(Connection* connection)
{
   connection->mLastUsed = Timer::getTimeMs();

   connection->remove();
   connection->mOlder = mPreYoungest.mOlder;
   connection->mYounger = &mPreYoungest;
   mPreYoungest.mOlder = connection;
}

ConnectionMap::Connection*
ConnectionMap::add(Transport::Tuple who, Socket socket)
{
   assert(mConnections.find(who) == mConnections.end());

   Connection* connection = new Connection(who, socket);
   mConnections[who] = connection;
   touch(connection);
   return connection;
}

ConnectionMap::Connection*
ConnectionMap::get(Transport::Tuple who, int attempt)
{
   Map::const_iterator i = mConnections.find(who);
   if (i != mConnections.end())
   {
      return i->second;
   }
   
   // attempt to open
   int sock = socket( AF_INET, SOCK_STREAM, 0 );
   if ( sock == -1 )
   {
      if (attempt > ConnectionMap::MaxAttempts)
      {
         return 0;
      }

      // !dlb! does the file descriptor become available immediately?
      gc(ConnectionMap::MinLastUsed);
      return get(who, attempt+1);
   }
   
   struct sockaddr_in servaddr;
   
   memset( &servaddr, sizeof(servaddr), 0 );
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = who.port;
   servaddr.sin_addr = who.ipv4;
   
   Socket e = connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
   if ( e == -1 ) 
   {
      // !cj! do error printouets 
      //int err = errno;
      return 0;
   }
   
   // succeeded, add the connection
   return add(who, sock);
}

void
ConnectionMap::close(Transport::Tuple who)
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

ConnectionMap::Connection::Connection()
   : mSocket(0),
     mLastUsed(0),
     mYounger(0),
     mOlder(0)
{
}

ConnectionMap::Connection::Connection(Transport::Tuple who,
                                      Socket socket)
   : mWho(who),
     mSocket(socket),
     mLastUsed(Timer::getTimeMs()),
     mYounger(0),
     mOlder(0)
{
}

ConnectionMap::Connection::~Connection()
{
   remove();
   shutdown(mSocket,2);
}

ConnectionMap::Connection* 
ConnectionMap::Connection::remove()
{
   Connection* next = mYounger;

   if (mYounger != 0 || mOlder != 0)
   {
      assert(mYounger != 0);
      assert(mOlder != 0);

      mYounger->mOlder = mOlder;
      mOlder->mYounger = mYounger;
      
      mYounger = 0;
      mOlder = 0;
   }
   
   return next;
}
   
