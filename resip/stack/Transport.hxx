#if !defined(RESIP_TRANSPORT_HXX)
#define RESIP_TRANSPORT_HXX

#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/FdSetIOObserver.hxx"
#include "rutil/ProducerFifoBuffer.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/TcpConnectState.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Compression.hxx"
#include "resip/stack/SendData.hxx"
#include "rutil/SharedPtr.hxx"
namespace resip
{

class TransactionMessage;
class SipMessage;
class Connection;
class Compression;
class FdPollGrp;

/**
 * TransportFlags is bit-mask that can be set when creating a transport.
 * All flags default to "off" to preserve "traditional" resip behavior.
 * Not all Transports support all flags. All options are experimental.
 * The flags are:
 * NOBIND:
 *    On transports that support it (TCP/TLS), do not bind a listening
 *    socket; thus no in-bound connections are possible. This can conserve
 *    ports and avoid port conflicts in pure-outbound (NAT'd) cases.
 * RXALL:
 *    When receiving from socket, read all possible messages before
 *    returning to event loop. This allows higher thruput. Without this flag,
 *    only one message is read at a time, which is slower over-all, but
 *    is more "even" and balanced.
 * TXALL:
 *    When transmitting to a socket, write all possible messages before
 *    returning to event loop. This allows higher thruput but burstier.
 * KEEP_BUFFER:
 *    With this flag, Transports will keep receive and transmit buffers
 *    allocated even when not in use. This increases memory utilization
 *    but speeds things up. Without this flag, the buffer is released
 *    when not in used.
 * TXNOW:
 *    When a message to transmit is posted to Transport's transmit queue
 *    immediately try sending it. This should have less latency
 *    and less overhead with select/poll stuff, but will have deeper
 *    call stacks.
 * OWNTHREAD:
 *    Specifies whether this Transport object has its own thread (ie; if
 *    set, the TransportSelector should not run the select/poll loop for
 *    this transport, since that is another thread's job)
 */
#define RESIP_TRANSPORT_FLAG_NOBIND      (1<<0)
#define RESIP_TRANSPORT_FLAG_RXALL       (1<<1)
#define RESIP_TRANSPORT_FLAG_TXALL       (1<<2)
#define RESIP_TRANSPORT_FLAG_KEEP_BUFFER (1<<3)
#define RESIP_TRANSPORT_FLAG_TXNOW       (1<<4)
#define RESIP_TRANSPORT_FLAG_OWNTHREAD   (1<<5)

/**
   @brief The base class for Transport classes.

   A Transport presents layer 4 of the OSI model, the transport layer.
   For IP-based protocols, this means that a Transport object has an
   IP address (v4 or v6), a transport layer protocol (UDP or TCP/TLS),
   and a port number.  These are managed through the Transport's Tuple
   member.

*/
class Transport : public FdSetIOObserver
{
   public:
    class SipMessageLoggingHandler
      {
      public:
          virtual ~SipMessageLoggingHandler(){}
          virtual void outboundMessage(const Tuple &source, const Tuple &destination, const SipMessage &msg) = 0;
          // Note:  retranmissions store already encoded messages, so callback doesn't send SipMessage it sends
          //        the encoded version of the SipMessage instead.  If you need a SipMessage you will need to
          //        re-parse back into a SipMessage in the callback handler.
          virtual void outboundRetransmit(const Tuple &source, const Tuple &destination, const SendData &data) {}
          virtual void inboundMessage(const Tuple& source, const Tuple& destination, const SipMessage &msg) = 0;
      };

      void setSipMessageLoggingHandler(SharedPtr<SipMessageLoggingHandler> handler) { mSipMessageLoggingHandler = handler; }
      SipMessageLoggingHandler* getSipMessageLoggingHandler() { return 0 != mSipMessageLoggingHandler.get() ? mSipMessageLoggingHandler.get() : 0; }

      /**
         @brief General exception class for Transport.

         This would be thrown if there was an attempt to bind to a port
         that is already in use.
      */
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "TransportException"; }
      };
      
      /**
         @param rxFifo the TransactionMessage Fifo that will receive
         any ConnectionTerminated or TransportFailure messages.
      
         @param tlsDomain the domain name of the Transport
      
         @param socketFunc subclassers can call this function after
         the socket is created.  This is not currently used by
         Transport.
      */
      Transport(Fifo<TransactionMessage>& rxFifo, 
                const GenericIPAddress& address,
                const Data& tlsDomain = Data::Empty, // !dcm! where is this used?
                AfterSocketCreationFuncPtr socketFunc = 0,
                Compression &compression = Compression::Disabled
         );

      /**
         @param rxFifo the TransactionMessage Fifo that will receive
         any ConnectionTerminated or TransportFailure messages.
      
         @param interfaceObj a "presentation format" representation
         of the IP address of this transport
         @see Tuple::inet_ntop() for information about "presentation
         format"
      
         @param portNum is the port to receive and/or send on
      
         @param tlsDomain the domain name of the Transport

         @todo Note that because of InternalTransport's constructor,
         tlsDomain is always set to Data::Empty at construction time,
         in practice.
      
         @param socketFunc subclassers can call this function after
         the socket is created.  This is not currently used by
         Transport.
      */
      Transport(Fifo<TransactionMessage>& rxFifo, 
                int portNum, 
                IpVersion version, 
                const Data& interfaceObj,
                const Data& tlsDomain = Data::Empty,
                AfterSocketCreationFuncPtr socketFunc = 0,
                Compression &compression = Compression::Disabled,
                unsigned transportFlags = 0,
                const Data& netNs = Data::Empty);

      virtual ~Transport();

      /**
         @note Subclasses override this method by checking whether
         there are unprocessed messages on the TransactionMessage
         Fifo (that was passed in to the constructor).
      */
      virtual bool isFinished() const=0;
      
      std::auto_ptr<SendData> makeSendData( const Tuple& tuple, const Data& data, const Data& tid, const Data &sigcompId = Data::Empty);
      
      /**
         @todo !bwc! What we do with a SendData is flexible. It might make a
         copy, or send synchronously, or convert to another type,
         etc.

         @todo !bch! Should this be protected and not public?
               !bwc! TransportSelector uses this directly for retransmissions.
      */
      virtual void send(std::auto_ptr<SendData> data)=0;

      /**
         Called when a writer is done adding messages to the TxFifo; this is
         used to interrupt the select call if the Transport is running in its
         own thread. This does nothing if select is not currently blocking, so
         don't bother calling this from the same thread that selects on this
         Transport's fds. Default impl is a no-op.
      */
      virtual void poke(){};

      /**
         If there is work to do, this is the method that does it. If
         the socket is readable, it is read.  If the socket is
         writable and there are outgoing messages to be sent, they are
         sent.

         Incoming messages are parsed and dispatched to the relevant
         entity.  SIP messages will be posted to the
         TransactionMessage Fifo.

         @see sendData()

         @param fdset is the FdSet after select() has been called.
         @see FdSet::select()
      */
      virtual void process(FdSet& fdset) = 0;

      /**
         Adds the Transport's socket FD to the appropriate read or
         write sets as applicable.
      */
      virtual void buildFdSet( FdSet& fdset) =0;

      virtual unsigned int getTimeTillNextProcessMS(){return UINT_MAX;}

      /**
         Version of process to be invoked periodically when using callback-based 
         IO (via FdPollGrp).
      */
      virtual void process() = 0;

      virtual void setPollGrp(FdPollGrp *grp) = 0;

      /**
         Posts a ConnectionTerminated message to TransactionMessage
         Fifo.
      */
      void flowTerminated(const Tuple& flow);
      void keepAlivePong(const Tuple& flow);

      /**
         Posts a TransportFailure to the TransactionMessage Fifo.
      */
      void fail(const Data& tid,
            TransportFailure::FailureReason reason = TransportFailure::Failure,
            int subCode = 0);

      /**
         Posts a TcpConnectState to the TransactionMessage Fifo.
      */
      void setTcpConnectState(const Data& tid, TcpConnectState::State state);

      /**
         Generates a generic log for the platform specific socket
         error number.

         @param e the socket error number
      */
      static void error(int e);

      // These methods are used by the TransportSelector
      const Data& interfaceName() const { return mInterface; }

      int port() const { return mTuple.getPort(); } 
    
      /// @deprecated use ipVersion()
      bool isV4() const { return mTuple.isV4(); } // !dcm! -- deprecate ASAP

      IpVersion ipVersion() const { return mTuple.ipVersion(); }
    
      /**
         @return the domain name that will be used for TLS, to, for
         example, find the certificate to present in the TLS
         handshake.
      */
      const Data& tlsDomain() const { return mTlsDomain; }
      const sockaddr& boundInterface() const { return mTuple.getSockaddr(); }
      const Tuple& getTuple() const { return mTuple; }
    
      /// @return This transport's TransportType.
      const TransportType transport() const { return mTuple.getType(); }
      virtual bool isReliable() const =0;
      virtual bool isDatagram() const =0;

      /// @return net namespace in which Transport is bound
      const Data& netNs() const { return(mTuple.getNetNs()); }

      /**
         @return true here if the subclass has a specific contact
         value that it wishes the TransportSelector to use.
      */
      virtual bool hasSpecificContact() const { return false; }

      /**
         Perform basic sanity checks on message. Return false
         if there is a problem eg) no Vias.

         @note --SIDE EFFECT-- This will queue a response if it CAN
         for a via-less request. Response will go straight into the
         TxFifo
      */
      bool basicCheck(const SipMessage& msg);

      void makeFailedResponse(const SipMessage& msg,
                              int responseCode = 400,
                              const char * warning = 0);
      std::auto_ptr<SendData> make503(SipMessage& msg,
                                      UInt16 retryAfter);

      std::auto_ptr<SendData> make100(SipMessage& msg);
      void setRemoteSigcompId(SipMessage&msg, Data& id);
      // mark the received= and rport parameters if necessary
      static void stampReceived(SipMessage* request);

      /**
         Returns true if this Transport should be included in the
         FdSet processing loop, false if the Transport will provide
         its own cycles.  If the Transport is going to provide its own
         cycles, the startOwnProcessing() and shutdown() will be
         called to tell the Transport when to process.
      
         @retval true will run in the SipStack's processing context
         @retval false provides own cycles, just puts messages in rxFifo
      */
      virtual bool shareStackProcessAndSelect() const=0;

      /**
         transports that returned false to
         shareStackProcessAndSelect() shouldn't put messages into the
         fifo until this is called
       
         @todo ?dcm? avoid the received a message but haven't added a
         transport to the TransportSelector race, but this might not
         be necessary.
      */
      virtual void startOwnProcessing()=0;

      /// only applies to transports that shareStackProcessAndSelect 
      virtual bool hasDataToSend() const = 0;
      
      /**
         This starts shutting-down procedures for this Transport.  New
         requests may be denied while "mShuttingDown" is true.
         
         Overriding implementations should chain through to this.
       
         @todo ?dcm? pure virtual protected method to enforce this?
         @see basicCheck()
         @see isFinished()
      */
      virtual void shutdown()
      {
         // !jf! should use the fifo to pass this in
         mShuttingDown = true;
      }
      virtual bool isShuttingDown() { return mShuttingDown; }

      // also used by the TransportSelector.
      // requires that the two transports be
      bool operator==(const Transport& rhs) const;

      //# queued messages on this transport
      virtual unsigned int getFifoSize() const=0;

      void callSocketFunc(Socket sock);

      virtual void setCongestionManager(CongestionManager* manager)
      {
         mCongestionManager=manager;
      }

      CongestionManager::RejectionBehavior getRejectionBehaviorForIncoming() const
      {
         if(mCongestionManager)
         {
            return mCongestionManager->getRejectionBehavior(&mStateMachineFifo.getFifo());
         }
         return CongestionManager::NORMAL;
      }

      UInt32 getExpectedWaitForIncoming() const
      {
         return (UInt32)mStateMachineFifo.getFifo().expectedWaitTimeMilliSec()/1000;
      }

      // called by Connection to deliver a received message
      virtual void pushRxMsgUp(SipMessage* msg);

      // set the receive buffer length (SO_RCVBUF)
      virtual void setRcvBufLen(int buflen) { };	// make pure?

      inline unsigned int getKey() const {return mTuple.mTransportKey;} 
      inline void setKey(unsigned int pKey) { mTuple.mTransportKey = pKey;} // should only be called once after creation

   protected:

      Data mInterface;
      Tuple mTuple;

      CongestionManager* mCongestionManager;
      ProducerFifoBuffer<TransactionMessage> mStateMachineFifo; // passed in
      bool mShuttingDown;

      void setTlsDomain(const Data& domain) { mTlsDomain = domain; }
   private:
      static const Data transportNames[MAX_TRANSPORT];
      friend EncodeStream& operator<<(EncodeStream& strm, const Transport& rhs);

      Data mTlsDomain;
      SharedPtr<SipMessageLoggingHandler> mSipMessageLoggingHandler;

   protected:
      AfterSocketCreationFuncPtr mSocketFunc;
      Compression &mCompression;
      unsigned mTransportFlags;
};

EncodeStream& operator<<(EncodeStream& strm, const Transport& rhs);

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0
 *
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * ====================================================================
 *
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 * vi: set shiftwidth=3 expandtab:
 */
