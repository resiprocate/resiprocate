#include <assert.h>
#include "sip2/sipstack/ConnectionMap.hxx"
#include "sip2/util/Socket.hxx"
#include "sip2/sipstack/Preparse.hxx"
#include "sip2/util/Logger.hxx"

#define VOCAL_SUBSYSTEM Subsystem::TRANSPORT

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


#include <iostream>
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
ConnectionMap::add(const Transport::Tuple& who, Socket socket)
{
   assert(mConnections.find(who) == mConnections.end());

   Connection* connection = new Connection(who, socket);
   mConnections[who] = connection;
   touch(connection);

   DebugLog(<< "ConnectionMap::add: " << who << " fd: " << socket);
      
   return connection;
}

Connection*
ConnectionMap::get(const Transport::Tuple& who, int attempt)
{
   Map::const_iterator i = mConnections.find(who);
   if (i != mConnections.end())
   {
      return i->second;
   }
   
   // attempt to open
   Socket sock = socket( AF_INET, SOCK_STREAM, 0 );
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
   servaddr.sin_port = htons(who.port);
   servaddr.sin_addr = who.ipv4;

  
   int e = connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
   if ( e == -1 ) 
   {
      int err = errno;
      DebugLog( << "Error on connect to " << who << ": " << strerror(err));
      return 0;
   }

   makeSocketNonBlocking(sock);

   // succeeded, add the connection
   return add(who, sock);
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

