#include "resiprocate/TuSelector.hxx"
#include "resiprocate/TransactionUser.hxx"
#include "resiprocate/TransactionUserMessage.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/os/TimeLimitFifo.hxx"

#include "resiprocate/os/Logger.hxx"
#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

using namespace resip;

TuSelector::TuSelector(TimeLimitFifo<Message>& fallBackFifo) :
   mFallBackFifo(fallBackFifo) ,
   mTuSelectorMode(false),
   mStatsPayload()
{
}

TuSelector::~TuSelector()
{
   //assert(mTuList.empty());
}

void
TuSelector::process()
{
   if (mShutdownFifo.messageAvailable())
   {
      TransactionUserMessage* msg = mShutdownFifo.getNext();
      
      switch (msg->type() )
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
   }
}

void
TuSelector::add(Message* msg, TimeLimitFifo<Message>::DepthUsage usage)
{
   if (msg->hasTransactionUser())
   {
      if (exists(msg->getTransactionUser()))
      {
         msg->getTransactionUser()->postToTransactionUser(msg, usage);
      }
      else
      {
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
      }
      else
      {
         mFallBackFifo.add(msg, usage);
      }
   }
}

bool
TuSelector::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
{
   if (mTuSelectorMode)
   //InfoLog(<< "TuSelector::wouldAccept " << mTuSelectorMode);   
   {
      for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
      {
         if (!it->shuttingDown && it->tu->wouldAccept(usage))
         {
            //InfoLog(<< "TuSelector::wouldAccept returning true");   
            return true;
         }
      }
      //InfoLog(<< "TuSelector::wouldAccept returning false");   
      return false;
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
         tu->postToTransactionUser(done, TimeLimitFifo<resip::Message>::InternalElement);
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

