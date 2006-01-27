#include "repro/monkeys/QValueTargetHandler.hxx"

#include <vector>
#include <deque>

#include "repro/RequestContext.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/ForkControlMessage.hxx"
#include "repro/QValueTarget.hxx"

#include "rutil/Data.hxx"


namespace repro
{
QValueTargetHandler::QValueTargetHandler(ForkBehavior behavior,
                                          bool cancelBetweenForkGroups,
                                          bool waitForTerminate,
                                          int delayBetweenForkGroupsMS,
                                          int cancellationDelayMS)
{
   mForkBehavior=behavior;
   mCancelBetweenForkGroups=cancelBetweenForkGroups;
   mWaitForTerminate=waitForTerminate;
   mDelayBetweenForkGroups=delayBetweenForkGroupsMS;
   mCancellationDelay=cancellationDelayMS;
}


QValueTargetHandler::~QValueTargetHandler()
{}

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
   assert(msg);


   repro::ForkControlMessage* fc = dynamic_cast<repro::ForkControlMessage*>(msg);
   
   if(fc)
   {
      shouldContinue=false;
      
      std::vector<resip::Data>::iterator i;
      
      //Might we have scheduled cancellations?
      if(mCancelBetweenForkGroups)
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

         //Are there active targets in this queue already?
         bail=false;
         for(;inner!=outer->end() && !bail;inner++)
         {
            if(rsp.isActive(*inner))
            {
               activeTargets=true;
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
            std::vector<resip::Data> beginTargets;
            
            fillNextTargetGroup(beginTargets,*outer,rsp);
            
            if(beginTargets.empty())
            {
               bail=true;
            }
            else
            {
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
            }
         }
            
         //If we just started some targets, and we are not supposed to wait
         //for these targets to terminate before beginning the next group
         //in this queue, we should schedule the next group now.
         //If there is no next group, nextBeginTids will be empty.
         if(startedTargets && !mWaitForTerminate)
         {
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
   

      assert(activeTargets || !startedTargets);
   }
   
   //Do we have anything to schedule for later?
   if(!nextCancelTids.empty() || !nextBeginTids.empty())
   {
      if(mCancellationDelay==mDelayBetweenForkGroups)
      {
         ForkControlMessage* fork = new ForkControlMessage(*this,tid,proxy);
         fork->mTransactionsToProcess=nextBeginTids;
         fork->mTransactionsToCancel=nextCancelTids;

         resip::ApplicationMessage* app=
                           dynamic_cast<resip::ApplicationMessage*>(fork);
                           
         proxy->postMS(std::auto_ptr<resip::ApplicationMessage>(app),
                              mDelayBetweenForkGroups);
      }
      else
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
   if(!activeTargets && shouldContinue )
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
   std::list<resip::Data>::const_iterator i = queue.begin();
   float currentQ=0;
   

   //Find the first Candidate target in the queue, noting
   // whether there are any active targets.
   while(i!=queue.end())
   {

      if(rsp.isCandidate(*i))
      {
         currentQ=rsp.getTarget(*i)->getPriority();
         break;
      }

      i++;
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
         }
         break;
         
      case FULL_PARALLEL:
         while(i!=queue.end())
         {
            fillHere.push_back(*i);
         }   
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


void 
QValueTargetHandler::dump(std::ostream &os) const
{
  os << "QValueTargetHandler baboon" << std::endl;
}




}