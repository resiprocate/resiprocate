#include "resiprocate/TuSelector.hxx"
#include "resiprocate/TransactionUser.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/os/TimeLimitFifo.hxx"

using namespace resip;

TuSelector::TuSelector(TimeLimitFifo<Message>& fallBackFifo) :
   mFallBackFifo(fallBackFifo) ,
   mTuSelectorMode(false)
{
}

void
TuSelector::add(Message* msg, TimeLimitFifo<Message>::DepthUsage usage)
{
   if (msg->hasTransactionUser() && exists(msg->getTransactionUser()))
   {
      msg->getTransactionUser()->postToTransactionUser(msg, usage);
   }
   else
   {
      mFallBackFifo.add(msg, usage);
   }
}

bool
TuSelector::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
{
   if (mTuSelectorMode)
   {
      for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
      {
         if ((*it)->wouldAccept(usage))
         {
            return true;
         }
      }
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
         total += (*it)->size();
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
   mTuList.push_back(&tu);
}

TransactionUser* 
TuSelector::selectTransactionUser(const SipMessage& msg)
{
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if ((*it)->isForMe(msg))
      {
         return (*it);
      }
   }
   return 0;
}

bool
TuSelector::exists(TransactionUser* tu)
{
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if ((*it) == tu)
      {
         return true;
      }
   }
   return false;
}

