#if !defined(RESIP_ASYNC_CONNECTION_HXX)
#define RESIP_ASYNC_CONNECTION_HXX 

/**
   !dlb!
   Relationship between buffer allocation and parsing is broken.

   If the read returns with a few bytes, a new buffer is allocated in performRead.

   performRead should handle allocation, read, and parsing. it should be the only
   public read accessor in Connection. read should be protected.

*/

#include "resiprocate/ConnectionBase.hxx"
#include "resiprocate/AsyncStreamTransport.hxx"

namespace resip
{
//write and connect requests are delegated to the connectionManager(for now)
class AsyncConnection : public ConnectionBase
{
   public:
      AsyncConnection(const Tuple& who, AsyncID streamID, AsyncStreamTransport& tport, 
                      bool fromAccept);

      AsyncConnection(const Tuple& who, AsyncID streamID, AsyncStreamTransport& tport);

      virtual ~AsyncConnection();
      
      virtual void write(const Tuple& dest, const Data& pdata, const Data& tid);
      void handleRead(char* bytes, int count, Fifo< TransactionMessage >& fifo);
      
      //succesful connection
      void handleConnectSuccess();

      bool hasCurrentTid() const { return !mCurrentTid.empty(); }
      
      const Data& getCurrentTid() const { return mCurrentTid; }
    
      const AsyncID& getAsyncStreamID() const { return mStreamID; }
    
      enum State
      {
         New = 0, 
         Opening,
         Connected,
         Closing,
         Closed
      };
         
      State getState() { return mState; }
         
   protected:
      AsyncConnection();
      AsyncID mStreamID;
      State mState;
      AsyncStreamTransport& mAsyncStreamTransport;
      Data mCurrentTid;
      std::list<SendData*> mQueue;
};
}

#endif
