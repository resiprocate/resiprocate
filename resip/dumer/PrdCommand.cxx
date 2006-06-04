#include "resip/dumer/PrdCommand.hxx"
#include "resip/dumer/PrdManager.hxx"
#include "rutil/Postable.hxx"

using namespace resip;

void SipMessagePrdCommand::operator()()
{
   SharedPtr<Ptr> prd(mPrd);
   if (prd.get() != 0)
   {
      prd->onSipMessage(mSipMessage);
   }
   else if (mSipMessage.get() && mSipMessage->isRequest())
   {
      /* create new SIP 481 and tell the other guy to go away */
      SipMessage response = Helper::makeResponse(mSipMessage, 481);
      assert(response.hasTransactionUser());
      mStack.send(response);
   }
}
