#if !defined(RESIP_ASYNC_TRANSPORT_INTERFACES_HXX)
#define RESIP_ASYNC_TRANSPORT_INTERFACES_HXX

#include "resiprocate/external/AsyncID.hxx"
#include "resiprocate/GenericIPAddress.hxx"
#include "resiprocate/os/transporttype.hxx"

namespace resip
{

//External, design may be too specific to make generic--any code that seems common can be factored out later.
//Connection creation and destruction currently happens outside of resiprocate.  Packaging up the requests would
//allow more flexibility, for behaviours like passing in a buffer, and asyncronously requesting reads instead of
//having all reads pushed in.
class ExternalAsyncTransport
{
   public:
      virtual GenericIPAddress boundAddress()=0;
      virtual TransportType transportType()=0;
};

class AsyncCLessReceiveResult : public AsyncResult
{
   public:
      AsyncCLessReceiveResult(const GenericIPAddress& source, 
                              char* bytesRead, int count) : 
         mSource(source),
         bytes(bytesRead), 
         length(count) 
      {}
      AsyncCLessReceiveResult(long errorCode) : AsyncResult(errorCode) {}
         
      GenericIPAddress remoteAddress() { return mSource; }
         
      char* bytes;
      int length;
         
   private:
      GenericIPAddress mSource;
};

class ExternalAsyncCLessTransportHandler
{
   public:
      virtual void handleReceive(AsyncCLessReceiveResult res)=0;
};

class ExternalAsyncCLessTransport : public ExternalAsyncTransport
{
   public:
      virtual void setHandler(ExternalAsyncCLessTransportHandler* handler)=0;  
      virtual void send(GenericIPAddress dest, char* byte, int count)=0;
};

//stream transport

class AsyncStreamResult : public AsyncResult
{
   public:
      AsyncStreamResult(AsyncID streamID) : mStreamID(streamID) {}
      AsyncStreamResult(AsyncID streamID, long errorCode) : AsyncResult(errorCode), mStreamID(streamID) {}
      AsyncID streamID() const { return mStreamID; }
   private:
      AsyncID mStreamID;
};

//?dcm? -- memory must be destroyed by the receiver, not automatic in a destructor as this should eventually be
//extended to the case where aysnc reads are read directly into the requestors buffer.  Also, some cleverness
//could be done to just turn this buffer into the buffer for a SIP message when connections are not in the partial
//read case
class AsyncStreamReadResult : public AsyncStreamResult
{
   public:
      AsyncStreamReadResult(AsyncID id, char* bytes, int count) : AsyncStreamResult(id), bytes(bytes), 
                                                                  length(count) {}

      AsyncStreamReadResult(AsyncID id, long errorCode) : AsyncStreamResult(id, errorCode), bytes(0), length(0) {}

      char* bytes;
      int length;
};

class ExternalAsyncStreamTransportHandler
{
   public:
    
      virtual void handleRead(AsyncStreamReadResult res)=0;

      //possibly a write refused, or a close with an error could be used instead;  for now, flow control
      //is pushed to the provider
      //virtual void handleWrite(AsyncID stream, int count)=0;
      virtual void handleAccept(AsyncStreamResult res)=0;
      virtual void handleConnect(AsyncStreamResult res)=0;
      virtual void handleClose(AsyncStreamResult res)=0;
};

class ExternalAsyncStreamTransport : public ExternalAsyncTransport
{
   public:
      virtual AsyncID generateAsyncID()=0;
      virtual void setHandler(ExternalAsyncStreamTransportHandler* handler)=0;
      virtual void connect(GenericIPAddress host, AsyncID stream)=0;
      virtual void write(AsyncID stream, char* bytes, int count)=0;
      virtual void close(AsyncID stream)=0;
};
}
  

#endif
