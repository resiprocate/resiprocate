#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/Logger.hxx"

#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/DnsResolver.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/StatelessHandler.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/TransportMessage.hxx"



using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatelessHandler::StatelessHandler(TransactionController& c) : mController(c)
{
}

void 
StatelessHandler::process()
{
   Message* msg = mController.mStateMacFifo.getNext();
   assert(msg);

   SipMessage* sip = dynamic_cast<SipMessage*>(msg);
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
            mController.mTUFifo.add(sip);
         }
         else if (sip->isRequest())
         {
            if (sip->getDestination().transport)
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               mController.mTransportSelector.transmit(sip, sip->getDestination()); // results not used
            }
            else
            {
               DebugLog (<< "Processing request from TU : " << msg->brief());
               StatelessMessage* stateless = new StatelessMessage(mController.mTransportSelector, sip);
               mController.mTransportSelector.dnsResolve(sip, stateless);
            }
         }
         else // no dns for sip responses
         {
            assert(sip->isResponse());
            DebugLog (<< "Processing response from TU: " << msg->brief());
            const Via& via = sip->header(h_Vias).front();
            Tuple destination;
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
            destination.transportType = Tuple::toTransport(via.transport());
            
            mController.mTransportSelector.transmit(sip, destination); // results not used
         }
      }
   }
   else if (transport)
   {
      DebugLog (<< "Processing Transport result: " << msg->brief());
      
      if (transport->isFailed())
      {
         InfoLog (<< "Not yet supported");
      }
   }
   else
   {
      DebugLog (<< "Dropping: " << msg->brief());
   }
}


StatelessMessage::StatelessMessage(TransportSelector& selector, SipMessage* msg) : mSelector(selector), mMsg(msg)
{
}

void 
StatelessMessage::handle(DnsResult* result)
{
   if (result->available() == DnsResult::Available)
   {
      Tuple next = result->next();
      mSelector.transmit(mMsg, next);
   }

   delete this;
   result->destroy();
}
