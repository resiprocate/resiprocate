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

ConnectionMap::Connection*
ConnectionMap::add(const Transport::Tuple& who, Socket socket)
{
   assert(mConnections.find(who) == mConnections.end());

   Connection* connection = new Connection(who, socket);
   mConnections[who] = connection;
   touch(connection);

   DebugLog(<< "ConnectionMap::add: " << who << " fd: " << socket);
      
   return connection;
}

ConnectionMap::Connection*
ConnectionMap::get(const Transport::Tuple& who, int attempt)
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
   servaddr.sin_port = htons(who.port);
   servaddr.sin_addr = who.ipv4;

#if WIN32
   unsigned long block = 0;
   int errNoBlock = ioctlsocket( sock, FIONBIO , &block );
   assert( errNoBlock == 0 );
#else
   int flags  = fcntl( sock, F_GETFL, 0);
   int errNoBlock = fcntl(sock, F_SETFL, flags| O_NONBLOCK );
   assert( errNoBlock == 0 );
#endif
   
   int e = connect( sock, (struct sockaddr *)&servaddr, sizeof(servaddr) );
   if ( e == -1 ) 
   {
      int err = errno;
      DebugLog( << "Error on connect to " << who << ": " << strerror(err));
      return 0;
   }
   
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
   : mYounger(0),
     mOlder(0),
     mSocket(0),
     mLastUsed(0),
     mState(NewMessage)
{
}

ConnectionMap::Connection::Connection(const Transport::Tuple& who,
                                      Socket socket)
   : mYounger(0),
     mOlder(0),
     mWho(who),
     mSocket(socket),
     mLastUsed(Timer::getTimeMs()),
     mState(NewMessage)
{}

ConnectionMap::Connection::~Connection()
{
   remove();
//   shutdown(mSocket, SHUT_RDWR);
   ::close(mSocket);
}

ConnectionMap::Connection* 
ConnectionMap::Connection::remove()
{
   Connection* next = mYounger;

   if (mYounger != 0 && mOlder != 0)
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
   
std::pair<char* const, size_t> 
ConnectionMap::Connection::getWriteBuffer()
{
   if (mState == NewMessage)
   {
      mBuffer = new char [Connection::ChunkSize];
      mBufferSize = Connection::ChunkSize;
      mBufferPos = 0;
   }
   return make_pair(mBuffer + mBufferPos, mBufferSize - mBufferPos);
}

char* 
ConnectionMap::Connection::connectionStates[ConnectionMap::Connection::MAX] = { "NewMessage", "ReadingHeaders", "PartialBody" };

bool
ConnectionMap::Connection::process(size_t bytesRead, Fifo<Message>& fifo)
{
//   DebugLog(<< "In State: " << connectionStates[mState]);
   switch(mState)
   {
      case NewMessage:
      {
         mPreparse.reset();
         mMessage = new SipMessage();
         mMessage->setSource(mWho);
      }
      case ReadingHeaders:
      {
         if (mPreparse.process(*mMessage, mBuffer, mBufferPos + bytesRead) != 0)
         {
            delete mBuffer;
            mBuffer = 0;
            delete mMessage;
            mMessage = 0;
            return false;
         }
         if (!mPreparse.isDataAssigned())
         {
            mBufferPos += bytesRead;
            if (mBufferPos == mBufferSize)
            {
               size_t newBufferSize = size_t(mBufferSize*1.5);
               char* largerBuffer = new char[newBufferSize];
               memcpy(largerBuffer, mBuffer, mBufferSize);
               delete[] mBuffer;
               mBuffer = largerBuffer;
               mBufferSize = newBufferSize;
            }
            mState = ReadingHeaders;
         }
         else if (mPreparse.isFragmented())
         {
            mMessage->addBuffer(mBuffer);
            int overHang = mBufferPos + bytesRead - mPreparse.nDiscardOffset();
            size_t size = max((size_t)(overHang*1.5), (size_t)Connection::ChunkSize);
            char* newBuffer = new char[size];
         
            memcpy(newBuffer, mBuffer + mPreparse.nDiscardOffset(), overHang);
            mBuffer = newBuffer;
            mBufferPos = overHang;
            mBufferSize = size;
            mState = ReadingHeaders;
         }
         else if (mPreparse.isHeadersComplete())
         {         
            mMessage->addBuffer(mBuffer);
            size_t contentLength = mMessage->header(h_ContentLength).value();
            
            if (mBufferPos + bytesRead - mPreparse.nDiscardOffset() >= contentLength)
            {
               mMessage->setBody(mBuffer + mPreparse.nDiscardOffset(), contentLength);
               fifo.add(mMessage);

               int overHang = (mBufferPos + bytesRead) - (mPreparse.nDiscardOffset() + contentLength);
               size_t size = max((size_t)(overHang*1.5), (size_t)Connection::ChunkSize);
               char* newBuffer = new char[size];
               memcpy(newBuffer, mBuffer + mPreparse.nDiscardOffset() + contentLength, overHang);
               mBuffer = newBuffer;
               mBufferPos = overHang;
               mBufferSize = size;

               mState = NewMessage;
            }
            else
            {
               mBufferPos += bytesRead;
               char* newBuffer = new char[contentLength];
               memcpy(newBuffer, mBuffer + mPreparse.nDiscardOffset(), mBufferPos - mPreparse.nDiscardOffset());
               mBufferPos = mBufferPos - mPreparse.nDiscardOffset();
               mBufferSize = contentLength;
               mBuffer = newBuffer;
            
               mState = PartialBody;
            }
         }
         else
         {
            assert(0);
         }
         break;
      }
      case PartialBody:
      {
         size_t contentLength = mMessage->header(h_ContentLength).value();
         mBufferPos += bytesRead;
         if (mBufferPos == contentLength)
         {
            mMessage->setBody(mBuffer, contentLength);
            fifo.add(mMessage);
            
            mState = NewMessage;
         }
         break;
      }
      default:
         assert(0);
   }
   return true;
}
            
            
std::ostream& 
Vocal2::operator<<(std::ostream& strm, const Vocal2::ConnectionMap::Connection& c)
{
   strm << "CONN: " << c.getSocket() << " " << c.mWho;
   return strm;
}

