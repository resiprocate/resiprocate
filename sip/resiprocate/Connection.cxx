
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Connection.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Security.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

char 
Connection::connectionStates[Connection::MAX][32] = { "NewMessage", "ReadingHeaders", "PartialBody" };


Connection::Connection()
   : mYounger(0),
     mOlder(0),
     mTlsConnection(0),
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
     mTlsConnection(0),
     mSocket(socket),
     mLastUsed(Timer::getTimeMs()),
     mState(NewMessage)
{
   mWho.connection = this;
}

Connection::~Connection()
{
   remove();

   //shutdown(mSocket, SD_BOTH );

   if ( mTlsConnection )
   {
      delete mTlsConnection; mTlsConnection=0;
   }
   
   closesocket(mSocket);
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
   
std::pair<char*, size_t> 
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

bool
Connection::process(size_t bytesRead, Fifo<Message>& fifo)
{
   DebugLog(<< "In State: " << connectionStates[mState]);
   switch(mState)
   {
      case NewMessage:
      {
         mPreparse.reset();
         assert(mWho.transport);
         mMessage = new SipMessage(mWho.transport);
         DebugLog(<< "Connection::process setting source " << mWho);
         mMessage->setSource(mWho);
      }
      case ReadingHeaders:
      {
         if (mPreparse.process(*mMessage, mBuffer, mBufferPos + bytesRead) != 0)
         {
            WarningLog(<< "Discarding preparse!");
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
            size_t size = overHang*3/2;
            if ( size < Connection::ChunkSize )
            {
               size = Connection::ChunkSize;
            }
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

               mState = NewMessage;
               if (overHang > 0) 
               {
                  size_t size = overHang*3/2;
                  if ( size < Connection::ChunkSize )
                  {
                     size = Connection::ChunkSize;
                  }
                  char* newBuffer = new char[size];
                  memcpy(newBuffer, mBuffer + mPreparse.nDiscardOffset() + contentLength, overHang);
                  mBuffer = newBuffer;
                  mBufferPos = 0;
                  mBufferSize = size;
                  
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
            //DebugLog(<< "Data assigned, not fragmented, not complete");
            mMessage->addBuffer(mBuffer);
            mBuffer = new char[ChunkSize];
            mBufferPos = 0;
            mBufferSize = ChunkSize;
            mState = ReadingHeaders;
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
resip::operator<<(std::ostream& strm, const resip::Connection& c)
{
   strm << "CONN: " << int(c.getSocket()) << " " << c.mWho;
   return strm;
}

