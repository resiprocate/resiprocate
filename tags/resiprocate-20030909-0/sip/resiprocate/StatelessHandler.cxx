#include "resiprocate/os/Logger.hxx"

#include "resiprocate/SipStack.hxx"
#include "resiprocate/StatelessHandler.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/TransportMessage.hxx"
#include "resiprocate/ReliabilityMessage.hxx"
#include "resiprocate/DnsResolver.hxx"


using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatelessHandler::StatelessHandler(SipStack& stack) :
   mStack(stack),
   mTid(1)
{
}

void 
StatelessHandler::process()
{
   Message* msg = mStack.mStateMacFifo.getNext();
   assert(msg);

   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
   DnsResolver::DnsMessage* dns=dynamic_cast<DnsResolver::DnsMessage*>(msg);
   TransportMessage* transport = dynamic_cast<TransportMessage*>(msg);

   if (sip)
   {
      if (sip->header(h_Vias).empty())
      {
         InfoLog(<< "TransactionState::process dropping message with no Via: " << sip->brief());
         delete sip;
         return;
      }
      else
      {
         if (sip->isExternal())
         {
            DebugLog (<< "Processing sip from wire: " << msg->brief());
            Via& via = sip->header(h_Vias).front();
            // this is here so that we will reuse the tcp connection
            via.param(p_rport).port() = sip->getSource().port;
            mStack.mTUFifo.add(sip);
         }
         else if (sip->isRequest())
         {
            if (sip->getDestination().transport)
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               mStack.mTransportSelector.send(sip, sip->getDestination(), Data::Empty); // results not used
            }
            else
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               mTid++;
               mMap[Data(mTid)] = sip;
               mStack.mTransportSelector.dnsResolve(sip, Data(mTid));
            }
         }
         else // no dns for sip responses
         {
            assert(sip->isResponse());
            DebugLog (<< "Processing response from TU: " << msg->brief());
            const Via& via = sip->header(h_Vias).front();
            Transport::Tuple destination;
#ifdef WIN32
			assert(0); // !CJ! TODO 
#else
			inet_pton(AF_INET, via.param(p_received).c_str(), &destination.ipv4);
#endif
			if (via.exists(p_rport) && via.param(p_rport).hasValue())
            {
               destination.port = via.param(p_rport).port();
            }
            else
            {
               destination.port = via.sentPort();
            }
            destination.transportType = Transport::toTransport(via.transport());
            
            mStack.mTransportSelector.send(sip, destination, Data::Empty); // results not used
         }
      }
   }
   // !jf! for now, wait until all results are back before sending anything
   else if (dns)
   {
      DebugLog (<< "Processing DNS result: " << msg->brief());
      //assert(dns->isFinal);
      Transport::Tuple tuple = *dns->mTuples.begin();
      MapIterator i = mMap.find(dns->getTransactionId());
      assert (i != mMap.end());
      mStack.mTransportSelector.send(i->second, tuple, dns->getTransactionId());
   }
   else if (transport)
   {
      DebugLog (<< "Processing Transport result: " << msg->brief());
      
      if (transport->isFailed())
      {
         // !jf! should move on to next dns result. For now, give up :(
         MapIterator i = mMap.find(transport->getTransactionId());
         if (i != mMap.end())
         {
            delete i->second;
            mMap.erase(i);
         }
      }
      else
      {
         MapIterator i = mMap.find(transport->getTransactionId());
         if (i != mMap.end())
         {
            delete i->second;
            mMap.erase(i);
         }
      }
   }
   else
   {
      DebugLog (<< "Dropping: " << msg->brief());
   }
}

