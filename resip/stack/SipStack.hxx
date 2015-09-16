#if !defined(RESIP_SIPSTACK_HXX)
#define RESIP_SIPSTACK_HXX

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <set>
#include <iosfwd>

#include "rutil/CongestionManager.hxx"
#include "rutil/FdSetIOObserver.hxx"
#include "rutil/TimeLimitFifo.hxx"
#include "rutil/Mutex.hxx"
#include "rutil/TransportType.hxx"
#include "rutil/BaseException.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/TransportSelector.hxx"
#include "resip/stack/SecurityTypes.hxx"
#include "resip/stack/StatisticsManager.hxx"
#include "resip/stack/TuSelector.hxx"
#include "resip/stack/WsConnectionValidator.hxx"
#include "resip/stack/WsCookieContextFactory.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/SharedPtr.hxx"

/**
    Let external applications know that this version of the stack
    supports the (e)poll interfaces.
**/
#define RESIP_SIPSTACK_HAVE_FDPOLL 1

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
class FdPollGrp;

/**
   This class holds constructor-time initialization arguments for SipStack.

   Most values are pointers, and default to zero unless otherwise indicated.

   It has public members:
      mSecurity
         Security Object required by the stack for TLS, DTLS, SMIME and
         Identity-draft compliance.  If empty the stack will not support
         these advanced security features.  The compile flag USE_SSL is
         also required.  The security object will be owned by the SipStack
         and deleted in the SipStack destructor.

       mAsyncProcessHandler
          AsyncProcessHandler that will be invoked when Messages
          are posted to the stack.  Posted messages are added
          to thread-safe queue, and then processed later within
          event loop. For example:  SelectInterruptor.

       mStateless
          This parameter does not appear to be used. Default false.

       mSocketFunc
          A pointer to a function that will be called after a socket in
          the DNS or SIP transport layers of the stack has been created.
          This callback can be used to control low level socket options,
          such as Quality-of-Service/DSCP.  Note:  For SIP TCP sockets
          there is one call for the listen socket, and one (or two)
          calls for each connection created afterwards.  For each inbound
          TCP connection the first callback is called immediately before
          the socket is connected, and if configured it is called again
          after the connect call has completed and before the first data
          is sent out.  On some OS's you cannot set QOS until the socket
          is successfully connected.  To enable this behavior call:
                  Connection::setEnablePostConnectSocketFuncCall();

       mCompression
          Compression configuration object required for
          SigComp. If set to 0, then SigComp compression
          will be disabled. The SipStack takes ownership of this object,
          and will be deleted in the destructor.

       mPollGrp
          Polling group to support file-io callbacks.
          See EventStackThread. The SipStack does NOT take ownership;
          the application (or a helper such as EventStackSimpleMgr) must
          release this object after the SipStack is destructed.
**/
class SipStackOptions
{
   public:
      SipStackOptions()
         : mSecurity(0), mExtraNameserverList(0),
           mAsyncProcessHandler(0), mStateless(false),
           mSocketFunc(0), mCompression(0), mPollGrp(0)
      {
      }

      Security* mSecurity;
      const DnsStub::NameserverList* mExtraNameserverList;
      AsyncProcessHandler* mAsyncProcessHandler;
      bool mStateless;
      AfterSocketCreationFuncPtr mSocketFunc;
      Compression *mCompression;
      FdPollGrp* mPollGrp;
};



/**
   @brief This is the primary point of interaction between the app-layer and the
      stack.

   @details For a SipStack to be usable, transports must be added by calling
   the addTransport() method.

   The useful work done by SipStack occurs when
   SipStack::process(unsigned int) is called.  A separate thread (such 
   as EventStackThread) can be dedicated to this task, or it can be called
   from within a loop in the main thread of the executable.

   Graceful shutdown is accomplished by advising the SipStack to begin
   shutdown procedures via the shutdown() method.  The SipStack should
   continue to be serviced through the process(unsigned int) method until the
   Transaction User is informed by receiving a ShutdownMessage that
   the requested shutdown is complete.

   @note Previously, buildFdSet(FdSet&), FdSet::select(), and process(FdSet&) 
      were the canonical way of giving the SipStack cycles. Because of 
      shortcomings of the fd_set approach, these have been deprecated.

   @ingroup resip_crit
   @ingroup resip_config
*/
class SipStack : public FdSetIOObserver
{
   public:
      /**
         Constructor. First instantiate SipStackOptions, then set
         any values special to your application, then call this constructor.
         The {options} instance is not referenced after construction (i.e.,
         the SipStack doesn't keep a reference to it). However, it does
         copy each individual value, and takes ownership of several
         of the objects.
      **/
      SipStack(const SipStackOptions& options);

      /**
          Constructor. This constructor is obsolete. The SipStackOptions-based
          constructor should be used instead.

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

          @param pollGrp     Polling group to support file-io callbacks; if one 
                              is not passed, one will be created. Ownership is 
                              not taken.
      */
      SipStack(Security* security=0,
               const DnsStub::NameserverList& additional = DnsStub::EmptyNameserverList,
               AsyncProcessHandler* handler = 0,
               bool stateless=false,
               AfterSocketCreationFuncPtr socketFunc = 0,
               Compression *compression = 0,
               FdPollGrp* pollGrp = 0);

      virtual ~SipStack();

      /**
         @brief Causes this SipStack object to create and run threads for carrying
         out processing. 

         This includes a thread for DNS lookups, a thread for
         transaction processing, and a thread for transport processing 
         (individual Transport objects may be registered as having their own 
         thread; these are not serviced by the thread spawned by this call). 
         This function is intended to be used in addition to the normal method/s
         of giving the SipStack cycles (eg; EventStackThread, StackThread, etc);
         these are still necessary to give the SipStack cycles for handling 
         application timers, statistics logging, and shutdown logic.
      */
      void run();

      /** 
         @brief perform orderly shutdown
         @details Inform the transaction state machine processor that it should not
         create any new transactions and to perform an orderly shutdown. When
         the transactions are all terminated, return a ShutdownMessage to the TU.
         @note If the SipStack is running in multithreaded mode, this function 
            DOES NOT shut down the threads. This is what 
            shutdownAndJoinThreads() is for. (Shutting down threads in this call
            would prevent orderly shutdown from working; outstanding 
            transactions would be dropped on the floor)
      */
      void shutdown();

      /**
         @brief Shutdown and join any threads that the SipStack is running.
      */
      void shutdownAndJoinThreads();

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
         Used by the application to provide a handler that will get called for all
         inbound and outbound SIP messages on transports that are added after calling this.

         @note                        If you want a custom handler per transport then
                                      you can call setSipMessageLoggingHandler on the
                                      Transport pointer returned from addTransport

         @param handler               SharedPtr to a handler to call for inbound and
                                      outbound SIP messages for all transports added
                                      after calling this.
      */
      void setTransportSipMessageLoggingHandler(SharedPtr<Transport::SipMessageLoggingHandler> handler) { mTransportSipMessageLoggingHandler = handler; }

      /**
         Used by the application to add in a new built-in transport.  The transport is
         created and then added to the Transport Selector.

         @throws Transport::Exception If the transport couldn't be added, usually
                                      because the port was already bound.

         @param protocol              TCP, UDP, TLS, DTLS, etc.

         @param port                  Specifies which port to bind to.

         @param version               Protocol Version:  V4 or V6

         @param stun                  Specifies whether STUN is enabled.

         @param ipInterface           Specifies which ethernet interface to bind to. If set to
                                      Data::Empty, bind to all interfaces. Binding to all 
                                      interfaces can impose a performance penalty, however, so it is 
                                      recommended that you bind to specific interfaces when using in 
                                      high-throughput deployments.  Note:  Interfaces
                                      must be identified via IP address.

         @param sipDomainname         Only allow messages to
                                      be sent as the specified domain.  For default case,
                                      you can pass in domainname = DnsUtil::getLocalDomainName().

         @param privateKeyPassPhrase  Private key pass phrase used to decrypt private key
                                      certificates.  Note:  For now this parameter is not used
                                      we are loading PKCS7 keys, so a pass phrase is not required.

         @param sslType               Version of the TLS specification to use:  SSLv23 or TLSv1

         @param cvm                   SSL verify mode for the peer (whether we accept and/or
                                      insist on a client certificate from the peer)

         @param useEmailAsSIP         If true, we will accept the email address in a client's
                                      subjectAltName as if it were a SIP URI.  This is convenient
                                      because many commercial CAs offer email certificates but not
                                      sip: certificates.  For reasons of standards compliance, it
                                      is disabled by default.

         @param netNs                 Set the network namespace (netns) in which the Transport is
                                      to bind the the given address and port.

      */
      Transport* addTransport(TransportType protocol,
                              int port,
                              IpVersion version=V4,
                              StunSetting stun=StunDisabled,
                              const Data& ipInterface = Data::Empty,
                              const Data& sipDomainname = Data::Empty, // only used for TLS based stuff
                              const Data& privateKeyPassPhrase = Data::Empty,
                              SecurityTypes::SSLType sslType = SecurityTypes::TLSv1,
                              unsigned transportFlags = 0,
                              const Data& certificateFilename = "", const Data& privateKeyFilename = "",
                              SecurityTypes::TlsClientVerificationMode cvm = SecurityTypes::None,
                              bool useEmailAsSIP = false,
                              SharedPtr<WsConnectionValidator> = SharedPtr<WsConnectionValidator>(),
                              SharedPtr<WsCookieContextFactory> = SharedPtr<WsCookieContextFactory>(),
                              const Data& netns = Data::Empty
                             );

      /**
          Used to plug-in custom transports.  Adds the transport to the Transport
          Selector.

          @param transport Pointer to an externally created transport.  SipStack
                           assumes ownership.
      */
      void addTransport(std::auto_ptr<Transport> transport);

      /**
          Used to remove a previously added transport.

          @param transportKey The key for the transpor to remove.  Use Transport::getKey 
                 to get the key of a transport after it is added to the stack.
      */      
      void removeTransport(unsigned int transportKey);

      /**
          Returns the fifo that subclasses of Transport should use for the rxFifo
          cons. param.

          @returns A fifo of TransactionMessage's
      */
      Fifo<TransactionMessage>& stateMacFifo();

      /**
          @brief add an alias for this sip element
          
          @details Used to add an alias for this sip element. e.g. foobar.com and boo.com
          are both handled by this stack.  Alias is added 
          to internal list of Domains and can be checked with isMyDomain.

          @param domain   Domain name that this stack is responsible for.

          @param port     Port for domain that this stack is responsible for.
          @ingroup resip_config
      */
      void addAlias(const Data& domain, int port);

      /**
          @brief removes an alias from this sip element
          
          @details Used to remove an existing alias from this sip element.
          Only removed if reference count hits 0.

          @param domain   Domain name that this stack is responsible for.

          @param port     Port for domain that this stack is responsible for.
          @ingroup resip_config
      */
      void removeAlias(const Data& domain, int port);

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
      void post(std::auto_ptr<ApplicationMessage> message,
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
      void postMS(const std::auto_ptr<ApplicationMessage> message,
                  unsigned int ms,
                  TransactionUser* tu=0);

      /**
          @brief Makes the message available to the TU later
          
          @note  TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post
      */
      void post(std::auto_ptr<ApplicationMessage> message);

      /**
          @brief Makes the message available to the TU later
          
          @note Makes a copy of the Message.  Caller is responsible for deleting
          the memory and may  do so as soon as it returns.  

          @note  TranasctionUser subclasses can just post to themselves.
          
          @param message ApplicationMessage to post
      */
      void post(const ApplicationMessage& message);

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

      /*
       * Handler is notified when a message is posted to the default
       * application receive queue. (Fetching using receive() above).
       * This handler is called from SipStack's thread, and can
       * be used to "wake-up" the application's thread.
       */
      void setFallbackPostNotify(AsyncProcessHandler *handler);

      /**
          Build the FD set to use in a select to find out when process(FdSet&) 
          must be called again. 

          @param fdset an empty or partially populated fdset, fd's are added
                       to the fdset on return

          @note This must be called prior to calling process.
          @note select() must also be called on the fdset prior to process.

          @deprecated Because of serious shortcomings in fd_set (most notably 
            the inability to store FDs whose value exceeds a relatively small 
            number; ~1000), we are deprecating the FdSet-based process loop. 
            @see EventStackThread for an alternative, or if you wish to drive 
            the SipStack yourself, @see process(unsigned int). On platforms that 
            do not support epoll, fd_set ends up being used anyway since there 
            is no other choice, but this is hidden from the app.
      */
      RESIP_DEPRECATED(virtual void buildFdSet(FdSet& fdset));

      /**
          This allows the executive to give processing time to stack components.

          @note Must call buildFdSet and select before calling this.
          @note The transports are serviced, and then timers are serviced.

          @param fdset a populated and 'select'ed fdset
          @deprecated Because of serious shortcomings in fd_set (most notably 
            the inability to store FDs whose value exceeds a relatively small 
            number; ~1000), we are deprecating the FdSet-based process loop. 
            @see EventStackThread for an alternative, or if you wish to drive 
            the SipStack yourself, @see process(unsigned int). On platforms that 
            do not support epoll, fd_set ends up being used anyway since there 
            is no other choice, but this is hidden from the app.
      */
      RESIP_DEPRECATED(virtual void process(FdSet& fdset));

      /**
         @brief Give processing time to the SipStack components, when operating 
            in single-threaded mode.

         This call will allow all the components in the SipStack a chance to 
         perform processing. This includes:

         - Transaction processing (handling of SIP messages and timers)
         - DNS processing (includes DNS network IO and 3263 logic)
         - Transport processing (network IO for raw SIP traffic)
         - Application timers

         To give the SipStack cycles, it is sufficient to simply call this 
         function repeatedly.

         @param timeoutMs The maximum time, in milliseconds, we will wait for IO 
            in this call. We will not necessarily wait this long if no IO 
            occurs; if timers are scheduled to fire before this duration 
            elapses, our timeout value will be adjusted accordingly. Similarly, 
            if there is work waiting in fifos when this call is made, no timeout 
            will be used. Lower values will cause higher CPU utilization when 
            idle, higher values may result in processing delays of messages from 
            the TU sent during the call to process(unsigned int) (eg; you call 
            process(50), then immediately schedule an app timer to fire in 25ms, 
            or send a SipMessage to the stack, it could be 50ms before either of 
            these are processed). You can work around these caveats by creating 
            a SelectInterruptor, adding its FD to the SipStack's FdPollGrp, and 
            calling SelectInterruptor::interrupt() to cause 
            process(unsigned int) to be interrupted. The best approach will 
            vary, based on your performance needs.

         @note If you wish to add an FD that will interrupt this call if it 
            becomes ready (either because it has IO you are interested in, or 
            merely to interrupt this call to carry out some unrelated task, see 
            getPollGrp())

         @return Whether any work was done as a result of this call.
      */
      bool process(unsigned int timeoutMs);

      inline FdPollGrp* getPollGrp() {return mPollGrp;} 

      /** 
         @brief Returns time in milliseconds when process next needs to be 
            called.

         @return The maximum time in ms that whatever is giving the 
         SipStack processing cycles should wait before calling either 
         process(unsigned int) or process(FdSet&). In most circumstances, this 
         is simply when the next timer (either an app timer, or a SIP 
         transaction timer) is scheduled to fire. However, in cases where there 
         is processing work to be done (in the form of messages in a fifo 
         somewhere), this will return 0.
      */
      virtual unsigned int getTimeTillNextProcessMS();

      /** 
         @brief Check all timers

         @note If you are driving this SipStack's IO using its FdPollGrp 
         directly (because you have more than one stack sharing the FdPollGrp, 
         for example), you need to call this periodically (see impl of 
         EventStackThread for an example of this). process(unsigned int) does 
         this for you.
      */
      virtual void processTimers();

      /**
         @brief Sets the interval that determines the time between Statistics messages
         @ingroup resip_config
      */
      void setStatisticsInterval(unsigned long seconds);

      /**
         @brief Resets all of the cumulative statistics counters.
      */
      void zeroOutStatistics();

      /**
         @brief Immediately polls for statistics to be logged and sent to
         external handlers, instead of waiting for next statistics interval.
         Returns false if statistics manager is not enabled.
      */
      bool pollStatistics();

      /** Installs a handler for the stacks internal StatisticsManager.  This handler is called before the
        * default behavior.
        */
      void setExternalStatsHandler(ExternalStatsHandler *handler)
      {
         mStatsManager.setExternalStatsHandler(handler);
      }

      /** @brief get statistics manager **/
      const StatisticsManager* getStatisticsManager() {return(&mStatsManager);}

      /** @brief output current state of the stack - for debug **/
      EncodeStream& dump(EncodeStream& strm) const;

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
          Register a handler with the DNS Interface for notifications of when a Dns
          Resource Record has been blacklisted.

          @param listener Class implementing the onMark() callback 
                                   event sink defined in MarkListener
          @ingroup resip_config
      */
      void registerMarkListener(MarkListener* listener);

      /**
          Removed a registered BlacklistListener handler from the DNS Interface
          for a particualr Resource Record type and handler pointer.

          @param listener Class implementing the onMark() callback 
                                   event sink defined in MarkListener

          @ingroup resip_config
      */
      void unregisterMarkListener(MarkListener* listener);

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
          @brief Specify which domains in the To: Uri will be subject to ENUM

          @details If the Uri hostname/domain is one of these domains,
          the user part will be considered for ENUM search

          @ingroup resip_config
      */
      void setEnumDomains(const std::map<Data,Data>& domains);

      /**
          @brief Clear the DNS Cache
      */
      void clearDnsCache();

      /**
          @brief Log the DNS Cache to WarningLog for Debugging
      */
      void logDnsCache();

      /**
          @brief Get a string representation of the DNS Cache. 
          @param key - a pair representing the request key, can be used
                       by the callback handler to identify the originating
                       request
          @param handler - pointer to a class that implements the handler 
                           method: onDnsCacheRetrieved
      */
      void getDnsCacheDump(std::pair<unsigned long, unsigned long> key, GetDnsCacheDumpHandler* handler);

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
         return mTransactionController->mFixBadDialogIdentifiers;
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
         mTransactionController->mFixBadDialogIdentifiers = pFixBadDialogIdentifiers;
      }

      inline bool getFixBadCSeqNumbers() const
      {
         return mTransactionController->getFixBadCSeqNumbers();
      }

      inline void setFixBadCSeqNumbers(bool pFixBadCSeqNumbers)
      {
         mTransactionController->setFixBadCSeqNumbers(pFixBadCSeqNumbers);
      }

      bool setUdpOnlyOnNumeric(bool value)
      {
         return mTransactionController->transportSelector().setUdpOnlyOnNumeric(value);
      }

      bool getUdpOnlyOnNumeric() const
      {
         return mTransactionController->transportSelector().getUdpOnlyOnNumeric();
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
          if(mCongestionManager)
          {
             mCongestionManager->unregisterFifo(&mExtraFifo);
          }
          mCongestionManager=cm;
          if(mCongestionManager)
          {
             mCongestionManager->registerFifo(&mExtraFifo);
          }
         }
         @endcode
         
         @param manager The CongestionManager to use. Ownership is not taken.
         @ingroup resip_config
      */
      void setCongestionManager ( CongestionManager *manager )
      {
         mTransactionController->setCongestionManager(manager);
         mTuSelector.setCongestionManager(manager);
         if(mCongestionManager)
         {
            mCongestionManager->unregisterFifo(&mTUFifo);
         }
         mCongestionManager=manager;
         if(mCongestionManager)
         {
            mCongestionManager->registerFifo(&mTUFifo);
         }
      }

      /**
         @brief Accessor for the CongestionManager object the stack is using.
         @return The CongestionManager object being used.
         @note If no Compression object was set on construction, this will 
            return null.
         @ingroup resip_config
      */
      CongestionManager* getCongestionManager() { return mCongestionManager; }

      /**
         @brief Accessor for the Compression object the stack is using.
         @return The Compression object being used.
         @note If no Compression object was set on construction, this will be 
            initialized to a null-implementation, so this function will be safe 
            to call.
         @ingroup resip_config
      */
      Compression &getCompression() { return *mCompression; }

      void terminateFlow(const resip::Tuple& flow);
      void enableFlowTimer(const resip::Tuple& flow);

   private:
      /// Performs bulk of work of constructor.
      // WATCHOUT: can only be called once (just like constructor)
      void init(const SipStackOptions& options);

      /** @brief Notify an async process handler - if one has been registered **/
      void checkAsyncProcessHandler();

      FdPollGrp* mPollGrp;
      bool mPollGrpIsMine;

      /** @brief if this object exists, it manages advanced security featues **/
      Security* mSecurity;

      DnsStub* mDnsStub;
      DnsThread* mDnsThread;

      /** @brief If this object exists, it manages compression parameters **/
      Compression* mCompression;

      /** 
         @brief if this object exists, it gets notified when messages are posted 
            to any of the various fifos in the stack. 
         @note When running in multithreaded mode, this is only invoked when 
            either app timers are scheduled, shutdown has been requested, or a 
            TransactionUser has unregistered. The other fifos in the system are 
            associated with their own AsyncProcessHandlers, since they are being 
            processed by different threads.
      **/
      AsyncProcessHandler* mAsyncProcessHandler;
      bool mInterruptorIsMine;

      /** @brief Disallow copy, by not implementing **/
      SipStack(const SipStack& copy);

      /** @brief Disallow assignment, by not implementing **/
      SipStack& operator=(const SipStack& rhs);         
      
      /** @brief fifo used to communicate between the TU (TransactionUser) and stack 
          @note since the introduction of multiple TU's this Fifo should no 
          longer be used by most applications - each TU now owns it's own Fifo. */
      TimeLimitFifo<Message> mTUFifo;
      CongestionManager* mCongestionManager;

      /// Responsible for routing messages to the correct TU based on installed rules
      TuSelector mTuSelector;

      /** @brief Protection for AppTimerQueue **/
      mutable Mutex mAppTimerMutex;
      
      /** @details timers associated with the application. When a timer fires, it is
          placed in the mTUFifo (if there is not associated TU), or it is placed
          on the fifo of the appropriate TU */
      TuSelectorTimerQueue  mAppTimers;

      /** @brief Used to Track stack statistics **/
      StatisticsManager mStatsManager;

      /** @brief All aspects of the Transaction State Machine / DNS resolver **/
      TransactionController* mTransactionController;
      std::auto_ptr<ProducerFifoBuffer<TransactionMessage> > mStateMacFifoBuffer;

      TransactionControllerThread* mTransactionControllerThread;
      TransportSelectorThread* mTransportSelectorThread;
      bool mInternalThreadsRunning;
      bool mProcessingHasStarted; 

      /** @brief store all domains that this stack is responsible for.
          @note Controlled by addAlias and addTransport interface
          and checks can be made with isMyDomain() */
      typedef std::map<Data, unsigned int> DomainsMap;
      DomainsMap mDomains;  // Second item (unsigned int) is for reference counting
      Uri mUri;
      mutable Mutex mDomainsMutex;  // Protects both mDomains and mUri, since they are related

      /** store all ports that this stack is lisenting on.  Controlled by addTransport
          and checks can be made with isMyPort() */
      std::map<int, unsigned int> mPorts;  // Second item (unsigned int) is for reference counting
      mutable Mutex mPortsMutex;

      // Used to ensure new Transport additions will always succeed without needing to ask 
      // TransportSelector if add will be valid and introduce locking
      // Note:  We could add a Mutex here and add thread safe accesor methods to transport pointers 
      //        as a convience to API users
      typedef std::map<Tuple, Transport*> NonSecureTransportMap;
      NonSecureTransportMap mNonSecureTransports;
      typedef std::map<TransportSelector::TlsTransportKey, Transport*> SecureTransportMap;
      SecureTransportMap mSecureTransports;

      bool mShuttingDown;
      mutable Mutex mShutdownMutex;
      volatile bool mStatisticsManagerEnabled;

      AfterSocketCreationFuncPtr mSocketFunc;

      unsigned int mNextTransportKey;

      SharedPtr<Transport::SipMessageLoggingHandler> mTransportSipMessageLoggingHandler;

      friend class Executive;
      friend class StatelessHandler;
      friend class StatisticsManager;
      friend class TestDnsResolver;
      friend class TestFSM;
      friend class TransactionState;
      friend class TransactionController;
      friend class TuSelector;
};

EncodeStream& operator<<(EncodeStream& strm, const SipStack& stack);

inline void
SipStack::sendOverExistingConnection(const SipMessage& msg, const Tuple& tuple,
                                     TransactionUser* tu)
{
   resip_assert(tuple.mFlowKey);
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
 * vi: set shiftwidth=3 expandtab:
 */
