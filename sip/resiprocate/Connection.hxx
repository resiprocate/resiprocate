#if !defined(RESIP_CONNECTION_HXX)
#define RESIP_CONNECTION_HXX 

#include <list>

#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/Transport.hxx"
#ifndef NEW_MSG_HEADER_SCANNER
#include "resiprocate/Preparse.hxx"
#else
#include <resiprocate/MsgHeaderScanner.hxx>
#endif

namespace resip
{

class Message;
class TlsConnection;

class Connection
{
   public:
      Connection(const Transport::Tuple& who, Socket socket);
      Connection();

      ~Connection();
            
      Socket getSocket() const {return mSocket;}
            
      bool process(size_t bytesRead, Fifo<Message>& fifo);

      std::pair<char*, size_t> getWriteBuffer();
            
      Connection* remove(); // return next youngest
      Connection* mYounger;
      Connection* mOlder;

      Transport::Tuple mWho;

      Data::size_type mSendPos;     //position in current message being sent
      std::list<SendData*> mOutstandingSends;
      SendData* mCurrent;

      TlsConnection* mTlsConnection;
      
      enum { ChunkSize = 2048 }; //!dcm! -- bad size, perhaps 2048-4096?
   private:
            
      SipMessage* mMessage;
      char* mBuffer;
      size_t mBufferPos;
      size_t mBufferSize;

      enum State
      {
         NewMessage = 0,
         ReadingHeaders,
         PartialBody,
         MAX
      };

      static char connectionStates[MAX][32];

      Socket mSocket;
      UInt64 mLastUsed;
            
      State mState;
#ifndef NEW_MSG_HEADER_SCANNER
      Preparse mPreparse;
#else
      MsgHeaderScanner mMsgHeaderScanner;
#endif
      
      friend class ConnectionMap;
};

std::ostream& 
operator<<(std::ostream& strm, const resip::Connection& c);

}

#endif
