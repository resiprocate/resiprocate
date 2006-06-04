#include "resip/dumer/PrdCommand.hxx"
#include "resip/dumer/PrdManager.hxx"
#include "rutil/Postable.hxx"

using namespace resip;

PrdCommand::PrdCommand(PrdManager& prdm) : mPrdManager(prdm)
{
}

ManagePrdCommand::ManagePrdCommand(PrdManager& prdm, SharedPtr<Prd> prd) : 
   PrdCommand(prdm),
   mPrd(prd)
{
}

void 
ManagePrdCommand::operator()
{
   mPrdManager.internalManage(mPrd);
}

UnmanagePrdCommand::UnmanagePrdCommand(PrdManager& prdm, SharedPtr<Prd> prd) : 
   PrdCommand(prdm),
   mPrd(prd)
{
}

void 
UnmanagePrdCommand::operator()
{
   mPrdManager.internalUnmanage(mPrd);
}

SendPrdCommand::SendPrdCommand(PrdManager& prdm, std::auto_ptr<SipMessage> msg) : 
   PrdCommand(prdm),
   mMsg(msg)
{
}

void 
SendPrdCommand::operator()()
{
   mPrdManager.internalSend(mMsg);
}

