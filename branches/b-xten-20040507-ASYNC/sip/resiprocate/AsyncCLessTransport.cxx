#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <memory>
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DnsUtil.hxx"
#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/AsyncCLessTransport.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/ParseBuffer.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace std;
using namespace resip;


AsyncCLessTransport::AsyncCLessTransport(Fifo<TransactionMessage>& rxFifo, ExternalAsyncCLessTransport* eaTransport, 
					 bool ownedByMe)
:  Transport(rxFifo, eaTransport->boundAddress()),
   mTransportType(eaTransport->transportType()),
   mExternalTransport(eaTransport), 
   mOwnedByMe(ownedByMe)
{
   mExternalTransport->setHandler(this);
   mTuple.setType(mTransportType);
}



AsyncCLessTransport::~AsyncCLessTransport()
{
  if (mOwnedByMe)
  {
     delete mExternalTransport;
  }
}

void 
AsyncCLessTransport::transmit(const Tuple& dest, const Data& pdata, const Data& tid)
{
   unsigned char* bytes = new unsigned char[pdata.size()];
   memcpy(bytes, pdata.data(), pdata.size());

   mExternalTransport->send(dest.toGenericIPAddress(), bytes, pdata.size());
}

void 
AsyncCLessTransport::handleReceive(AsyncCLessReceiveResult res)
{
   if (!res.success())
   {
      //handle error code here...eventually
      delete [] res.bytes;
      return;
   }
   if (res.length + 1 >= MaxBufferSize)
   {
      InfoLog(<<"Datagram exceeded max length " << MaxBufferSize);
      delete [] res.bytes;
      return;
   }

   //extra characters for MsgHeaderCharacter
   char* buffer = new char[res.length + MsgHeaderScanner::MaxNumCharsChunkOverflow];
   memcpy(buffer, res.bytes, res.length);
   delete [] res.bytes;
   int len = res.length;

   // !dcm!  code from UdpTransport, eventually refactor
   //  buffer[len]=0; // null terminate the buffer string just to make debug easier and reduce errors

   //DebugLog ( << "UDP Rcv : " << len << " b" );
   //DebugLog ( << Data(buffer, len).escaped().c_str());
   
   Tuple tuple(res.remoteAddress(), transport());
   SipMessage* message = new SipMessage(this);

   // Save all the info where this message came from
   tuple.transport = this;
   message->setSource(tuple);   
//   DebugLog (<< "Received from: " << tuple);
   
   // Tell the SipMessage about this datagram buffer.
   message->addBuffer(buffer);

   try
   {
      mMsgHeaderScanner.prepareForMessage(message);
      
      char *unprocessedCharPtr;
      if (mMsgHeaderScanner.scanChunk(buffer,
                                      len,
                                      &unprocessedCharPtr) !=
          MsgHeaderScanner::scrEnd)
      {
         DebugLog(<<"Scanner rejecting datagram as unparsable / fragmented from " << tuple);
         DebugLog(<< Data(buffer, len));
         delete message; 
         message=0; 
         return;
      }
      
      // no pp error
      int used = unprocessedCharPtr - buffer;
      
      if (used < len)
      {
         // body is present .. add it up.
         // NB. The Sip Message uses an overlay (again)
         // for the body. It ALSO expects that the body
         // will be contiguous (of course).
         // it doesn't need a new buffer in UDP b/c there
         // will only be one datagram per buffer. (1:1 strict)
         
         message->setBody(buffer+used,len-used);
         //DebugLog(<<"added " << len-used << " byte body");
      }
      
      if (!basicCheck(*message))
      {
         delete message; // cannot use it, so, punt on it...
         // basicCheck queued any response required
         message = 0;
         return;
      }
      stampReceived(message);
      mStateMachineFifo.add(message);
   }
   catch(ParseBuffer::Exception)
   {
      DebugLog(<<"Parser fail from " << tuple);
      DebugLog(<< Data(buffer, len));
      delete message; 
      message=0; 
      return;
   }
}


