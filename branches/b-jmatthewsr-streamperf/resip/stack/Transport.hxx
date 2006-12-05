#if !defined(RESIP_TRANSPORT_HXX)
#define RESIP_TRANSPORT_HXX

#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "rutil/Fifo.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Compression.hxx"

namespace resip
{

class TransactionMessage;
class SipMessage;
class Connection;
class Compression;

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
                Compression &compression = Compression::Disabled
         );

      virtual ~Transport();

      virtual bool isFinished() const=0;
      
      virtual void send( const Tuple& tuple, const Data& data, const Data& tid, const Data &sigcompId = Data::Empty);
      virtual void process(FdSet& fdset) = 0;
      virtual void buildFdSet( FdSet& fdset) =0;

      void connectionTerminated(ConnectionId id);
            
         
      void fail(const Data& tid, TransportFailure::FailureReason reason = TransportFailure::Failure); // called when transport failed
      
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

   protected:
      Data mInterface;
      Tuple mTuple;

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
};

EncodeStream& operator<<(EncodeStream& strm, const Transport& rhs);
/** ivr mod */
typedef bool (*TransportScreenFunction)(const GenericIPAddress &source);

class TransportScreenSingleton
{
public:	
    
	~TransportScreenSingleton()
	{
		Destroy();
	}
    static bool Create(TransportScreenFunction function)
	{
		if (!mInstance)
        {
            mInstance = new TransportScreenSingleton();
			mInstance->mFunction = function;
			mInstance->mDestroying = false;
			return true;
        }
		return false;
	}

	static bool Destroy(void)
	{
		if( mInstance && !mInstance->mDestroying)
		{			
			mInstance->mDestroying = true;
			delete mInstance;
			mInstance = 0;
		}
		return true;
	}

	static bool Screen(const GenericIPAddress &source)
	{
		if(!mInstance || !mInstance->mFunction)
		{
			return true;
		}
		
		return (mInstance->mFunction)(source);
	}
    	
private:    
	
    // Protection
	TransportScreenSingleton():mFunction(NULL),mDestroying(false){}
    
    // Data        
    static TransportScreenSingleton	*mInstance;
	TransportScreenFunction			mFunction;
	bool							mDestroying;
};

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
 */
