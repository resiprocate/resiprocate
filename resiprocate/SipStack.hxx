#if !defined(RESIP_SIPSTACK_HXX)
#define RESIP_SIPSTACK_HXX

#include <set>
#include <iosfwd>

#include "resiprocate/os/TimeLimitFifo.hxx"
#include "resiprocate/os/Mutex.hxx"
#include "resiprocate/os/TransportType.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/SecurityTypes.hxx"
#include "resiprocate/StatisticsManager.hxx"
#include "resiprocate/TuSelector.hxx"
#include "resiprocate/os/Win32Export.hxx"

namespace resip 
{

class ApplicationMessage;
class Data;
class Message;
class Security;
class SipMessage;
class StatisticsManager;
class Tuple;
class Uri;
class TransactionUser;
class AsyncProcessHandler;

class RESIP_API SipStack
{
   public:
      // Set stateless=true, if you want to use the stack for a stateless proxy
      // (no transactions)
      SipStack(Security* security=0, AsyncProcessHandler* handler = 0, 
               bool stateless=false);

      virtual ~SipStack();

      // inform the transaction state machine processor that it should not
      // create any new transactions and to perform an orderly shutdown. When
      // the transactions are all terminated, return a ShutdownMessage to the TU
      void shutdown();

      class Exception : public BaseException
      {
         public:
            Exception(const char* msg, const char* file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "SipStack::Exception"; }
      };
      
      // Used by the application to add in a new transport
      // ipInterface parameter is used to specify which ethernet interface to
      // bind to. If set to Data::Empty, bind to all interfaces 
      // If port = 0, use DNS to lookup the port number for the specified
      // domain. Only allow messages to be sent as the specified domain. 
      // For default case, you can pass in domainname =
      // DnsUtil::getLocalDomainName().

      enum TransportProcessApproach
      {
         SharesStackProcessAndSelect,
         RunsInOwnThread
      };
      
      //factory method to construct built-in transports.  A Transport::Exception
      //will be thrown if the transport couldn't be added, usually because the
      //port was already bound. 
      void addTransport( TransportType protocol,
                         int port=0, 
                         IpVersion version=V4,
                         const Data& ipInterface = Data::Empty, 
                         const Data& sipDomainname = Data::Empty, // only used
                                                                  // for TLS
                                                                  // based stuff 
                         const Data& privateKeyPassPhrase = Data::Empty,
                         SecurityTypes::SSLType sslType = SecurityTypes::TLSv1,
                         TransportProcessApproach threadApproach = SharesStackProcessAndSelect);
      
      void addTransport( std::auto_ptr<Transport> transport);
      
      //this is the fifo subclasses of Transport should use for the rxFifo
      //cons. param
      Fifo<TransactionMessage>& stateMacFifo();      

      // used to add an alias for this sip element. e.g. foobar.com and boo.com
      // are both handled by this proxy. 
      // not threadsafe
      void addAlias(const Data& domain, int port);
      
      // return true if domain is handled by this stack. convenience for
      // Transaction Users. 
      bool isMyDomain(const Data& domain, int port) const;
      
      // get one of the names for this host (calls through to gethostbyname) and
      // is not threadsafe
      static Data getHostname();

      // get one of the IP address for this host (calls through to gethostbyname) and
      // is not threadsafe
      static Data getHostAddress();

      // get one of the Uris for this host
      // not threadsafe
      const Uri& getUri() const;

      /** interface for the TU to send a message.Loose Routing processing as per
          RFC3261 must be done before calling send by the TU. See
          Helper::processStrictRoute */
      void send(std::auto_ptr<SipMessage> msg, TransactionUser* tu=0);

      /** interface for the TU to send a message. makes a copy of the
          SipMessage. Caller is responsible for deleting the memory and may do
          so as soon as it returns. Loose Routing processing as per RFC3261 must
          be done before calling send by the TU. See Helper::processStrictRoute */
      void send(const SipMessage& msg, TransactionUser* tu=0);
      
      /** this is only if you want to send to a destination not in the route. You
          probably don't want to use it. */
      void sendTo(std::auto_ptr<SipMessage> msg, const Uri& uri, TransactionUser* tu=0);
      /** this is only if you want to send to a destination not in the route. You
          probably don't want to use it. */
      void sendTo(std::auto_ptr<SipMessage> msg, const Tuple& tuple, TransactionUser* tu=0);

      /** this is only if you want to send to a destination not in the route. You
          probably don't want to use it. message is copied. */
      void sendTo(const SipMessage& msg, const Uri& uri, TransactionUser* tu=0);
      /** this is only if you want to send to a destination not in the route. You
          probably don't want to use it. message is copied. */
      void sendTo(const SipMessage& msg, const Tuple& tuple,
                  TransactionUser* tu=0);

      /** makes the message available to the TU, TranasctionUser subclasses
          can just post to themselves. */
      void post(std::auto_ptr<ApplicationMessage> message);

      /** makes the message available to the TU later, TranasctionUser subclasses
          can just post to themselves. */
      void post(std::auto_ptr<ApplicationMessage>, 
                unsigned int secondsLater,
                TransactionUser* tu=0);

      /** makes the message available to the TU later, TranasctionUser subclasses
          can just post to themselves. */
      void postMS(std::auto_ptr<ApplicationMessage> message, 
                  unsigned int ms,
                  TransactionUser* tu=0);
      /** makes the message available to the TU, TranasctionUser subclasses
          can just post to themselves. message is copied. */
      void post(const ApplicationMessage& message);

      /** makes the message available to the TU later, TranasctionUser subclasses
          can just post to themselves. message is copied. */
      void post(const ApplicationMessage& message, unsigned int secondsLater,
                TransactionUser* tu=0);

      /** makes the message available to the TU later, TranasctionUser subclasses
          can just post to themselves. message is copied. */
      void postMS(const ApplicationMessage& message, unsigned int ms,
                  TransactionUser* tu=0);
      // Return true if the stack has new messages for the TU
      bool hasMessage() const;
      
      // applications posting non-sip messages must use receive any.
      // caller now owns the memory. returns 0 if nothing there
      SipMessage* receive(); 

      // May return TransactionTerminated* or SipMessage* or derived ApplicationMessage*
      Message* receiveAny(); 
      
      // build the FD set to use in a select to find out when process bust be
      // called again. This must be called prior to calling process. 
      virtual void buildFdSet(FdSet& fdset);
	
      // should call buildFdSet before calling process. This allows the
      // executive to give processing time to stack components. 
      virtual void process(FdSet& fdset);

      /// returns time in milliseconds when process next needs to be called 
      virtual unsigned int getTimeTillNextProcessMS() const;

      // Inform the TU that whenever a transaction has been terminated. 
      void registerForTransactionTermination();

      // Allow the application to specify strict routing behavior, by default
      // loose routing policy is used. 
      void enableStrictRouting(bool strict=true) { mStrictRouting = strict; }
      bool isStrictRouting() const { return mStrictRouting; }

      void setStatisticsInterval(unsigned long seconds);

      // output current state of the stack - for debug
      std::ostream& dump(std::ostream& strm) const;
      
      Security* getSecurity() const;

      //adds a Tu to the tu selection chain. Tu do not call receive or
      //receiveAny, the SipStack will call postToTu on the appropriate
      //Tu. Messages no associated with a registered TU go into SipStack::mTuFifo
      void registerTransactionUser(TransactionUser&);
      void requestTransactionUserShutdown(TransactionUser&);
      void unregisterTransactionUser(TransactionUser&);
      
   private:
      void checkAsyncProcessHandler();

      /// if this object exists, it manages advanced security featues
      Security* mSecurity;
      AsyncProcessHandler* mAsyncProcessHandler;

      SipStack(const SipStack& copy);
      SipStack& operator=(const SipStack& rhs);         
      
      // fifo used to communicate between the TU (Transaction User) and stack 
      TimeLimitFifo<Message> mTUFifo;

      // timers associated with the application. When a timer fires, it is
      // placed in the mTUFifo
      mutable Mutex mAppTimerMutex;
      TuSelectorTimerQueue  mAppTimers;
      
      // Track stack statistics
      StatisticsManager mStatsManager;

      // All aspects of the Transaction State Machine / DNS resolver
      TransactionController mTransactionController;

      // store all domains that this stack is responsible for. Controlled by
      // addAlias and addTransport interfaces and checks can be made with isMyDomain()
      std::set<Data> mDomains;

      bool mStrictRouting;
      bool mShuttingDown;

      TuSelector mTuSelector;

      friend class Executive;
      friend class StatelessHandler;
      friend class StatisticsManager;
      friend class TestDnsResolver;
      friend class TestFSM;
      friend class TransactionState;
      friend class TransactionController;
      friend class TuSelector;
};

RESIP_API std::ostream& operator<<(std::ostream& strm, const SipStack& stack);
 
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
