#include <sipstack/TransactionState.hxx>
#include <sipstack/SipStack.hxx>
#include <sipstack/SipMessage.hxx>
#include <sipstack/TimerMessage.hxx>

using namespace Vocal2;

void
Vocal2::TransactionState::process(SipStack& stack)
{
   Message* message = stack.mStateMacFifo.getNext();
   
   SipMessage* sip = dynamic_cast<SipMessage*>(message);
   TimerMessage* timer=0;
   
   if (sip == 0)
   {
      timer = dynamic_cast<TimerMessage*>(message);
   }
   
   if (sip)
   {
      if (stack.mTransactionMap.find(message->getTransactionId()))
      {
      }
   }
   
}


void
TransactionState::process( Message* msg )
{
   switch (mMachine)
   {
      case ClientNonInvite: processClientNonInvite( msg ); break;
      case ClientInvite:    processClientInvite( msg ); break;
      case ServerNonInvite: processServerNonInvite( msg ); break;
      case ServerInvite:    processServerInvite( msg ); break;
      case Stale: processStale( msg ); break;
      default:
         assert(0);
   }
}


void
TransactionState::processClientNonInvite(  Message* msg )
{
}


void
TransactionState::processClientInvite(  Message* msg )
{
}


void
TransactionState::processServerNonInvite(  Message* msg )
{
}


void
TransactionState::processServerInvite(  Message* msg )
{
}


void
TransactionState::processStale(  Message* msg )
{
}

