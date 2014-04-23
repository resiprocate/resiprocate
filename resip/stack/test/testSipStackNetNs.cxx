#include <memory>

#include <cassert>
#include <sys/types.h>
#include <ifaddrs.h>

#include "rutil/Log.hxx"
#include "rutil/NetNs.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/Helper.hxx"

using namespace resip;
using namespace std;

#ifdef USE_NETNS

typedef vector<Data> DataVector;

class WakeUpMessage : public Message
{
public:
    WakeUpMessage(){};
    virtual ~WakeUpMessage(){};

    virtual resip::Message* clone() const {return(new WakeUpMessage());};
    virtual std::ostream& encode(std::ostream& s) const {return(s);};
    virtual std::ostream& encodeBrief(std::ostream& s) const {return(s);};
};

class TestTransactionConsumer : public TransactionUser
{
private:
    static Data sName;

public:
    TestTransactionConsumer()
    {
    };

    ~TestTransactionConsumer()
    {
    };

    Data& name() const
    {
        return(sName);
    };

    Message* getMessage()
    {
        return(mFifo.getNext(3000));
    };

    void sendWakeUp()
    {
        WakeUpMessage* wakeUp = new WakeUpMessage();
        mFifo.add(wakeUp, TimeLimitFifo<Message>::InternalElement);
    };
};
Data TestTransactionConsumer::sName;

int getInterfaces(HashMap<Data, DataVector> & interfaces)
{
   int interfaceCount = 0;
   struct ifaddrs* interfaceList = NULL;

   getifaddrs(&interfaceList);
   struct ifaddrs* interface = interfaceList;
   while(interface)
   {
      if(interface->ifa_flags)
      {
         Data address = 
            inet_ntoa(((struct sockaddr_in*)interface->ifa_addr)->sin_addr);

         if(address.find(".0.0.0") == Data::npos)
         {
            interfaces[interface->ifa_name].push_back(address);
            //resipCerr << interface->ifa_name << "=" << address
            //    << "(" << interface->ifa_flags << ")" << std::endl;
            interfaceCount++;
         }
      }

      interface = interface->ifa_next;
   }

   return(interfaceCount);
}

bool testSendReceiveOnAllInterfaces()
{
   int sendPort = 15060;
   int receivePort = 25060;

   // Create a stack to send message from
   FdPollGrp* sendFdPollGroup = FdPollGrp::create();
   EventThreadInterruptor* sendResipStackInteruptor = new EventThreadInterruptor(*sendFdPollGroup);

   SipStackOptions sendSipStackOptions;
   sendSipStackOptions.mAsyncProcessHandler = sendResipStackInteruptor;
   sendSipStackOptions.mStateless = false;
   sendSipStackOptions.mPollGrp = sendFdPollGroup;

   SipStack* sendSipStack = new SipStack(sendSipStackOptions);
   
   EventStackThread* sendMainSipStackThread =  
       new EventStackThread(*sendSipStack,
                            *sendResipStackInteruptor,
                            *sendFdPollGroup);

   // Register a consumer for SIP transactions
   TestTransactionConsumer* sendSipTransactionConsumer = new TestTransactionConsumer();
   sendSipStack->registerTransactionUser(*sendSipTransactionConsumer);

   // Start the sender stack
   sendSipStack->run();
   sendMainSipStackThread->run();


   // Create a stack to receive message from
   FdPollGrp* receiveFdPollGroup = FdPollGrp::create();
   EventThreadInterruptor* receiveResipStackInteruptor = new EventThreadInterruptor(*receiveFdPollGroup);

   SipStackOptions receiveSipStackOptions;
   receiveSipStackOptions.mAsyncProcessHandler = receiveResipStackInteruptor;
   receiveSipStackOptions.mStateless = false;
   receiveSipStackOptions.mPollGrp = receiveFdPollGroup;

   SipStack* receiveSipStack = new SipStack(receiveSipStackOptions);
   
   EventStackThread* receiveMainSipStackThread =  
       new EventStackThread(*receiveSipStack,
                            *receiveResipStackInteruptor,
                            *receiveFdPollGroup);

   // Register a consumer for SIP transactions
   TestTransactionConsumer* receiveSipTransactionConsumer = new TestTransactionConsumer();
   receiveSipStack->registerTransactionUser(*receiveSipTransactionConsumer);

   // Start the receiver stack
   receiveSipStack->run();
   receiveMainSipStackThread->run();


   vector<Data> publicNetNs;
   int netnsCount = NetNs::getPublicNetNs(publicNetNs);
   //resipCerr << "netns count: " << netnsCount <<std::endl;
   //resipCerr << "vector size: " << publicNetNs.size() <<std::endl;
   int sentMessageCount = 0;
   int receivedMessageCount = 0;

   assert(netnsCount);
   assert(netnsCount == (int) publicNetNs.size());
   for(unsigned int netNsIndex = 0; netNsIndex < publicNetNs.size(); netNsIndex++)
   {
      HashMap<Data, DataVector> currentInterfaces;

      resipCerr << "Found netns: \"" << publicNetNs[netNsIndex] << "\"" << std::endl;

      // Switch the current netns
      NetNs::setNs(publicNetNs[netNsIndex]);

      // Get the interfaces in the current netns
      getInterfaces(currentInterfaces);

      // Reset back to the default namespace
      // This is to be sure that addTransport uses the specified netns as opposed to
      // just getting lucky and using the current netns
      NetNs::setNs("");

      for(HashMap<Data, DataVector>::iterator interfaceItr = currentInterfaces.begin();
              interfaceItr != currentInterfaces.end(); ++ interfaceItr)
      {
         for(size_t addressIndex = 0; addressIndex < interfaceItr->second.size(); addressIndex++)
         {
            resipCerr << interfaceItr->first << "=" << interfaceItr->second[addressIndex] << std::endl;
            Data ipAddress = interfaceItr->second[addressIndex];
            bool isIpV6 = resip::DnsUtil::isIpV6Address(ipAddress);

            try
            {
                Transport* transport =
                    sendSipStack->addTransport(TCP,
                                               0, // any port available
                                               isIpV6 ? resip::V6 : resip::V4,
                                               StunDisabled,
                                               ipAddress,
                                               Data::Empty, // TLD domain
                                               Data::Empty, // private key pass phrase
                                               SecurityTypes::TLSv1,
                                               0, // transport flags
                                               Data::Empty, // cert filename
                                               Data::Empty, // private key filename
                                               SecurityTypes::None,
                                               false, // use email as SIP
                                               SharedPtr<resip::WsConnectionValidator>(),
                                               SharedPtr<resip::WsCookieContextFactory>(),
                                               publicNetNs[netNsIndex]);
                assert(transport);
                sendPort = transport->getTuple().getPort();
                cout << "Sending from port: " << sendPort << endl;

            }
            catch(resip::BaseException& e)
            {
                resipCerr << "Failed to bind TCP " << ipAddress << ":" << sendPort << "\n"
                        << e << std::endl;
                assert(0);
            }


            try
            {
                Transport* transport =
                    receiveSipStack->addTransport(TCP,
                                               0, // any port available
                                               isIpV6 ? resip::V6 : resip::V4,
                                               StunDisabled,
                                               ipAddress,
                                               Data::Empty, // TLD domain
                                               Data::Empty, // private key pass phrase
                                               SecurityTypes::TLSv1,
                                               0, // transport flags
                                               Data::Empty, // cert filename
                                               Data::Empty, // private key filename
                                               SecurityTypes::None,
                                               false, // use email as SIP
                                               SharedPtr<resip::WsConnectionValidator>(),
                                               SharedPtr<resip::WsCookieContextFactory>(),
                                               publicNetNs[netNsIndex]);
                assert(transport);
                receivePort = transport->getTuple().getPort();
                cout << "Receiving on port: " << receivePort << endl;

            }
            catch(resip::BaseException& e)
            {
                resipCerr << "Failed to bind TCP " << ipAddress << ":" << receivePort << "\n"
                        << e << std::endl;
                assert(0);
            }

            NameAddr from;
            from.uri().scheme() = "sip";
            from.uri().user() = "sender";
            from.uri().host() = ipAddress;
            from.uri().port() = sendPort;
            NameAddr target;
            target.uri().scheme() = "sip";
            target.uri().user() = "receiver";
            target.uri().host() = ipAddress;
            target.uri().port() = receivePort;
            target.uri().param(p_transport) = "tcp";
            target.uri().netNs() = publicNetNs[netNsIndex];
            MethodTypes method = OPTIONS;
            cout << "From: " << from << " To: " << target << endl;
            auto_ptr<SipMessage> sipRequest = auto_ptr<SipMessage>(Helper::makeRequest(target, from, method));
            assert(sipRequest->header(h_RequestLine).uri().netNs() == publicNetNs[netNsIndex]);

            // Send a message
            sendSipStack->send(sipRequest, sendSipTransactionConsumer);
            sentMessageCount++;

            // Receive request
            Message* message = receiveSipTransactionConsumer->getMessage();
            SipMessage* receivedRequest = static_cast<SipMessage*>(message);
            if(receivedRequest)
            {
               receivedMessageCount++;

               // Send back a response to stop resends
               // TODO
               auto_ptr<SipMessage> sipResponse(Helper::makeResponse(*receivedRequest, 200, "Got it"));
               receiveSipStack->send(sipResponse, receiveSipTransactionConsumer);
               
               // Assert we received it on the right interface
               Tuple receivedInterface = receivedRequest->getReceivedTransportTuple();
               cout << "Received request source netns: " << publicNetNs[netNsIndex] << " " << ipAddress << ":" << receivedInterface.getPort() << endl;
               assert(receivedInterface.getPort() == receivePort);
               assert(receivedInterface.getType() == resip::TCP);
               assert(Tuple::inet_ntop(receivedInterface) == ipAddress);
               assert(receivedInterface.getNetNs() == publicNetNs[netNsIndex]);
               Tuple receivedSourceTuple = receivedRequest->getSource();
               // TCP port of source will be actual client port, not server/listener port
               assert(receivedSourceTuple.getType() == resip::TCP);
               assert(Tuple::inet_ntop(receivedSourceTuple) == ipAddress);
               assert(receivedSourceTuple.getNetNs() == publicNetNs[netNsIndex]);
            }
            else
            {
               cout << "No message Received from netns: " << publicNetNs[netNsIndex] << " " << ipAddress << endl;
               assert(receivedRequest);
            }

            // Receive response
            message = sendSipTransactionConsumer->getMessage();
            assert(message);
            cout << "Message: " << message << endl;
            SipMessage* receivedResponse = static_cast<SipMessage*>(message);
            if(receivedResponse)
            {
               // Verify response came back on right IP address and netns
               Tuple receivedResponseInterface = receivedResponse->getReceivedTransportTuple();
               cout << "Received response source netns: " << publicNetNs[netNsIndex] << " " << ipAddress << ":" << receivedResponseInterface.getPort() << endl;
               assert(receivedResponseInterface.getType() == resip::TCP);
               assert(Tuple::inet_ntop(receivedResponseInterface) == ipAddress);
               assert(receivedResponseInterface.getPort() == sendPort);
               assert(receivedResponseInterface.getNetNs() == publicNetNs[netNsIndex]);
               Tuple receivedResponseSourceTuple = receivedResponse->getSource();
               // TCP port of source will be actual client port, not server/listener port
               assert(receivedResponseSourceTuple.getType() == resip::TCP);
               assert(Tuple::inet_ntop(receivedResponseSourceTuple) == ipAddress);
               assert(receivedResponseSourceTuple.getNetNs() == publicNetNs[netNsIndex]);
            }
            else if(message)
            {
               cout << "Non-SIP message: " << *message << endl;
            }
            else
            {
               cout << "No response Received from netns: " << publicNetNs[netNsIndex] << " " << ipAddress << endl;
               assert(receivedResponse);
            }
         }
      }

   }
   cout << "Received " << receivedMessageCount << " of " << sentMessageCount << " messages" << endl;

   return(false);
}

int main(int argc, const char* argv[])
{
    //Log::setLevel(Log::Stack);
    assert(!testSendReceiveOnAllInterfaces());

    resipCerr << "ALL OK" << std::endl;
    return(0);
}

#else

int main(int argc, const char* argv[])
{
    resipCerr << "USE_NETNS not set." << std::endl;
    return(0);
}

#endif

/* ====================================================================
 *
 * Copyright (c) 2014 Daniel Petrie, SIPez LLC  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 */
 // vi: set shiftwidth=3 expandtab:
