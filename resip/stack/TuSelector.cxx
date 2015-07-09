#include "resip/stack/ConnectionTerminated.hxx"
#include "resip/stack/KeepAlivePong.hxx"
#include "resip/stack/TuSelector.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/SipStack.hxx"

#include "rutil/TimeLimitFifo.hxx"
#include "rutil/AsyncProcessHandler.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

using namespace resip;

TuSelector::TuSelector(TimeLimitFifo<Message>& fallBackFifo) :
   mFallBackFifo(fallBackFifo), 
   mCongestionManager(0),
   mFallbackPostNotify(0),
   mTuSelectorMode(false),
   mStatsPayload()
{
   mShutdownFifo.setDescription("TuSelector::mShutdownFifo");
}

TuSelector::~TuSelector()
{
   //assert(mTuList.empty());
}


void
TuSelector::setFallbackPostNotify(AsyncProcessHandler *handler) 
{
    mFallbackPostNotify = handler;
}

void
TuSelector::process()
{
   if (mShutdownFifo.messageAvailable())
   {
      TransactionUserMessage* msg = mShutdownFifo.getNext();
      
      switch (msg->type())
      {
         case TransactionUserMessage::RequestShutdown:
            InfoLog (<< "TransactionUserMessage::RequestShutdown " << *(msg->getTransactionUser()));
            markShuttingDown(msg->getTransactionUser());
            break;
         case TransactionUserMessage::RemoveTransactionUser:
            InfoLog (<< "TransactionUserMessage::RemoveTransactionUser " << *(msg->getTransactionUser()));
            remove(msg->getTransactionUser());
            break;
         default:
            resip_assert(0);
            break;
      }
      delete msg;
   }
}

void
TuSelector::add(Message* msg, TimeLimitFifo<Message>::DepthUsage usage)
{
   if (msg->hasTransactionUser())
   {
      if (exists(msg->getTransactionUser()))
      {
         DebugLog (<< "Send to " << *(msg->getTransactionUser()) << " " << std::endl << std::endl << *msg);
         msg->getTransactionUser()->postToTransactionUser(msg, usage);
      }
      else
      {
         WarningLog (<< "Send to TU that no longer exists: " << std::endl << std::endl << *msg);
         delete msg;
      }
   }
   else
   {
      StatisticsMessage* stats = dynamic_cast<StatisticsMessage*>(msg);
      if (stats)
      {
         InfoLog(<< "Stats message " );
         stats->loadOut(mStatsPayload);
         stats->logStats(RESIPROCATE_SUBSYSTEM, mStatsPayload);
         delete msg;
      }
      else
      {
         DebugLog(<< "Send to default TU: " << std::endl << std::endl << *msg);
         mFallBackFifo.add(msg, usage);
         if ( mFallbackPostNotify )
            mFallbackPostNotify->handleProcessNotification();
      }
   }
}

void
TuSelector::add(ConnectionTerminated* term)
{
   InfoLog (<< "Sending " << *term << " to TUs");
   
   for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if (!it->shuttingDown && it->tu->isRegisteredForConnectionTermination())
      {
         it->tu->post(term->clone());
      }
   }
}

void 
TuSelector::add(KeepAlivePong* pong)
{
   //InfoLog (<< "Sending " << *pong << " to TUs");
   
   for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if (!it->shuttingDown && it->tu->isRegisteredForKeepAlivePongs())
      {
         it->tu->post(pong->clone());
      }
   }
}

bool
TuSelector::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
{
   if (mTuSelectorMode)
   {
      for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
      {
         if (!it->shuttingDown  && !it->tu->wouldAccept(usage))
         {
            return false;
         }
      }
      return true;
   }
   else
   {
      return mFallBackFifo.wouldAccept(usage);
   }
}
      
unsigned int 
TuSelector::size() const      
{
   if (mTuSelectorMode)
   {
      unsigned int total=0;   
      for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
      {
         total += it->tu->size();
      }
      return total;
   }
   else
   {
      return mFallBackFifo.size();
   }
}

void 
TuSelector::registerTransactionUser(TransactionUser& tu)
{
   mTuSelectorMode = true;
   mTuList.push_back(Item(&tu));
}

void
TuSelector::requestTransactionUserShutdown(TransactionUser& tu)
{
   TransactionUserMessage* msg = new TransactionUserMessage(TransactionUserMessage::RequestShutdown, &tu);
   mShutdownFifo.add(msg);
}

void
TuSelector::unregisterTransactionUser(TransactionUser& tu)
{
   TransactionUserMessage* msg = new TransactionUserMessage(TransactionUserMessage::RemoveTransactionUser, &tu);
   mShutdownFifo.add(msg);
}

TransactionUser* 
TuSelector::selectTransactionUser(const SipMessage& msg)
{
   DebugLog(<< "TuSelector::selectTransactionUser: Checking which TU message belongs to:" << std::endl << std::endl << msg);
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if (it->tu->isForMe(msg))
      {
         return it->tu;
      }
   }
   return 0;
}

void
TuSelector::markShuttingDown(TransactionUser* tu)
{
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if (it->tu == tu)
      {
         it->shuttingDown = true;
         return;
      }
   }
   resip_assert(0);
}

void
TuSelector::remove(TransactionUser* tu)
{
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if (it->tu == tu)
      {
         TransactionUserMessage* done = new TransactionUserMessage(TransactionUserMessage::TransactionUserRemoved, tu);
         tu->post(done);
         mTuList.erase(it);
         return;
      }
   }
   resip_assert(0);
}

bool
TuSelector::exists(TransactionUser* tu)
{
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if (it->tu == tu)
      {
         return true;
      }
   }
   return false;
}

unsigned int 
TuSelector::getTimeTillNextProcessMS()
{
    if(mShutdownFifo.messageAvailable()) //  || !mFallBackFifo.messageAvailable())  // .slg. fallback fifo is not really used
    {
        return 0;
    }
    else
    {
        return INT_MAX;
    }
} 

bool
TuSelector::isTransactionUserStillRegistered(const TransactionUser* tu) const
{
   if (mTuSelectorMode)
   {
      for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
      {
         if (!it->shuttingDown  && it->tu == tu)
         {
            return true;
         }
      }
   }
   return false;
}

void 
TuSelector::setCongestionManager(CongestionManager* manager)
{
   for(TuList::iterator i=mTuList.begin(); i!=mTuList.end();++i)
   {
      i->tu->setCongestionManager(manager);
   }
   
   // Note:  We are intentionally not registering the following Fifos
   // mFallbackFifo - this is the same fifo as SipStack::TuFifo (passed in 
   //                 constructor).
   // mShutdownFifo - since it is only used at shutdown time, it doesn need
   //                 congestion management
}

CongestionManager::RejectionBehavior 
TuSelector::getRejectionBehavior(TransactionUser* tu) const
{
   if(!mCongestionManager)
   {
      return CongestionManager::NORMAL;
   }

   if(tu)
   {
      return tu->getRejectionBehavior();
   }

   return mCongestionManager->getRejectionBehavior(&mFallBackFifo);
}

UInt32 
TuSelector::getExpectedWait(TransactionUser* tu) const
{
   if(tu)
   {
      return tu->getExpectedWait();
   }

   return (UInt32)mFallBackFifo.expectedWaitTimeMilliSec();
}



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
