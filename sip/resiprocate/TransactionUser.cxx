#include  "resiprocate/TransactionUser.hxx"

TransactionUser::TransactionUser()
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

virtual 
unsigned int TransactionUser::size() const
{
   return mFifo.size();
}    

bool 
TransactionUser::wouldAccept(TimeLimitFifo<Message>::DepthUsage usage) const
{
   return mFifo.wouldAccept(usage);
}

