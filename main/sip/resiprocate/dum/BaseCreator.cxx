#include "DialogUsageManager.hxx"
#include "Profile.hxx"
#include "BaseCreator.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

BaseCreator::BaseCreator(DialogUsageManager& dum) : mDum(dum)
{
}

BaseCreator::~BaseCreator()
{}

SipMessage& 
BaseCreator::getLastRequest()
{
   return mLastRequest;
}

const SipMessage& 
BaseCreator::getLastRequest() const
{
   return mLastRequest;
}

void 
BaseCreator::makeInitialRequest(const NameAddr& target, const NameAddr& from, MethodTypes method)
{
   RequestLine rLine(method);
   rLine.uri() = target.uri();   
   mLastRequest.header(h_RequestLine) = rLine;

   mLastRequest.header(h_To) = target;
   mLastRequest.header(h_MaxForwards).value() = 70;
   mLastRequest.header(h_CSeq).method() = method;
   mLastRequest.header(h_CSeq).sequence() = 1;
   mLastRequest.header(h_From) = from;
   mLastRequest.header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   mLastRequest.header(h_CallId).value() = Helper::computeCallId();

   NameAddr contact; // if no GRUU, let the stack fill in the contact 
   if (mDum.getProfile()->hasGruu(target.uri().getAor()))
   {
      contact = mDum.getProfile()->getGruu(target.uri().getAor());
      mLastRequest.header(h_Contacts).push_front(contact);
   }
   else
   {
      contact.uri().user() = from.uri().user();      
      mLastRequest.header(h_Contacts).push_front(contact);
   }
      
   Via via;
   mLastRequest.header(h_Vias).push_front(via);

   mLastRequest.header(h_Supporteds) = mDum.getProfile()->getSupportedOptionTags();
   mLastRequest.header(h_Accepts) = mDum.getProfile()->getSupportedMimeTypes();
   
   DebugLog ( << "BaseCreator::makeInitialRequest: " << mLastRequest);
}
