#include "resip/dumer/PrdManagerCommand.hxx"
#include "resip/dumer/PrdManager.hxx"
#include "rutil/Postable.hxx"

using namespace resip;

PrdManagerCommand::PrdManagerCommand(PrdManager& prdm) : mPrdManager(prdm)
{
}

ManagePrdManagerCommand::ManagePrdManagerCommand(PrdManager& prdm, SharedPtr<Prd> prd) : 
   PrdManagerCommand(prdm),
   mPrd(prd)
{
}

void 
ManagePrdManagerCommand::operator()
{
   mPrdManager.internalManage(mPrd);
}

UnmanagePrdManagerCommand::UnmanagePrdManagerCommand(PrdManager& prdm, SharedPtr<Prd> prd) : 
   PrdManagerCommand(prdm),
   mPrd(prd)
{
}

void 
UnmanagePrdManagerCommand::operator()
{
   mPrdManager.internalUnmanage(mPrd);
}

SendPrdManagerCommand::SendPrdManagerCommand(PrdManager& prdm, std::auto_ptr<SipMessage> msg) : 
   PrdManagerCommand(prdm),
   mMsg(msg)
{
}

void 
SendPrdManagerCommand::operator()()
{
   mPrdManager.internalSend(mMsg);
}

DumTimeoutPrdManagerCommand::DumTimeoutPrdManagerCommand(PrdManager& prdm, std::auto_ptr<SipMessage> msg) : 
   PrdManagerCommand(prdm),
   mMsg(msg)
{
}

void 
DumTimeoutPrdManagerCommand::operator()()
{
   mPrdManager.internalSend(mMsg);
}

