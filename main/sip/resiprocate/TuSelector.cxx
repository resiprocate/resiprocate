#include "resiprocate/TuSelector.hxx"
#include "resiprocate/TransactionUser.hxx"
#include "resiprocate/SipStack.hxx"
#include "resiprocate/os/TimeLimitFifo.hxx"

using namespace resip;


void
TuSelector::add(Message* msg, TimeLimitFifo<Message>::DepthUsage usage)
{
   if (msg->hasTransactionUser())
   {
      msg->getTransactionUser()->postToTransactionUser(msg, usage);
   }
   else
   {
      mFallBackFifo.add(msg, usage);
   }
}

bool 
TuSelector::messageAvailable() const
{
   return mFallBackFifo.messageAvailable();
   
//    for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
//    {
//       if ((*it)->messageAvailable())
//       {
//          return true;
//       }
//    }
//    return false;   
}

bool
TuSelector::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
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
      
unsigned int 
TuSelector::size() const      
{
   unsigned int total=0;   
   for(TuList::const_iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      total += (*it)->size();
   }
   return total;   
}

void 
TuSelector::registerTransactionUser(TransactionUser& tu)
{
   mTuList.push_back(&tu);
}

TransactionUser* 
TuSelector::selectTransactionUser(const SipMessage& msg)
{
   for(TuList::iterator it = mTuList.begin(); it != mTuList.end(); it++)
   {
      if ((*it)->isForMe(msg))
      {
         return *it;
      }
   }
   return 0;
}
