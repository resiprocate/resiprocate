#include "repro/monkeys/QValueTargetHandler.hxx"


#include "repro/RequestContext.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/ForkControlMessage.hxx"
#include "repro/QValueTarget.hxx"

#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"
#include "repro/ProxyConfig.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

namespace repro
{

QValueTargetHandler::QValueTargetHandler(ProxyConfig& config) : 
   Processor("QValueTargetHandler")
{
   mForkBehavior=QValueTargetHandler::EQUAL_Q_PARALLEL;
      
   if(config.getConfigData("QValueBehavior", "") =="FULL_SEQUENTIAL")
   {
      mForkBehavior=QValueTargetHandler::FULL_SEQUENTIAL;
   }
   else if(config.getConfigData("QValueBehavior", "") == "FULL_PARALLEL")
   {
      mForkBehavior=QValueTargetHandler::FULL_PARALLEL;
   }

   mCancelBetweenForkGroups=config.getConfigBool("QValueCancelBetweenForkGroups", true);
   mWaitForTerminate=config.getConfigBool("QValueWaitForTerminateBetweenForkGroups", true);
   mDelayBetweenForkGroups=config.getConfigInt("QValueMsBetweenForkGroups", 3000);
   mCancellationDelay=config.getConfigInt("QValueMsBeforeCancel", 30000);
}

QValueTargetHandler::~QValueTargetHandler()
{
}

Processor::processor_action_t
QValueTargetHandler::process(RequestContext &rc)
{
   std::vector<resip::Data> nextCancelTids;
   std::vector<resip::Data> nextBeginTids;

   Proxy* proxy=&(rc.getProxy());

   resip::Data tid = rc.getTransactionId();
   ResponseContext& rsp=rc.getResponseContext();
   bool shouldContinue=true;
   
   //Use this as a loop invariant.
   bool bail=false;

   resip::Message* msg = rc.getCurrentEvent();
   resip_assert(msg);

   if(msg)
   {
      repro::ForkControlMessage* fc = dynamic_cast<repro::ForkControlMessage*>(msg);
      
      if(fc)
      {
         shouldContinue=false;
         std::vector<resip::Data>::iterator i;
         
         //Might we have scheduled cancellations? If so, is there anything else
         //worth trying? (We won't cancel stuff if there is nothing else left 
         //to try)
         if(mCancelBetweenForkGroups && rsp.hasCandidateTransactions())
         {
            std::vector<resip::Data>& cancelTids=fc->mTransactionsToCancel;
            for(i=cancelTids.begin();i!=cancelTids.end();i++)
            {
               //Calling cancelClientTransaction on an already cancelled
               //target is safe, and usually more efficient than checking
               //beforehand.
               rsp.cancelClientTransaction(*i);
            }
         }
         
         //Might we have scheduled some transactions to start?
         //(and if we did, should we now schedule them for cancellation later?)
         if(!mWaitForTerminate)
         {
            std::vector<resip::Data>& beginTids=fc->mTransactionsToProcess;
            for(i=beginTids.begin();i!=beginTids.end();i++)
            {
               //Calling beginClientTransaction on an already active
               // (or Terminated) transaction is safe, and more
               // efficient than checking beforehand.
               rsp.beginClientTransaction(*i);
               if(mCancelBetweenForkGroups)
               {            
                  nextCancelTids.push_back(*i);
               }
            }
         }
      }
      else
      {
         DebugLog(<<"No ForkControlMessage for me.");
      }
   }

   std::list<std::list<resip::Data> >& targetCollection = 
         rsp.mTransactionQueueCollection;
   std::list<std::list<resip::Data> >::iterator outer;
   std::list<std::list<resip::Data> >::iterator temp;
   std::list<resip::Data>::iterator inner;
   bool activeTargets=false;
   bool startedTargets=false;

   for(outer=targetCollection.begin(); 
      outer!=targetCollection.end() && !activeTargets;)
   {
      inner=outer->begin();
      
      if(inner!=outer->end() && isMyType(rsp.getTarget(*inner)))
      {
         DebugLog(<<"QValueTargetHandler: "
                  <<"Found a queue of QValueTargets. Are any of them active?");
         //Are there active targets in this queue already?
         bail=false;
         for(;inner!=outer->end() && !bail;inner++)
         {
            if(rsp.isActive(*inner))
            {
               activeTargets=true;
               DebugLog(<<"There are active targets. "
                        <<"I don't need to do anything else yet.");
               bail=true;
            }
         }

         //If no active targets, we need to start some new ones.
         //(and schedule their cancellation if configured to)
         //However, calling beginClientTransaction does not guarantee that a
         //target will start, since that target may be a duplicate.
         //So, we keep firing up target groups until something sticks, or we hit
         //the end of this queue.
         bail=false;
         while(!activeTargets && !bail)
         {
            DebugLog(<<"There are no active targets here. "
                     <<"Looking for a group to start.");
            std::vector<resip::Data> beginTargets;
            
            fillNextTargetGroup(beginTargets,*outer,rsp);
            
            if(beginTargets.empty())
            {
               DebugLog(<<"There are no more targets to start in this queue."
                        <<" Trying to find another queue to work on.");
               bail=true;
            }
            else
            {
               DebugLog(<<"This queue has a group of targets in it. "
                        <<"Trying to start this group.");
               std::vector<resip::Data>::iterator i;
               for(i=beginTargets.begin();i!=beginTargets.end();i++)
               {
                  bool success = rsp.beginClientTransaction(*i);
                  if(success && mCancelBetweenForkGroups)
                  {
                     nextCancelTids.push_back(*i);
                  }
                  
                  activeTargets |= success;
                  startedTargets |= success;
               }
               if(startedTargets)
               {
                  DebugLog(<<"Successfully started some targets.");
               }
               else
               {
                  DebugLog(<<"None of these Targets were valid!"
                           << " Moving on to another group.");
               }
            }
         }
            
         if(!startedTargets)
         {
            DebugLog(<< "There weren't any valid Targets in this queue!");
         }
         //If we just started some targets, and we are not supposed to wait
         //for these targets to terminate before beginning the next group
         //in this queue, we should schedule the next group now.
         //If there is no next group, nextBeginTids will be empty.
         if(startedTargets && !mWaitForTerminate)
         {
            DebugLog(<<"Now I need to schedule the next group of Targets.");
            fillNextTargetGroup(nextBeginTids,*outer,rsp);
         }
         
         //Clean up the queue we just tried
         removeTerminated(*outer,rsp);
      }
      
      if(outer->empty())
      {
         temp=outer;
         outer++;
         targetCollection.erase(temp);
      }
      else
      {
         outer++;
      }
   
      resip_assert(activeTargets || !startedTargets);
   }
   
   //Do we have anything to schedule for later?
   if(!nextCancelTids.empty() || !nextBeginTids.empty())
   {
      // If the delays are equal, then put both cancel and nextBeginTids in one ForkControlMessage
      if(mCancellationDelay == mDelayBetweenForkGroups)  
      {
         ForkControlMessage* fork = new ForkControlMessage(*this,tid,proxy);
         fork->mTransactionsToProcess=nextBeginTids;
         fork->mTransactionsToCancel=nextCancelTids;

         resip::ApplicationMessage* app=
                           dynamic_cast<resip::ApplicationMessage*>(fork);
                           
         proxy->postMS(std::auto_ptr<resip::ApplicationMessage>(app),
                              mDelayBetweenForkGroups);
      }
      else // Issue two seperate ForkControlMessages
      {
         if(!nextCancelTids.empty())
         {
            ForkControlMessage* cancel = new ForkControlMessage(*this,tid,proxy);
            cancel->mTransactionsToCancel=nextCancelTids;

            resip::ApplicationMessage* app=
                              dynamic_cast<resip::ApplicationMessage*>(cancel);

            rc.getProxy().postMS(std::auto_ptr<resip::ApplicationMessage>(app),
                                 mCancellationDelay);
         }
         if(!nextBeginTids.empty())
         {
            ForkControlMessage* begin = new ForkControlMessage(*this,tid,proxy);
            begin->mTransactionsToProcess=nextBeginTids;

            resip::ApplicationMessage* app
                              =dynamic_cast<resip::ApplicationMessage*>(begin);

            proxy->postMS(std::auto_ptr<resip::ApplicationMessage>(app),
                                 mDelayBetweenForkGroups);
         }
      }
   }

   //We should not pass control on to the rest of the chain until all
   //of the QValueTargets have been taken care of. Also, we should not
   //pass control to the rest of the chain if we received a 
   //ForkControlMessage explicitly intended for us.
   //(this could confuse the other Target Processors)
   if(!activeTargets && shouldContinue)
   {
      return Processor::Continue;
   }
   else
   {
      return Processor::SkipAllChains;
   }
}

void
QValueTargetHandler::fillNextTargetGroup(std::vector<resip::Data>& fillHere,
                        const std::list<resip::Data> & queue,
                        const ResponseContext& rsp) const
{
   if(queue.empty())
   {
      return;
   }

   std::list<resip::Data>::const_iterator i = queue.begin();
   int currentQ=0;
   
   //Find the first Candidate target in the queue.
   for(i=queue.begin();i!=queue.end();i++)
   {
      if(rsp.isCandidate(*i))
      {
         currentQ=rsp.getTarget(*i)->getPriority();
         break;
      }
   }

   switch(mForkBehavior)
   {
      case FULL_SEQUENTIAL:
         if(i!=queue.end())
         {
            fillHere.push_back(*i);
         }
         break;
         
      case EQUAL_Q_PARALLEL:   
         while(i!=queue.end() && rsp.getTarget(*i)->getPriority()==currentQ)
         {
            fillHere.push_back(*i);
            i++;
         }
         break;
         
      case FULL_PARALLEL:
         while(i!=queue.end())
         {
            fillHere.push_back(*i);
            i++;
         }
         break;
         
      default:
         ErrLog(<<"mForkBehavior is not defined! How did this happen?");
   }
}

bool
QValueTargetHandler::isMyType( Target* target) const
{
   QValueTarget* qt = dynamic_cast<QValueTarget*>(target);
   if(qt)
   {
      return true;
   }
   else
   {
      return false;
   }
}

void
QValueTargetHandler::removeTerminated(std::list<resip::Data> & queue,
                                      const ResponseContext& rsp) const
{
   std::list<resip::Data>::iterator i = queue.begin();

   while(i!=queue.end())
   {
      if(rsp.isTerminated(*i))
      {
         std::list<resip::Data>::iterator temp=i;
         i++;
         queue.erase(temp);
      }
      else
      {
         i++;
      }
   }
}

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
 */
