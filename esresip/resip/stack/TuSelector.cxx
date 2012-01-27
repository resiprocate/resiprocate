/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#include "resip/stack/ConnectionTerminated.hxx"
#include "resip/stack/TuSelector.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/TransactionUserMessage.hxx"
#include "resip/stack/SipStack.hxx"

#include "rutil/Fifo.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

using namespace resip;

TuSelector::TuSelector(Fifo<Message>& fallBackFifo) :
   mFallBackFifo(fallBackFifo) ,
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
TuSelector::process()
{
   static int count=0;
   if(count%128==0)
   {
      if (mShutdownFifo.messageAvailable())
      {
         TransactionUserMessage* msg = mShutdownFifo.getNext();
         
         switch (msg->type())
         {
            case TransactionUserMessage::RequestShutdown:
               InfoLog (<< "TransactionUserMessage::RequestShutdown " << *(msg->tu));
               markShuttingDown(msg->tu);
               break;
            case TransactionUserMessage::RemoveTransactionUser:
               InfoLog (<< "TransactionUserMessage::RemoveTransactionUser " << *(msg->tu));
               remove(msg->tu);
               break;
            default:
               assert(0);
               break;
         }
         delete msg;
      }
      count=0;
   }
   ++count;
}

void
TuSelector::add(Message* msg)
{
   if (msg->hasTransactionUser())
   {
      if (exists(msg->getTransactionUser()))
      {
         DebugLog (<< "Send to TU: " << *(msg->getTransactionUser()) << " " << std::endl << std::endl << *msg);
         msg->getTransactionUser()->postToTransactionUser(msg);
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
         mFallBackFifo.add(msg);
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
   assert(0);
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
   assert(0);
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
   
   // ?bwc? Do we need to congestion-manage mFallbackFifo or mShutdownFifo?

}

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
