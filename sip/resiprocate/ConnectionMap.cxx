#include <sipstack/ConnectionMap.hxx>

ConnectionMap::ConnectionMap()
   : mLastUsed(Timer::getTimeMs())
{
   mPreYoungest.mOlder = &mPostOldest;
   mPostOldest.mYounger = &mPreYoungest;
}

void
ConnectionMap::touch(Connection* connection)
{
   connection->mLastUsed = Timer::getTimeMs();

   connection->remove();
   connection->mOlder = mPreYoungest->mOlder;
   connection->mYounger = &mPreYoungest;
   mPreYoungest->mOlder = connection;
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
   socket = open( AF_INET, SOCK_STREAM, 0 );
   if ( socket == -1 )
   {
      // could not get a socket
      assert(0);
      // !cj! look at the erros and decide which return what 
   }
   
   struct scokaddr_in servaddr;
   
   memset( &servaddr, sizeof(servaddr), 0 );
   servaddr.sin_family = AF_INET;
   servaddr.sin_port = who.port;
   servaddr.sin_addr = who.addr;
   
   Socket e = connect( socket, (struct sockaddr *)servaddr, sizeof(servaddr) );
   if ( e == -1 ) 
   {
      int err = errno;
      assert( 0 );
      // !cj! do error printouets 
   }
   
   if (socket)
   {
      // succeeded, add the connection
      Connection* connection = new Connection(socket);
      mConnections[who] = connection;
      touch(connection);
      return connection;
   }

   if (attempt > ConnectionMap::MaxAttempts)
   {
      return 0;
   }

   // !dlb! does the file descriptor become available immediately?
   gc(Connection::MinLastUsed);
   return get(who, attempt+1);
}

ConnectionMap::close(Transport::Tuple who)
{
   Map::const_iterator i = mConnections.find(who);
   if (i != mConnections,end())
   {
      i->second->remove();
      mConnections.erase(i);
      delete i->second;
   }
}

ConnectionMap::gc(UInt64 relThreshhold)
{
   UInt64 threshhold = Timer::getTimeMs() - relThreshhold;

   // start with the oldest
   Connection* i = mPostOldest.mYounger;
   while (i != &mPreYoungest)
   {
      if (i->mLastUsed < threshhold)
      {
         Connection old = i;
         i = i->remove();
         mConnections.erase(i->mWho);
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
   : mSocket(socket),
     mWho(who),
     mLastUsed(Timer::getTimeMs()),
     mYounger(0),
     mOlder(0)
{
}

ConnectionMap::Connection::~Connection()
{
   remove();
   close(mSocket);
}

ConnectionMap::Connection* 
ConnectionMap::ConnectionMap::remove();
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
   
