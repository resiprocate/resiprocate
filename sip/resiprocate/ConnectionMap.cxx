#include <assert.h>
#include "sip2/sipstack/ConnectionMap.hxx"
#include "sip2/util/Socket.hxx"
#include "sip2/sipstack/Preparse.hxx"

#define VOCAL_SUBSYSTEM Subsystem::SIP

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
   : mYounger(0),
     mOlder(0),
     mSocket(0),
     mLastUsed(0),
     mState(NewMessage)
{
}

ConnectionMap::Connection::Connection(Transport::Tuple who,
                                      Socket socket)
   : mYounger(0),
     mOlder(0),
     mWho(who),
     mSocket(socket),
     mLastUsed(Timer::getTimeMs()),
     mState(NewMessage)
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
   

void
ConnectionMap::Connection::allocateBuffer(int maxBufferSize)
{
   if(mState == NewMessage)
   {
      mBuffer = new char [maxBufferSize];
      mBufferSize = maxBufferSize;
      mBytesRead = 0;
   }
}


bool
ConnectionMap::Connection::process(int bytesRead, Fifo<Message>& fifo, Preparse& preparse, int maxBufferSize)
{

   if (mState == NewMessage)
   {
      mMessage = new SipMessage();
      mMessage->setSource(mWho);
      cerr << "NewMessage state." << endl;
   }
   
   if (mState == PartialHeaderRead)
   {
      cerr << "Partial Header state." << endl;
   }

   if (mState == NewMessage || mState == PartialHeaderRead)
   { 
      int bytesUsed;
   
      mMessage->addBuffer(mBuffer);

      Preparse::Status status;
#if 0 // !ah! needs porting

      preparse.process(*mMessage, mBuffer, mBytesRead + bytesRead, bytesUsed, status);
#endif
      if (status & PreparseConst::stPreparseError)
      {
         delete mMessage;
         //socket cleanup code
         return false;
      }
      if (status & PreparseConst::stFragmented)
      {
         char* partialHeader = new char[(bytesRead - bytesUsed) + maxBufferSize];
         memcpy(partialHeader, mBuffer + bytesUsed, (bytesRead - bytesUsed));
         mBuffer = partialHeader;
         mBufferSize = bytesRead - bytesUsed + maxBufferSize;
         mBytesRead = bytesRead - bytesUsed;

         mState = PartialHeaderRead;
         return true;
      }
      if (status == PreparseConst::stHeadersComplete)
      {
         return readAnyBody(bytesUsed, bytesRead, fifo, preparse, maxBufferSize);
      }
      assert(0);  // we should never get here

      return false;
      
   }
   else if (mState == PartialBodyRead)
   {
      assert(bytesRead + mBytesRead <= mBufferSize);

      if (bytesRead + mBytesRead == mBufferSize)
      {
         mMessage->setBody(mBuffer, mBufferSize);
         fifo.add(mMessage);
         mState = NewMessage;
      }
      else
      {
         mBytesRead += bytesRead;
      }
     
      return true;
   }
   assert(0);
   
   return false;
   
}


bool
ConnectionMap::Connection::prepNextMessage(int bytesUsed, int bytesRead, Fifo<Message>& fifo, Preparse& preparse, int maxBufferSize)
{
   if(bytesUsed < bytesRead)
   {
      char* newMsg = new char[maxBufferSize];
      memcpy(newMsg, mBuffer + bytesUsed, bytesRead - bytesUsed);
   
      fifo.add(mMessage);
      
      mBuffer = newMsg;
      mState = NewMessage;
      return process(bytesRead - bytesUsed, fifo, preparse, maxBufferSize);
   } 
   else
   {
      mState = NewMessage;
      fifo.add(mMessage);
      return true;
   }
}


bool
ConnectionMap::Connection::readAnyBody(int bytesUsed, int bytesRead, Fifo<Message>& fifo, Preparse& preparse, int maxBufferSize)
{
   cerr << "ReadAnyBody" << endl;
   if (mMessage->exists(h_ContentLength))
   {
      int contentLength = mMessage->header(h_ContentLength).value();
      if (contentLength > 0)
      {
         if (contentLength <= (bytesRead - bytesUsed))
         {
            // complete body read
            cerr << "complete body read" << endl;
            char* completeBody = new char[contentLength];
            memcpy(completeBody, mBuffer + bytesUsed, bytesRead - bytesUsed);
            mMessage->setBody(completeBody, contentLength);
            
            return prepNextMessage(contentLength + bytesUsed, bytesRead, fifo, preparse, maxBufferSize);
         }
         else
         {
            // partial body
            cerr << "partial body read" << endl;
            char* partialBody = new char[contentLength];
            memcpy(partialBody, mBuffer + bytesUsed, bytesRead - bytesUsed);
            mBuffer = partialBody;
            mBytesRead = bytesRead - bytesUsed;
            
            mState = PartialBodyRead;
            mBufferSize = contentLength;
            
            return true;
         }
      }
      else
      {
         return prepNextMessage(bytesUsed, bytesRead, fifo, preparse, maxBufferSize);
      }
      
   }
   else
   {
      delete mMessage;
      mMessage = 0;
      
      return false;
      
   }
}
