#include <algorithm>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "sip2/sipstack/Connection.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::TRANSPORT

Connection::Connection()
   : mYounger(0),
     mOlder(0),
     mSocket(0),
     mLastUsed(0),
     mState(NewMessage)
{
}

Connection::Connection(const Transport::Tuple& who,
                       Socket socket)
   : mYounger(0),
     mOlder(0),
     mWho(who),
     mSocket(socket),
     mLastUsed(Timer::getTimeMs()),
     mState(NewMessage)
{}

Connection::~Connection()
{
   remove();
//   shutdown(mSocket, SHUT_RDWR);
   ::close(mSocket);
}

Connection* 
Connection::remove()
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
Connection::getWriteBuffer()
{
   if (mState == NewMessage)
   {
      mBuffer = new char [Connection::ChunkSize];
      mBufferSize = Connection::ChunkSize;
      mBufferPos = 0;
   }
   return std::make_pair(mBuffer + mBufferPos, mBufferSize - mBufferPos);
}

char* 
Connection::connectionStates[Connection::MAX] = { "NewMessage", "ReadingHeaders", "PartialBody" };

bool
Connection::process(size_t bytesRead, Fifo<Message>& fifo)
{
   DebugLog(<< "In State: " << connectionStates[mState]);
   switch(mState)
   {
      case NewMessage:
      {
         mPreparse.reset();
         mMessage = new SipMessage(SipMessage::FromWire);
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
               size_t newBufferSize = size_t(mBufferSize*3/2);
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
            size_t size = std::max((size_t)(overHang*3/2), (size_t)Connection::ChunkSize);
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
               DebugLog(<< "##Connection: " << *this << " received: " << *mMessage);
               
               Transport::stampReceived(mMessage);
               fifo.add(mMessage);

               int overHang = (mBufferPos + bytesRead) - (mPreparse.nDiscardOffset() + contentLength);
               size_t size = std::max((size_t)(overHang*3/2), (size_t)Connection::ChunkSize);
               char* newBuffer = new char[size];
               memcpy(newBuffer, mBuffer + mPreparse.nDiscardOffset() + contentLength, overHang);
               mBuffer = newBuffer;
               mBufferPos = 0;
               mBufferSize = size;

               mState = NewMessage;
               if (overHang > 0) 
               {
                  DebugLog (<< "Extra bytes after message: " << overHang);
                  DebugLog (<< Data(mBuffer, overHang));
                  
                  process(overHang, fifo);
               }
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
Vocal2::operator<<(std::ostream& strm, const Vocal2::Connection& c)
{
   strm << "CONN: " << c.getSocket() << " " << c.mWho;
   return strm;
}

