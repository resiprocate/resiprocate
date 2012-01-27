/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

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
class DnsThread;
class Message;
class Security;
class SipMessage;
class StatisticsManager;
class Tuple;
class Uri;
class TransactionControllerThread;
class TransportSelectorThread;
class TransactionUser;
class AsyncProcessHandler;
class Compression;
class CongestionManager;


/**
   @brief This is the primary point of interaction between the app-layer and the
      stack.

   @details For a SipStack to be usable, transports must be added by calling
   the addTransport() method.

   The useful work done by SipStack occurs when
   SipStack::process(yourFdSet) is called.  A separate thread (such as
   StackThread) can be dedicated to this task, or it can be called
   from within a loop in the main thread of the executable.  Note that
   for the process method to do anything of value, an FdSet must be
   initialized by calling SipStack::buildFdSet(yourFdSet), and then
   FdSet::select() or FdSet::selectMilliSeconds() is called on the
   FdSet instance.  When process(yourFdSet) is called, the work on the
   active transports is handled.

   Graceful shutdown is accomplished by advising the SipStack to begin
   shutdown procedures via the shutdown() method.  The SipStack should
   continue to be serviced through the process() method until the
   Transaction User is informed by receiving a ShutdownMessage that
   the requested shutdown is complete.  
   
   @ingroup resip_crit
   @ingroup resip_config
*/
class SipStack
{
   public:
      /** 
          @brief Constructor

          @param security   Security Object required by the stack for TLS, DTLS, SMIME
                            and Identity-draft compliance.  If 0 is passed in 
                            the stack will not support these advanced security 
                            features.  The compile flag USE_SSL is also required.
                            The security object will be owned by the SipStack and
                            deleted in the SipStack destructor.  Default is 0. 
							
         @param threadedStack Whether to run the DNS code, 
               transaction-controller, and transport-selector in their own 
               threads. 
		  @param additional List of additional DNS servers if the first returns an
		                    error code

          @param handler    AsyncProcessHandler that will be invoked when Messages 
                            are posted to the stack.  For example:  SelectInterruptor.
                            Default is 0.

          @param stateless  This parameter does not appear to be used.

          @param socketFunc A pointer to a function that will be called after a socket 
                            in the DNS or SIP transport layers of the stack has been 
                            created.  This callback can be used to control low level 
                            socket options, such as Quality-of-Service/DSCP.
                            Note:  For SIP TCP sockets there is one call for the listen
                            socket, and one (or two) calls for each connection created 
                            afterwards.  For each inbound TCP connection the first 
                            callback is called immediately before the socket is connected, 
                            and if configured it is called again after the connect call
                            has completed and before the first data is sent out.  
                            On some OS's you cannot set QOS until the socket is successfully 
                            connected.  To enable this behavior call:
                            Connection::setEnablePostConnectSocketFuncCall();

          @param compression Compression configuration object required for
                             SigComp. If set to 0, then SigComp compression
                             will be disabled.
							 
		  @todo additional was added to public and is not standard DNS behavior. 
      */
      SipStack(Security* security=0, 
               bool threadedStack=true,
               const DnsStub::NameserverList& additional = DnsStub::EmptyNameserverList,
               AsyncProcessHandler* handler = 0, 
               bool stateless=false,
               AfterSocketCreationFuncPtr socketFunc = 0,
               Compression *compression = 0);      

      virtual ~SipStack();

      void run();

      /** 
         @brief perform orderly shutdown
         @details Inform the transaction state machine processor that it should not
         create any new transactions and to perform an orderly shutdown. When
         the transactions are all terminated, return a ShutdownMessage to the TU
      */
      void shutdown();

      /**
         @brief Join any threads that the SipStack is running.
      */
      void join();

      /**
        @brief thrown when the stack is unable to function.
        @details For instance, the stack cannot process messages because
        there are no transports associated with the stack.
      */
      class Exception : public BaseException
      {
         public:
            /**
              @brief constructor
              @param msg The message to be put in the exception
              @param file The source code file where the exception got thrown
              @param line The line number in the file where the exception
               got thrown
              @note Used thus in SipStack.cxx:
               throw Exception("exception message", __FILE__, __LINE__);
              */
            Exception(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) {}

            /**
              @brief what gets called instead of the pure virtual in the base
              */
            const char* name() const { return "SipStack::Exception"; }
      };

      /**
         @brief Used by the application to add in a new built-in transport.
         The transport is created and then added to the Transport Selector.
         @throws Transport::Exception If the transport couldn't be added,
          usually because the port was already bound.
         @param protocol TCP, UDP, TLS, SCTP, DTLS, etc.
         @param port Specifies which port to bind to.
         @param version Protocol Version:  V4 or V6
		 @param stun specifies whether STUN is enabled.
         @param ipInterface Specifies which ethernet interface to bind to.
          If set to Data::Empty, bind to all interfaces. Binding to all 
          interfaces can impose a performance penalty, however, so it is 
          recommended that you bind to specific interfaces when using in 
          high-throughput deployments. The exception is for SCTP, if you want to 
          use multi-homing. Note:  Interfaces must be identified via IP address.
         @param sipDomainname Only allow messages to be sent as the specified
          domain.  For default case, you can pass in
          domainname = DnsUtil::getLocalDomainName().
         @param privateKeyPassPhrase Private key pass phrase used to decrypt
          private key certificates. Note: For now this parameter is not used
          we are loading PKCS7 keys, so a pass phrase is not required.
         @param sslType Version of the TLS specification to use: SSLv23 or TLSv1
         @note Generally, SCTP is faster than TCP, which is faster than UDP. UDP 
         can also be quite memory-intensive.
       */
      Transport* addTransport( TransportType protocol,
                         int port, 
                         IpVersion version=V4,
                         StunSetting stun=StunDisabled,
                         const Data& ipInterface = Data::Empty, 
                         const Data& sipDomainname = Data::Empty, // only used
                                                                  // for TLS
                                                                  // based stuff 
                         const Data& privateKeyPassPhrase = Data::Empty,
                         SecurityTypes::SSLType sslType = SecurityTypes::TLSv1,
                         unsigned transportFlags = 0,
                         bool hasOwnThread=false);
      
      /**
          @brief Used to plug-in custom transports.  Adds the transport to the Transport
          Selector.

          @param transport Pointer to an externally created transport.  SipStack 
                           assumes ownership.
      */
      void addTransport( std::auto_ptr<Transport> transport);
      
      /** 
          @brief Returns the fifo that subclasses of Transport should use for the rxFifo
          cons. param.

          @returns A fifo of TransactionMessage's
      */
      Fifo<TransactionMessage>& stateMacFifo();      

      /**
          @brief add an alias for this sip element
          
          @details Used to add an alias for this sip element. e.g. foobar.com and boo.com
          are both handled by this stack.  Not threadsafe.  Alias is added 
          to internal list of Domains and can be checked with isMyDomain.

          @param domain   Domain name that this stack is responsible for.

          @param port     Port for domain that this stack is responsible for.
          @ingroup resip_config
      */
      void addAlias(const Data& domain, int port);
      
      /** 
          @brief Returns true if domain is handled by this stack.  Convenience for
          Transaction Users. 

          @param domain   Domain name to check.

          @param port     Port number to check.If 0, it makes the search
                           port-insensitive.
      */
      bool isMyDomain(const Data& domain, int port=0) const;
      
      /** 
          @brief Get one of the names for this host (calls through to gethostbyname) and
          is not threadsafe.
      */
      static Data getHostname();

      /**
          @brief Get one of the IP address for this host (calls through to gethostbyname) and
          is not threadsafe.
      */
      static Data getHostAddress();

      /** 
          @brief Get one of the domains/ports that are handled by this stack in Uri form. 
          "sip:" scheme is assumed.
      */
      const Uri& getUri() const;

      /** 
          @brief allows a TU to send a message
          @details Interface for the TU to send a message.  Makes a copy of the
          SipMessage.  Caller is responsible for deleting the memory and may do
          so as soon as it returns.  Loose Routing processing as per RFC3261 must
          be done before calling send by the TU. See Helper::processStrictRoute

          @param msg SipMessage to send.

          @param tu  TransactionUser to send from.
          
          @sa TransactionUser
      */
      void send(const SipMessage& msg, TransactionUser* tu=0);

      void send(std::auto_ptr<SipMessage> msg, TransactionUser* tu = 0);
      
      /** @brief this is only if you want to send to a destination not in the route.
          @note You probably don't want to use it. */
      void sendTo(std::auto_ptr<SipMessage> msg, const Uri& uri, TransactionUser* tu=0);
      /** @brief this is only if you want to send to a destination not in the route.
          @note You probably don't want to use it. */
      void sendTo(std::auto_ptr<SipMessage> msg, const Tuple& tuple, TransactionUser* tu=0);

      /**
          @brief send a message to a destination not in the route
          @details This is only if you want to send to a destination not in the route.
          Useful for implementing Outbound Proxy use.  Makes a copy of the
          SipMessage.  Caller is responsible for deleting the memory and may 
          do so as soon as it returns.

          @param msg SipMessage to send.

          @param uri Destination to send to, specified as a Uri.

          @param tu  TransactionUser to send from.
      */
      void sendTo(const SipMessage& msg, const Uri& uri, TransactionUser* tu=0);

      /**
          @brief send a message to a destination not in the route      
          @details This is only if you want to send to a destination not in the route. 
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
          @brief force the a message out over an existing connection

          @details This is only if you want to force send to only send over an existing 
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
          @brief Makes the message available to the TU at some later time, specified in
          seconds.  
          
          @note  TransactionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param secondsLater Number of seconds before message is to be posted.

          @param tu    TransactionUser to post to.
      */
      void post( std::auto_ptr<ApplicationMessage> message, 
                unsigned int secondsLater,
                TransactionUser* tu=0);

      /**
          @brief Makes the message available to the TU at some later time, specified in
          milli-seconds.  
          
          @note TransactionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param ms      Number of milli-seconds before message is to be posted.

          @param tu      TransactionUser to post to.
      */
      void postMS( std::auto_ptr<ApplicationMessage> message, 
                  unsigned int ms,
                  TransactionUser* tu=0);

      /**
          @brief Makes the message available to the TU later
          
          @note Makes a copy of the Message.  Caller is responsible for deleting
          the memory and may  do so as soon as it returns.  

          @note  TranasctionUser subclasses can just post to themselves.
          
          @deprecated Since the addition of TransactionUsers, 
          this method is deprecated.  Calling this will cause the TuSelector to 
          post to the old TuFifo that is not associated with any  TransactionUser.

          @param message ApplicationMessage to post.
		  
		  @param tu TransactionUser to post to.
      */
      void post(const ApplicationMessage& message, TransactionUser* tu=0);

      /**
          @brief Makes the message available to the TU later.

          @note  TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post.
		  
		  @param tu TransactionUser to post to.
      */
      void post( std::auto_ptr<ApplicationMessage> message, TransactionUser* tu=0);

      /**
          @brief Makes the message available to the TU at some later time, specified in
          seconds.
          
          @note Makes a copy of the ApplicationMessage.  Caller is responsible 
          for deleting the memory and may do so as soon as it returns.  

          @note TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post.

          @param secondsLater Number of seconds before message is to be posted.

          @param tu    TransactionUser to post to.
      */
      void post(const ApplicationMessage& message, unsigned int secondsLater,
                TransactionUser* tu=0);

      /**
          @brief Makes the message available to the TU at some later time, specified in
          milli-seconds. 
          
          @note Makes a copy of the ApplicationMessage.  Caller is responsible for deleting
          the memory and may do so as soon as it returns.  

          @note TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post

          @param ms      Number of milli-seconds before message is to be posted.

          @param tu      TransactionUser to post to.
      */
      void postMS(const ApplicationMessage& message, unsigned int ms,
                  TransactionUser* tu=0);

      /**
         Tells the stack that the TU has abandoned a server transaction. This
         is provided to allow better behavior in cases where an exception is 
         thrown due to garbage in the request, and the code catching the 
         exception has no way of telling whether the original request is still
         around. This frees the TU of the obligation to respond to the request.
         @param tid The transaction identifier for the server transaction.
         @note This function is distinct from cancelClientInviteTransaction().
      */
      void abandonServerTransaction(const Data& tid);

      /**
         Tells the stack that the TU wishes to CANCEL an INVITE request. This 
         frees the TU of the obligation to keep state on whether a 1xx has come 
         in yet before actually sending a CANCEL request, and also the 
         obligation of forming the CANCEL request itself. This _does_ _not_ free
         the TU of the obligation to handle any INVITE/200 that come in (usually
         by sending an ACK to the 200, and then a BYE).
         @param tid The transaction identifier of the INVITE request sent.
      */
      void cancelClientInviteTransaction(const Data& tid);

      /**
         Administratively closes a given connection.
         @param peer The peer (ie; remote) address for the connection to be 
            closed. This may or may not have Tuple::mFlowKey set; if it is set,
            the connection will only be closed if both the peer address and the 
            flow-key (ie; fd) match.
         @note This function is async, since the transport code lives in its own 
               thread. As yet, there is no mechanism for confirming when the 
               connection is closed unless the TU is registered to receive 
               ConnectionTerminated messages (see 
               TransactionUser::isRegisteredForConnectionTermination() ).
      */
      void closeConnection(const Tuple& peer);

      /**
          @brief does the stack have new messages for the TU?
          @return return true if the stack has new messages for the TU.
          
          @deprecated Since the addition of TransactionUsers, this method is deprecated. 
          This only looks into the old TuFifo that is not associated with any TransactionUser.
          
      */
      bool hasMessage() const;
      
      /** 
          @brief retrieve a SipMessage off the old TuFifo
          
          @details Retrieve a SipMessage off the old TuFifo.  Caller now owns the memory.  Returns 
          0 if nothing there.  Since the addition of TransactionUsers, this method 
          is deprecated.  This only looks into the old TuFifo that is not associated 
          with any TransactionUser.

          @note  Applications posting non-sip messages must use receive any.  If non 
                 SipMessages are on the Fifo, then they are just deleted.
                    
          @deprecated

          @returns pointer to received SipMessage, 0 if nothing there.
      */
      SipMessage* receive(); 

      /** 
          @brief retrieve a SipMessage off the old TuFifo
          
          @details Retrieve a SipMessage off the old TuFifo.  Caller now owns the memory.  Returns 
          0 if nothing there.  Since the addition of TransactionUsers, this method 
          is deprecated.  This only looks into the old TuFifo that is not associated 
          with any TransactionUser.

          @note  Applications posting non-sip messages must use receive any.  If non 
                 SipMessages are on the Fifo, then they are just deleted.
                    
          @deprecated

          @returns pointer to received SipMessage, 0 if nothing there.
      */
      SipMessage* receive(int waitMs); 

      /** 
          @brief retrieve a message off the old TuFifo
          
          Retrieve a Message off the old TuFifo.  Caller now owns the memory.  Returns 
          0 if nothing there.  Since the addition of TransactionUsers, this method 
          is deprecated.  This only looks into the old TuFifo that is not associated 
          with any TransactionUser.

          @note Applications posting non-sip messages must use receive any.  If non 
                 SipMessages are on the Fifo, then they are just deleted.
                    
          @deprecated

          @returns pointer to received Message, 0 if nothing there.  May return 
                   TransactionTerminated*, TimerMessage*, SipMessage* or derived 
                   ApplicationMessage* 
      */
      Message* receiveAny(); 

      /**  
          @brief Build the FD set to use in a select to find out when process must be
          called again.

          @note This must be called prior to calling process.

          @note select must also be called on the fdset prior to process.

          @param fdset an empty or partially populated fdset, fd's are added
                       to the fdset on return
      */
      virtual void buildFdSet(FdSet& fdset);
	
      /**  
      
          @brief This allows the executive to give processing time to stack components. 

          @note Must call buildFdSet and select before calling this.

          @note The transports are serviced, and then timers are serviced.

          @param fdset a populated and 'select'ed fdset
      */
      virtual void process(FdSet& fdset);

      /** @brief returns time in milliseconds when process next needs to be called  **/
      virtual unsigned int getTimeTillNextProcessMS(); 

      /**
         @brief Sets the interval that determines the time between Statistics messages
         @ingroup resip_config
      */
      void setStatisticsInterval(unsigned long seconds);

      /** Installs a handler for the stacks internal StatisticsManager.  This handler is called before the 
        * default behavior.
        */
      void setExternalStatsHandler(ExternalStatsHandler *handler)
      {
         mStatsManager.setExternalStatsHandler(handler);
      }

      /** @brief output current state of the stack - for debug **/
      std::ostream& dump(std::ostream& strm) const;
      
      /** @brief Returns a pointer to the embedded Security object, 0 if not set **/
      Security* getSecurity() const;

      /** 
          @brief add a TU to the TU selection chain
          
          @details Adds a TU to the TU selection chain.  Tu's do not call receive or
          receiveAny, the SipStack will call postToTu on the appropriate
          Tu. Messages not associated with a registered TU go into SipStack::mTuFifo.
      */
      void registerTransactionUser(TransactionUser&);

      /** @brief Queue a shutdown request to the specified TU **/
      void requestTransactionUserShutdown(TransactionUser&);

      /** @brief Removes a TU from the TU selection chain **/
      void unregisterTransactionUser(TransactionUser&);

      /**
          @brief Register a MarkListener handler with the TransactionController.

          @param listener Class implementing the onMark() callback 
                                   event sink defined in MarkListener
          @ingroup resip_config
      */
      void registerMarkListener(MarkListener*);

      /**
          @brief Removed a registered MarkListener handler from the TransactionController.

          @param listener Class implementing the onMark() callback 
                                   event sink defined in MarkListener

          @ingroup resip_config
      */
      void unregisterMarkListener(MarkListener*);

      DnsStub& getDnsStub() const;

      /** 
          @brief Specify which enum domains will be searched when sending
          
          @details Specify which enum domains will be searched when sending to URIs that
          return true to Uri::isEnumSearchable(). An enum query will be done for
          each suffix in parallel.
          
          @ingroup resip_config
      */
      void setEnumSuffixes(const std::vector<Data>& suffixes);

      /**
          @brief Clear the DNS Cache
      */
      void clearDnsCache();

      /**
          @brief Log the DNS Cache to WarningLog for Debugging
      */
      void logDnsCache();

      /**
          @todo is this documented correctly? [!]
          @brief Enable Statistics Manager
          @details Enable Statistics Manager.  SIP Statistics will be collected and 
          dispatched periodically via a StatisticsMessage.
          @note By default the Statistics Manager is enabled.

          @ingroup resip_config
      */
      volatile bool& statisticsManagerEnabled();
      const bool statisticsManagerEnabled() const;

      /**
         Returns whether the stack is fixing corrupted/changed dialog 
         identifiers (ie, Call-Id and tags) in responses from the wire.

         @return Denotes whether the stack is fixing corrupted/changed dialog 
            identifiers.
         @see getFixBadDialogIdentifiers()
         @ingroup resip_config
      */
      bool getFixBadDialogIdentifiers() const 
      {
         return mTransactionController.mFixBadDialogIdentifiers;
      }

      /**
         Specify whether the stack should fix corrupted/changed dialog 
         identifiers (ie, Call-Id and tags) in responses from the wire. This is
         intended to help TransactionUsers that assume responses will come back
         with the same dialog identifiers that the request had (excepting of 
         course the remote tag for dialog-forming requests).

         @param pFixBadDialogIdentifiers Denotes whether the stack should fix
            corrupted/changed dialog identifiers.
         @note This is enabled by default, and is recommended for 
            dialog-stateful TransactionUsers.
         @ingroup resip_config
      */
      void setFixBadDialogIdentifiers(bool pFixBadDialogIdentifiers) 
      {
         mTransactionController.mFixBadDialogIdentifiers = pFixBadDialogIdentifiers;
      }

      inline bool getFixBadCSeqNumbers() const
      {
         return mTransactionController.getFixBadCSeqNumbers();
      }

      inline void setFixBadCSeqNumbers(bool pFixBadCSeqNumbers)
      {
         mTransactionController.setFixBadCSeqNumbers(pFixBadCSeqNumbers);
      }

      /**
         @todo should this be fixed to work with other applicable transports? []
         @brief Used to enable/disable content-length checking on datagram-based 
         transports.
         @details Used to enable/disable content-length checking on datagram-based 
         transports. If disabled, the stack will ignore the
         value of the Content-Length header, and assume that the end of the 
         payload is at the end of the datagram (and not before). If enabled, it
         will take the Content-Length seriously, log a warning if the 
         Content-Length is too short, and reject the message if the 
         Content-Length is too long.
         @param check Denotes whether we should check Content-Length.
         @note By default, Content-Length checking is enabled.
         @note only works on UDP at this time
         @ingroup resip_config
      */
      void setContentLengthChecking(bool check)
      {
         SipMessage::checkContentLength=check;
      }

      /**
         @brief Controls whether Contact headers are automatically filled out in 
          outgoing SIP traffic
         @details Controls whether Contact headers are automatically filled out in 
         outgoing SIP traffic, once the stack determines what interface the 
         message will be sent on.
         @param filloutContacts Denotes whether we should automatically fill out 
            Contact headers or not.
         @note By default, this behavior is enabled.
         @ingroup resip_config
      */
      inline void setFilloutContacts(bool filloutContacts)
      {
         mTransactionController.setFilloutContacts(filloutContacts);
      }
      
      /**
         @brief Returns whether Contact headers are automatically filled out in 
         outgoing SIP traffic
         @details Returns whether Contact headers are automatically filled out in 
         outgoing SIP traffic, once the stack determines what interface the 
         message will be sent on.
         @return Denotes whether we are configured to automatically fill out 
            Contact headers or not.
         @note By default, this behavior is enabled.
         @ingroup resip_config
      */
      inline bool getFilloutContacts() const
      {
         return mTransactionController.getFilloutContacts();
      }

      /**
         Sets the CongestionManager used by the stack.
         In order to use a congestion-manager, you will need to create one and 
         pass it to the SipStack, like so:

         @code
          MyCongestionManager* mCM = new MyCongestionManager;
          mStack.setCongestionManager(mCM);
         @endcode

         This will cause the SipStack to register all its fifos with the 
         congestion manager, and will also call 
         TransactionUser::setCongestionManager() for every currently registered 
         TransactionUser. This means that, if you are working with 
         such a TransactionUser, you must have registered the TransactionUser 
         with the SipStack before making this call.
         
         If you are using the SipStack directly by implementing your own 
         subclass of TransactionUser, you can override 
         TransactionUser::setCongestionManager() to register additional fifos 
         (the default implementation registers TransactionUser::mFifo), like so
         
         @code
         MyTransactionUser::setCongestionManager(CongestionManager* cm)
         {
          TransactionUser::setCongestionManager(cm);
          mExtraFifo.unregisterFromCongestionManager();
          if(cm)
          {
             cm->registerFifo(&mExtraFifo,
                               CongestionManager::WAIT_TIME, // or SIZE, or TIME_DEPTH
                               200 // or whatever you want your max tolerance to be
                               );
          }
         }
         @endcode
         
         @param manager The CongestionManager to use. Ownership is not taken.
         @ingroup resip_config
      */
      void setCongestionManager ( CongestionManager *manager )
      {
         mTUFifo.unregisterFromCongestionManager();
         mTransactionController.setCongestionManager(manager);
         mTuSelector.setCongestionManager(manager);
         mCongestionManager=manager;
         if(manager)
         {
            manager->registerFifo(&mTUFifo,
                           CongestionManager::WAIT_TIME,
                           200);
         }
      }

      /**
         @brief Accessor for the Compression object the stack is using.
         @return The Compression object being used.
         @note If no Compression object was set on construction, this will be 
            initialized to a null-implementation, so this function will be safe 
            to call.
         @ingroup resip_config
      */
      Compression &getCompression() { return *mCompression; }
      
      inline const resip::Data& getFQDN() const 
      { 
         return mTransactionController.transportSelector().getFQDN();
      }

      inline void setFQDN(const resip::Data& pFQDN) 
      {
         mTransactionController.transportSelector().setFQDN(pFQDN);
      }

   private:
      /** @brief Notify an async process handler - if one has been registered **/
      void checkAsyncProcessHandler();

      /** @brief if this object exists, it manages advanced security featues **/
      Security* mSecurity;

      DnsStub* mDnsStub;
      DnsThread* mDnsThread;

      /** @brief If this object exists, it manages compression parameters **/
      Compression* mCompression;

      /** @brief if this object exists, it gets notified when ApplicationMessage's get posted **/
      AsyncProcessHandler* mAsyncProcessHandler;

      /** @brief Disallow copy, by not implementing **/
      SipStack(const SipStack& copy);

      /** @brief Disallow assignment, by not implementing **/
      SipStack& operator=(const SipStack& rhs);         
      
      /** @brief fifo used to communicate between the TU (TransactionUser) and stack 
          @note since the introduction of multiple TU's this Fifo should no 
          longer be used by most applications - each TU now owns it's own Fifo. */
      Fifo<Message> mTUFifo;
      std::deque<Message*> mTUBuffer;

      /** @brief Protection for AppTimerQueue **/
      mutable Mutex mAppTimerMutex;
      
      /** @details timers associated with the application. When a timer fires, it is
          placed in the mTUFifo (if there is not associated TU), or it is placed
          on the fifo of the appropriate TU */
      TuSelectorTimerQueue  mAppTimers;
      UInt64 mNextTimer;
      
      /** @brief Used to Track stack statistics **/
      StatisticsManager mStatsManager;

      /** @brief Used to react to congestion **/
      CongestionManager *mCongestionManager;
      
      /** @brief All aspects of the Transaction State Machine / DNS resolver **/
      TransactionController mTransactionController;

      TransactionControllerThread* mTransactionControllerThread;
      TransportSelectorThread* mTransportSelectorThread;
      bool mRunning;
      /** @brief store all domains that this stack is responsible for.
          @note Controlled by addAlias and addTransport interface
          and checks can be made with isMyDomain() */
      std::set<Data> mDomains;
      resip::Data mFQDN;

      Uri mUri;
      bool mShuttingDown;
      mutable Mutex mShutdownMutex;
      volatile bool mStatisticsManagerEnabled;

      /** @brief Responsible for routing messages to the correct TU based on installed rules **/
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

   private:
      // "Interceptor" function for folks who are migrating from a version that 
      // did not have a transportFlags param, with extra arg to help explain 
      // what is going on.
      typedef enum
      {
         THANK_GOODNESS
      } ImplicitTypeconversionForArgDisabled;

      Transport* addTransport( TransportType protocol,
                         int port, 
                         IpVersion version,
                         StunSetting stun,
                         const Data& ipInterface,
                         const Data& sipDomainname,
                         const Data& privateKeyPassPhrase,
                         SecurityTypes::SSLType sslType,
                         bool hasOwnThread,
                         ImplicitTypeconversionForArgDisabled=THANK_GOODNESS);
};

std::ostream& operator<<(std::ostream& strm, const SipStack& stack);

inline void 
SipStack::sendOverExistingConnection(const SipMessage& msg, const Tuple& tuple,
                                     TransactionUser* tu)
{
   assert(tuple.mFlowKey);   
   Tuple tup(tuple);
   tup.onlyUseExistingConnection = true;   
   sendTo(msg, tuple, tu);
}
 
}

#endif

/* Copyright 2007 Estacado Systems */

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
