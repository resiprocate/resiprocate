#if !defined(RESIP_SIPSTACK_HXX)
#define RESIP_SIPSTACK_HXX

#include <set>
#include <iosfwd>

#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/BaseException.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "resip/stack/StatisticsManager.hxx"
#include "resip/stack/TuSelector.hxx"
#include "rutil/dns/DnsStub.hxx"

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
class Compression;

class SipStack
{
   public:
      /** 
          Constructor

          @param security   Security Object required by the stack for TLS, DTLS, SMIME
                            and Identity-draft compliance.  If 0 is passed in 
                            the stack will not support these advanced security 
                            features.  The compile flag USE_SSL is also required.
                            The security object will be owned by the SipStack and
                            deleted in the SipStack destructor.  Default is 0. 

          @param handler    AsyncProcessHandler that will be invoked when Messages 
                            are posted to the stack.  For example:  SelectInterruptor.
                            Default is 0.

          @param stateless  This parameter does not appear to be used.

          @param socketFunc [PLEASE FILL THIS IN]

          @param compression Compression configuration object required for
                             SigComp. If set to 0, then SigComp compression
                             will be disabled.
      */
      SipStack(Security* security=0, 
               const DnsStub::NameserverList& additional = DnsStub::EmptyNameserverList,
               AsyncProcessHandler* handler = 0, 
               bool stateless=false,
               AfterSocketCreationFuncPtr socketFunc = 0,
               Compression *compression = 0);      

      virtual ~SipStack();

      /** 
         Inform the transaction state machine processor that it should not
         create any new transactions and to perform an orderly shutdown. When
         the transactions are all terminated, return a ShutdownMessage to the TU
      */
      void shutdown();

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            const char* name() const { return "SipStack::Exception"; }
      };

      /** 
         Used by the application to add in a new built-in transport.  The transport is
         created and then added to the Transport Selector.

         @throws Transport::Exception If the transport couldn't be added, usually
                                      because the port was already bound.

         @param protocol              TCP, UDP, TLS, DTLS, etc.

         @param port                  Specifies which port to bind to.  See sipDomainname
                                      parameter for more info.

         @param version               Protocol Version:  V4 or V6

         @param ipInterface           Specifies which ethernet interface to bind to. If set to 
                                      Data::Empty, bind to all interfaces.

         @param sipDomainname         For TLS only, if port = 0, use DNS to lookup the port number 
                                      for the specified domain. Only allow messages to 
                                      be sent as the specified domain.  For default case, 
                                      you can pass in domainname = DnsUtil::getLocalDomainName().

         @param privateKeyPassPhrase  Private key pass phrase used to decrypt private key 
                                      certificates.  Note:  For now this parameter is not used
                                      we are loading PKCS7 keys, so a pass phrase is not required.

         @param sslType               Version of the TLS specification to use:  SSLv23 or TLSv1
      */      
      Transport* addTransport( TransportType protocol,
                         int port=0, 
                         IpVersion version=V4,
                         StunSetting stun=StunDisabled,
                         const Data& ipInterface = Data::Empty, 
                         const Data& sipDomainname = Data::Empty, // only used
                                                                  // for TLS
                                                                  // based stuff 
                         const Data& privateKeyPassPhrase = Data::Empty,
                         SecurityTypes::SSLType sslType = SecurityTypes::TLSv1);
      
      /**
          Used to plug-in custom transports.  Adds the transport to the Transport
          Selector.

          @param transport Pointer to an externally created transport.  SipStack 
                           assumes ownership.
      */
      void addTransport( std::auto_ptr<Transport> transport);
      
      /** 
          Returns the fifo that subclasses of Transport should use for the rxFifo
          cons. param.

          @returns A fifo of TransactionMessage's
      */
      Fifo<TransactionMessage>& stateMacFifo();      

      /**
          Used to add an alias for this sip element. e.g. foobar.com and boo.com
          are both handled by this proxy.  Not threadsafe.  Alias is added to 
          internal list of Domains and can be checked with isMyDomain.

          @param domain   Domain name that this stack is responsible for.

          @param port     Port for domain that this stack is responsible for.
      */
      void addAlias(const Data& domain, int port);
      
      /** 
          Returns true if domain is handled by this stack.  Convenience for
          Transaction Users. 

          @param domain   Domain name to check.

          @param port     Port number to check.
      */
      bool isMyDomain(const Data& domain, int port) const;
      
      /**
          Returns true if port is handled by this stack.  Convenience for
          Transaction Users. 

          @param port     Port number to check.        
      */
      bool isMyPort(int port) const;

      /** 
          Get one of the names for this host (calls through to gethostbyname) and
          is not threadsafe.
      */
      static Data getHostname();

      /**
          Get one of the IP address for this host (calls through to gethostbyname) and
          is not threadsafe.
      */
      static Data getHostAddress();

      /** 
          Get one of the domains/ports that are handled by this stack in Uri form. 
          "sip:" scheme is assumed.
      */
      const Uri& getUri() const;

      /** 
          Interface for the TU to send a message.  Makes a copy of the
          SipMessage.  Caller is responsible for deleting the memory and may do
          so as soon as it returns.  Loose Routing processing as per RFC3261 must
          be done before calling send by the TU. See Helper::processStrictRoute

          @param msg SipMessage to send.

          @param tu  TransactionUser to send from.
      */
      void send(const SipMessage& msg, TransactionUser* tu=0);

      void send(std::auto_ptr<SipMessage> msg, TransactionUser* tu = 0);
      
      /** this is only if you want to send to a destination not in the route. You
          probably don't want to use it. */
      void sendTo(std::auto_ptr<SipMessage> msg, const Uri& uri, TransactionUser* tu=0);
      /** this is only if you want to send to a destination not in the route. You
          probably don't want to use it. */
      void sendTo(std::auto_ptr<SipMessage> msg, const Tuple& tuple, TransactionUser* tu=0);

      /**
          This is only if you want to send to a destination not in the route.
          Useful for implementing Outbound Proxy use.  Makes a copy of the
          SipMessage.  Caller is responsible for deleting the memory and may 
          do so as soon as it returns.

          @param msg SipMessage to send.

          @param uri Destination to send to, specified as a Uri.

          @param tu  TransactionUser to send from.
      */
      void sendTo(const SipMessage& msg, const Uri& uri, TransactionUser* tu=0);

      /**
          This is only if you want to send to a destination not in the route. 
          Useful for implementing Outbound Proxy use.  Makes a copy of the
          SipMessage.  Caller is responsible for deleting the memory and may 
          do so as soon as it returns.

          @param msg   SipMessage to send.

          @param tuple Destination to send to, specified as a Tuple.

          @param tu    TransactionUser to send from.
      */
      void sendTo(const SipMessage& msg, const Tuple& tuple,
                  TransactionUser* tu=0);

      /**
          This is only if you want to force send to only send over an existing 
          connection.  If there is no connection, then it will try the next tuple.  
          If there are no more Tuples to try, then a 503 is sent to the TU.  Makes 
          a copy of the SipMessage.  Caller is responsible for deleting the memory 
          and may  do so as soon as it returns.

          @param msg   SipMessage to send.

          @param tuple Destination to send to, specified as a Tuple.  A
                       connection to this destination must exist.  

          @param tu    TransactionUser to send from.
      */
      void sendOverExistingConnection(const SipMessage& msg, const Tuple& tuple,
                                      TransactionUser* tu=0);

      /**
          Makes the message available to the TU at some later time - specified in
          seconds.  
          Note:  TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param secondsLater Number of seconds before message is to be posted.

          @param tu    TransactionUser to post to.
      */
      void post(const std::auto_ptr<ApplicationMessage> message, 
                unsigned int secondsLater,
                TransactionUser* tu=0);

      /**
          Makes the message available to the TU at some later time - specified in
          milli-seconds.  
          Note: TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param ms      Number of milli-seconds before message is to be posted.

          @param tu      TransactionUser to post to.
      */
      void postMS(const std::auto_ptr<ApplicationMessage> message, 
                  unsigned int ms,
                  TransactionUser* tu=0);

      /**
          Makes the message available to the TU later.  Makes a copy of the
          Message.  Caller is responsible for deleting the memory and may 
          do so as soon as it returns.  Since the addition of TransactionUsers, 
          this method is deprecated.  Calling this will cause the TuSelector to 
          post to the old TuFifo that is not associated with any 
          TransactionUser.

          Note:  TranasctionUser subclasses can just post to themselves.
          
          @deprecated

          @param message ApplicationMessage to post
      */
      void post(const ApplicationMessage& message);

      /**
          Makes the message available to the TU at some later time - specified in
          seconds.  Makes a copy of the ApplicationMessage.  Caller is responsible 
          for deleting the memory and may do so as soon as it returns.  
          Note:  TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param secondsLater Number of seconds before message is to be posted.

          @param tu    TransactionUser to post to.
      */
      void post(const ApplicationMessage& message, unsigned int secondsLater,
                TransactionUser* tu=0);

      /**
          Makes the message available to the TU at some later time - specified in
          milli-seconds.  Makes a copy of the ApplicationMessage.  Caller is 
          responsible for deleting the memory and may do so as soon as it returns.  
          Note:  TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param ms      Number of milli-seconds before message is to be posted.

          @param tu      TransactionUser to post to.
      */
      void postMS(const ApplicationMessage& message, unsigned int ms,
                  TransactionUser* tu=0);

      /**
          Return true if the stack has new messages for the TU.  Since the addition 
          of TransactionUsers, this method is deprecated.  This only looks into the 
          old TuFifo that is not associated with any TransactionUser.

          @deprecated
      */
      bool hasMessage() const;
      
      /** 
          Retrieve a SipMessage off the old TuFifo.  Caller now owns the memory.  Returns 
          0 if nothing there.  Since the addition of TransactionUsers, this method 
          is deprecated.  This only looks into the old TuFifo that is not associated 
          with any TransactionUser.

          Note:  Applications posting non-sip messages must use receive any.  If non 
                 SipMessages are on the Fifo, then they are just deleted.
                    
          @deprecated

          @returns pointer to received SipMessage, 0 if nothing there.
      */
      SipMessage* receive(); 

      /** 
          Retrieve a Message off the old TuFifo.  Caller now owns the memory.  Returns 
          0 if nothing there.  Since the addition of TransactionUsers, this method 
          is deprecated.  This only looks into the old TuFifo that is not associated 
          with any TransactionUser.

          Note:  Applications posting non-sip messages must use receive any.  If non 
                 SipMessages are on the Fifo, then they are just deleted.
                    
          @deprecated

          @returns pointer to received Message, 0 if nothing there.  May return 
                   TransactionTerminated*, TimerMessage*, SipMessage* or derived 
                   ApplicationMessage* 
      */
      Message* receiveAny(); 

      /**  
          Build the FD set to use in a select to find out when process must be
          called again. This must be called prior to calling process.
          Note:  select must also be called on the fdset prior to process.

          @param fdset an empty or partially populated fdset, fd's are added
                       to the fdset on return
      */
      virtual void buildFdSet(FdSet& fdset);
	
      /**  
          This allows the executive to give processing time to stack components. 
          Must call buildFdSet and select before calling this.

          @param fdset a populated and 'select'ed fdset
      */
      virtual void process(FdSet& fdset);

      /// returns time in milliseconds when process next needs to be called 
      virtual unsigned int getTimeTillNextProcessMS(); 

      /// Sets the interval that determines the time between Statistics messages
      void setStatisticsInterval(unsigned long seconds);

      /// output current state of the stack - for debug
      std::ostream& dump(std::ostream& strm) const;
      
      /// Returns a pointer to the embedded Security object, 0 if not set
      Security* getSecurity() const;

      /** 
          Adds a TU to the TU selection chain.  Tu's do not call receive or
          receiveAny, the SipStack will call postToTu on the appropriate
          Tu. Messages not associated with a registered TU go into SipStack::mTuFifo.
      */
      void registerTransactionUser(TransactionUser&);

      /// Queue a shutdown request to the specified TU
      void requestTransactionUserShutdown(TransactionUser&);

      /// Removes a TU from the TU selection chain
      void unregisterTransactionUser(TransactionUser&);

      /**
          Register a handler with the DNS Interface for notifications of when a Dns 
          Resource Record has been blacklisted.

          @param rrType Resource Record type you are interested in receiving 
                        notifications for

          @param BlackListListener Class implementing the onBlacklisted callback 
                                   event sink defined in BlackListListener
      */
      void registerBlacklistListener(int rrType, DnsStub::BlacklistListener*);

      /**
          Removed a registered BlacklistListener handler from the DNS Interface
          for a particualr Resource Record type and handler pointer.

          @param rrType Resource Record type

          @param BlackListListener Pointer to the class implementing the 
                                   BlackListListener event sink
      */
      void unregisterBlacklistListener(int rrType, DnsStub::BlacklistListener*);

      DnsStub& getDnsStub() const;

      /** 
          Specify which enum domains will be searched when sending to URIs that
          return true to Uri::isEnumSearchable(). An enum query will be done for
          each suffix in parallel. 
      */
      void setEnumSuffixes(const std::vector<Data>& suffixes);

      /**
          Clear the DNS Cache
      */
      void clearDnsCache();

      /**
          Log the DNS Cache to WarningLog for Debugging
      */
      void logDnsCache();

      /**
          Enable Statistics Manager.  SIP Statistics will be collected and 
          dispatched periodically via a StatisticsMessage.  Note:  By default 
          the Statistics Manager is enabled.
      */
      volatile bool& statisticsManagerEnabled();
      const bool statisticsManagerEnabled() const;
      
      void setContentLengthChecking(bool check)
      {
         SipMessage::checkContentLength=check;
      }

      Compression &getCompression() { return *mCompression; }

   private:
      /// Notify an async process handler - if one has been registered
      void checkAsyncProcessHandler();

      /// if this object exists, it manages advanced security featues
      Security* mSecurity;

      DnsStub* mDnsStub;

      /// If this object exists, it manages compression parameters
      Compression* mCompression;

      /// if this object exists, it gets notified when ApplicationMessage's get posted
      AsyncProcessHandler* mAsyncProcessHandler;

      /// Disallow copy, by not implementing
      SipStack(const SipStack& copy);

      /// Disallow assignment, by not implementing
      SipStack& operator=(const SipStack& rhs);         
      
      /** fifo used to communicate between the TU (Transaction User) and stack 
          Note:  since the introduction of multiple TU's this Fifo should no 
          longer be used by most applications - each TU now owns it's own Fifo. */
      TimeLimitFifo<Message> mTUFifo;

      /// Protection for AppTimerQueue
      mutable Mutex mAppTimerMutex;

      /** timers associated with the application. When a timer fires, it is
          placed in the mTUFifo (if there is not associated TU), or it is placed
          on the fifo of the appropriate TU */
      TuSelectorTimerQueue  mAppTimers;
      
      /// Used to Track stack statistics
      StatisticsManager mStatsManager;

      /// All aspects of the Transaction State Machine / DNS resolver
      TransactionController mTransactionController;

      /** store all domains that this stack is responsible for. Controlled by
          addAlias and addTransport interfaces and checks can be made with isMyDomain() */
      std::set<Data> mDomains;

      /** store all ports that this stack is lisenting on.  Controlled by addTransport
          and checks can be made with isMyPort() */
      std::set<int> mPorts;

      bool mShuttingDown;
      volatile bool mStatisticsManagerEnabled;

      /// Responsible for routing messages to the correct TU based on installed rules
      TuSelector mTuSelector;

      AfterSocketCreationFuncPtr mSocketFunc;

      friend class Executive;
      friend class StatelessHandler;
      friend class StatisticsManager;
      friend class TestDnsResolver;
      friend class TestFSM;
      friend class TransactionState;
      friend class TransactionController;
      friend class TuSelector;
};

std::ostream& operator<<(std::ostream& strm, const SipStack& stack);

inline void 
SipStack::sendOverExistingConnection(const SipMessage& msg, const Tuple& tuple,
                                     TransactionUser* tu)
{
   assert(tuple.transport);
   assert(tuple.connectionId);   
   Tuple tup(tuple);
   tup.onlyUseExistingConnection = true;   
   sendTo(msg, tuple, tu);
}
 
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
