#include "BaseCreator.hxx"

BaseCreator::BaseCreator(DialogUsageManager& dum) : mDum(dum)
{
}

SipMessage& 
BaseCreator::getLastRequest()
{
   return mLastRequest;
}

void
BaseCreator::makeInitialRequest(const NameAddr& target, MethodTypes method)
{
   RequestLine rLine(method);
   rLine.uri() = target.uri();
   mLastRequest.header(h_RequestLine) = rLine;
   mLastRequest.header(h_To) = target;
   mLastRequest.header(h_MaxForwards).value() = 70;
   mLastRequest.header(h_CSeq).method() = method;
   mLastRequest.header(h_CSeq).sequence() = 1;
   mLastRequest.header(h_From) = mDum.getProfile()->getDefaultAor();
   mLastRequest.header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   mLastRequest.header(h_CallId).value() = Helper::computeCallId();

   NameAddr contact;
   mLastRequest.header(h_Contacts).push_front(contact);
   
   Via via;
   mLastRequest.header(h_Vias).push_front(via);
}
