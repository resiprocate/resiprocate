#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/Helper.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/StunTimerMessage.hxx"
#include "resip/stack/StunMessage.hxx"
#include "resip/stack/TransactionController.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/StunTransactionState.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/TransportFailure.hxx"
#include "resip/stack/TransportSelector.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TuSelector.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Random.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/stack/KeepAliveMessage.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StunTransactionState::StunTransactionState(TransactionController& controller, 
                                   State s, const Data& id, TransactionUser* tu) :
   TransactionState(controller, id, tu),
   mState(s),
   mIsReliable(true), // !jf! 
   mMsgToRetransmit(0)
{
   StackLog (<< "Creating new StunTransactionState: " << *this);
}

StunTransactionState::~StunTransactionState()
{
   assert(mState != Bogus);

   //StackLog (<< "Deleting StunTransactionState " << mId << " : " << this);
   erase(mId);
   
   delete mMsgToRetransmit;
   mMsgToRetransmit = 0;

   mState = Bogus;
}

void
StunTransactionState::process(TransactionController& controller, TransactionMessage* message)
{
   StunMessage* stun = dynamic_cast<StunMessage*>(message);

   Data tid;   
   try
   {
      tid = message->getTransactionId();
   }
   catch(SipMessage::Exception&)
   {
      DebugLog( << "StunTransactionState::process dropping message with invalid tid " << stun->brief());
      delete stun;
      return;
   }

   /*
   TransactionState* transState = 0;
   if (message->isClientTransaction()) 
   {
      transState = controller.mClientTransactionMap.find(tid);
   }

   StunTransactionState* state = dynamic_cast<StunTransactionState*>transState;
   if(!state)
   {
      CriticalLog( << "StunTransactionState::process retrieved a non StunTransactionState for tid " << tid);
      delete stun;
      return;
   }*/

   if(stun)
   {
      if(stun->isRequest())
      {
         /*
         if(stun->isFromWire())
         {
            // TODO send stun response here

            delete message;
            return;
         }
         else
         {
            // Create StunTransactionState

            // Send Stun Request
            // Start timers
         }*/
      }
      else
      {
         // find TransactionState and dispatch response
      }
   }

   StunTimerMessage* timer = dynamic_cast<StunTimerMessage*>(message);
   if(timer)
   {
      // TODO handle timer - find transaction state and dispatch timer

      delete message;
      return;
   }

   delete message;
   return;
}

void
StunTransactionState::processTransportFailure(TransactionMessage* msg)
{
   TransportFailure* failure = dynamic_cast<TransportFailure*>(msg);
   assert(failure);

   // Send error back to TU?
}

void
StunTransactionState::processReliability(TransportType type)
{
   switch (type)
   {
      case UDP:
      case DCCP:
         if (mIsReliable)
         {
            mIsReliable = false;
            StackLog (<< "Unreliable transport: " << *this);

            // Put in correct timer....
            mController.mTimers.add(Timer::TimerE1, mId, Timer::T1 );
         }
         break;
         
      default:
         if (!mIsReliable)
         {
            mIsReliable = true;
         }
         break;
   }
}

void
StunTransactionState::sendToWire(TransactionMessage* msg, bool resend) 
{
   StunMessage* stun = dynamic_cast<StunMessage*>(msg);

   if (!stun)
   {
      CritLog(<<"sendToWire: message not a stun message at address " << (void*)stun);
      assert(stun);
      return;
   }

   processReliability(stun->getDestination().getType());
   mController.mTransportSelector.transmit(stun, stun->getDestination()); 
}

void
StunTransactionState::add(const Data& tid)
{
   mController.mClientTransactionMap.add(tid, this);
}

void
StunTransactionState::erase(const Data& tid)
{
   mController.mClientTransactionMap.erase(tid);
}

bool
StunTransactionState::isRequest(TransactionMessage* msg) const
{
   StunMessage* stun = dynamic_cast<StunMessage*>(msg);   
   return stun && stun->isRequest();
}

bool
StunTransactionState::isResponse(TransactionMessage* msg) const
{
   StunMessage* stun = dynamic_cast<StunMessage*>(msg);
   return stun && stun->isResponse();
}

bool
StunTransactionState::isFromTU(TransactionMessage* msg) const
{
   StunMessage* stun = dynamic_cast<StunMessage*>(msg);
   return stun && !stun->isExternal();
}

bool
StunTransactionState::isFromWire(TransactionMessage* msg) const
{
   StunMessage* stun = dynamic_cast<StunMessage*>(msg);
   return stun && stun->isExternal();
}

std::ostream& 
resip::operator<<(std::ostream& strm, const resip::StunTransactionState& state)
{
   strm << "tid=" << state.mId << " [ ";
   switch (state.mState)
   {
      case StunTransactionState::Trying:
         strm << "Trying";
         break;
      case StunTransactionState::Completed:
         strm << "Completed";
         break;
      case StunTransactionState::Bogus:
         strm << "Bogus";
         break;
   }
   
   strm << (state.mIsReliable ? " reliable" : " unreliable");
   strm << " target=" << state.mResponseTarget;
   //if (state.mTransactionUser) strm << " tu=" << *state.mTransactionUser;
   //else strm << "default TU";
   strm << "]";
   return strm;
}


/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */

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
