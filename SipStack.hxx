#if !defined(RESIP_SIPSTACK_HXX)
#define RESIP_SIPSTACK_HXX

#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include <set>

#include "resiprocate/os/Fifo.hxx"
#include "resiprocate/Executive.hxx"
#include "resiprocate/Transport.hxx"
#include "resiprocate/TransactionController.hxx"

namespace resip
{

class Data;
class Message;
class SipMessage;
class Executive;
class Security;
class Uri;
	
class SipStack
{
   public:
      // If multithreaded=true, run transports, timerqueue, transaction and dns
      // in separate threads
      // Set stateless=true, if you want to use the stack for a stateless proxy
      // (no transactions)
      SipStack(bool multiThreaded=false, Security* security=0, bool stateless=false);
      virtual ~SipStack();

      // inform the transaction state machine processor that it should not
      // create any new transactions and to perform an orderly shutdown. When
      // the transactions are all terminated, return a ShutdownMessage to the TU
      void shutdown();
      
      // Used by the application to add in a new transport
      // hostname parameter is used to specify the host portion of the uri that
      // describes this sip element (proxy or ua)
      // nic is used to specify an ethernet interface by name. e.g. eth0
      void addTransport( TransportType protocol,
                         int port, 
                         const Data& hostName = Data::Empty, 
                         const Data& nic = Data::Empty);

      // If port = 0, use DNS to lookup the port number for the specified
      // domain. Only allow messages to be sent as the specified domain. 
      // For default case, you can pass in domainname =
      // DnsUtil::getLocalDomainName()
      // Note the SipDomainName is often your host name not your server domainname
      void addTlsTransport(  int port = 5061,
                             const Data& keyDir = Data::Empty, 
                             const Data& privateKeyPassPhrase = Data::Empty,
                             const Data& sipDomainname = Data::Empty, 
                             const Data& hostName = Data::Empty, 
                             const Data& nic = Data::Empty);

      // used to add an alias for this sip element. e.g. foobar.com and boo.com
      // are both handled by this proxy. 
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

      // interface for the TU to send a message. makes a copy of the
      // SipMessage. Caller is responsible for deleting the memory and may do
      // so as soon as it returns. Loose Routing processing as per RFC3261 must
      // be done before calling send by the TU. See Helper::processStrictRoute
      void send(const SipMessage& msg);

      // this is only if you want to send to a destination not in the route. You
      // probably don't want to use it. 
      void sendTo(const SipMessage& msg, const Uri& uri);
      void sendTo(const SipMessage& msg, const Tuple& tuple);

      // caller now owns the memory. returns 0 if nothing there
      SipMessage* receive(); 

      // May return TransactionTerminated* or SipMessage*
      Message* receiveAny(); 
      
      // build the FD set to use in a select to find out when process bust be
      // called again. This must be called prior to calling process. 
      virtual void buildFdSet(FdSet& fdset);
	
      // should call buildFdSet before calling process. This allows the
      // executive to give processing time to stack components. 
      virtual void process(FdSet& fdset);

      /// returns time in milliseconds when process next needs to be called 
      virtual int getTimeTillNextProcessMS(); 

      // Inform the TU that whenever a transaction has been terminated. 
      void registerForTransactionTermination();

      // Allow the application to specify strict routing behavior, by default
      // loose routing policy is used. 
      void enableStrictRouting(bool strict=true) { mStrictRouting = strict; }
      bool isStrictRouting() const { return mStrictRouting; }
      
      /// if this object exists, it manages advanced security featues
      Security* security;

   private:
      SipStack(const SipStack& copy);
      SipStack& operator=(const SipStack& rhs);
      
      // fifo used to communicate between the TU (Transaction User) and stack 
      Fifo<Message> mTUFifo;

      // Controls the processing of the various stack elements
      Executive mExecutive;

      // All aspects of the Transaction State Machine / DNS resolver
      TransactionController mTransactionController;

      // store all domains that this stack is responsible for. Controlled by
      // addAlias and addTransport interfaces and checks can be made with isMyDomain()
      std::set<Data> mDomains;

      bool mStrictRouting;
      bool mShuttingDown;
      
      friend class TransactionState;
      friend class StatelessHandler;
      friend class Executive;
      friend class TestDnsResolver;
      friend class TestFSM;
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
