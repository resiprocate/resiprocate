#if !defined(resipConnection_hxx)
#define resipConnection_hxx

#include <list>

#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Timer.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/Preparse.hxx"

namespace resip
{

class Message;
class Preparse;
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
            
      bool prepNextMessage(int bytesUsed, int bytesRead, Fifo<Message>& fifo, Preparse& preparse, int maxBufferSize);
      bool readAnyBody(int bytesUsed, int bytesRead, Fifo<Message>& fifo, Preparse& preparse, int maxBufferSize);

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
      Preparse mPreparse;
      
      friend class ConnectionMap;
};

std::ostream& 
operator<<(std::ostream& strm, const resip::Connection& c);

}

#endif
