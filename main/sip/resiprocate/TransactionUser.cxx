#include  "resiprocate/TransactionUser.hxx"

using namespace resip;

TransactionUser::TransactionUser() : mFifo(0, 0)
{
}

TransactionUser::~TransactionUser()
{
}

void 
TransactionUser::postToTransactionUser(Message* msg, TimeLimitFifo<Message>::DepthUsage usage)
{
   mFifo.add(msg, usage);
}

unsigned int 
TransactionUser::size() const
{
   return mFifo.size();
}    

bool 
TransactionUser::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
{
   return mFifo.wouldAccept(usage);
}

