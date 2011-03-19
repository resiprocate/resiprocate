#if !defined(RESIP_TRANSPORT_HXX)
#define RESIP_TRANSPORT_HXX

#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Compression.hxx"

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
 */
#define RESIP_TRANSPORT_FLAG_NOBIND      (1<<0)
#define RESIP_TRANSPORT_FLAG_RXALL       (1<<1)
#define RESIP_TRANSPORT_FLAG_TXALL       (1<<2)
#define RESIP_TRANSPORT_FLAG_KEEP_BUFFER (1<<3)
#define RESIP_TRANSPORT_FLAG_TXNOW       (1<<4)

class Transport
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line);
            const char* name() const { return "TransportException"; }
      };

      // portNum is the port to receive and/or send on
      Transport(Fifo<TransactionMessage>& rxFifo,
                const GenericIPAddress& address,
                const Data& tlsDomain = Data::Empty, //!dcm! where is this used?
                AfterSocketCreationFuncPtr socketFunc = 0,
                Compression &compression = Compression::Disabled
         );

      Transport(Fifo<TransactionMessage>& rxFifo,
                int portNum,
                IpVersion version,
                const Data& interfaceObj,
                const Data& tlsDomain = Data::Empty,
                AfterSocketCreationFuncPtr socketFunc = 0,
                Compression &compression = Compression::Disabled,
                unsigned transportFlags = 0
         );

      virtual ~Transport();

      virtual bool isFinished() const=0;

      // send a message on the wire. This may be called from outside
      // our thread context.
      virtual void send( const Tuple& tuple, const Data& data, const Data& tid, const Data &sigcompId = Data::Empty);
      virtual void process(FdSet& fdset) = 0;
      virtual void buildFdSet( FdSet& fdset) =0;
      virtual void setPollGrp(FdPollGrp *grp) = 0;

      void flowTerminated(const Tuple& flow);
      void keepAlivePong(const Tuple& flow);

      // called when transport failed
      void fail(const Data& tid,
            TransportFailure::FailureReason reason = TransportFailure::Failure,
            int subCode = 0);

      static void error(int e);

      // These methods are used by the TransportSelector
      const Data& interfaceName() const { return mInterface; }

      int port() const { return mTuple.getPort(); }
      bool isV4() const { return mTuple.isV4(); } //!dcm! -- deprecate ASAP
      IpVersion ipVersion() const { return mTuple.ipVersion(); }

      const Data& tlsDomain() const { return mTlsDomain; }
      const sockaddr& boundInterface() const { return mTuple.getSockaddr(); }
      const Tuple& getTuple() const { return mTuple; }

      virtual TransportType transport() const =0 ;
      virtual bool isReliable() const =0;
      virtual bool isDatagram() const =0;

      // return true here if the subclass has a specific contact value that it
      // wishes the TransportSelector to use.
      virtual bool hasSpecificContact() const { return false; }

      // Perform basic sanity checks on message. Return false
      // if there is a problem eg) no Vias. --SIDE EFFECT--
      // This will queue a response if it CAN for a via-less
      // request. Response will go straight into the TxFifo

      bool basicCheck(const SipMessage& msg);

      void makeFailedResponse(const SipMessage& msg,
                              int responseCode = 400,
                              const char * warning = 0);

      // mark the received= and rport parameters if necessary
      static void stampReceived(SipMessage* request);

      /**
      Returns true if this Transport should be included in the FdSet processing
      loop, false if the Transport will provide its own cycles.  If the Transport
      is going to provide its own cycles, the startOwnProcessing() and
      shutdown() will be called to tell the Transport when to process.

      @retval true will run in the SipStack's processing context
      @retval false provides own cycles, just puts messages in rxFifo
      */
      virtual bool shareStackProcessAndSelect() const=0;

      //transports that returned false to shareStackProcessAndSelect() shouldn't
      //put messages into the fifo until this is called
      //?dcm? avoid the received a message but haven't added a transport to the
      //TransportSelector race, but this might not be necessary.
      virtual void startOwnProcessing()=0;

      //only applies to transports that shareStackProcessAndSelect
      // Transmit should determine if it has any queue messages to send
      virtual bool hasDataToSend() const = 0;

      //overriding implementations should chain through to this
      //?dcm? pure virtual protected method to enforce this?
      virtual void shutdown()
      {
         // !jf! should use the fifo to pass this in
         mShuttingDown = true;
      }

      // also used by the TransportSelector.
      // requires that the two transports be
      bool operator==(const Transport& rhs) const;

      //# queued messages on this transport
      virtual unsigned int getFifoSize() const=0;

      void callSocketFunc(Socket sock);

      // called by Connection to deliver a received message
      virtual void pushRxMsgUp(TransactionMessage* msg);

      // set the receive buffer length (SO_RCVBUF)
      virtual void setRcvBufLen(int buflen) { };	// make pure?

      // Storing and retrieving transport specific record-route header
      virtual void setRecordRoute(const NameAddr& recordRoute) { mRecordRoute = recordRoute; mHasRecordRoute = true; }
      virtual bool hasRecordRoute() const { return mHasRecordRoute; }
      virtual const NameAddr& getRecordRoute() const { assert(mHasRecordRoute); return mRecordRoute; }

   protected:
      Data mInterface;
      Tuple mTuple;
      NameAddr mRecordRoute;
      bool mHasRecordRoute;

      Fifo<TransactionMessage>& mStateMachineFifo; // passed in
      bool mShuttingDown;

      //not a great name, just adds the message to the fifo in the synchronous(default) case,
      //actually transmits in the asyncronous case.  Don't make a SendData because asynchronous
      //transports would require another copy.
      virtual void transmit(const Tuple& dest, const Data& pdata, const Data& tid, const Data &sigcompId) = 0;

      void setTlsDomain(const Data& domain) { mTlsDomain = domain; }
   private:
      static const Data transportNames[MAX_TRANSPORT];
      friend EncodeStream& operator<<(EncodeStream& strm, const Transport& rhs);

      Data mTlsDomain;
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
