#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/Profile.hxx"
#include "resiprocate/dum/BaseCreator.hxx"

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
      if (mDum.getProfile()->hasOverrideHostAndPort())
      {
         contact.uri() = mDum.getProfile()->getOverideHostAndPort();
      }
      contact.uri().user() = from.uri().user();      
      mLastRequest.header(h_Contacts).push_front(contact);
   }
      
   Via via;
   mLastRequest.header(h_Vias).push_front(via);

   if(mDum.getProfile()->isAdvertisedCapability(Headers::Allow)) mLastRequest.header(h_Allows) = mDum.getProfile()->getAllowedMethods();
   if(mDum.getProfile()->isAdvertisedCapability(Headers::AcceptEncoding)) mLastRequest.header(h_AcceptEncodings) = mDum.getProfile()->getSupportedEncodings();
   if(mDum.getProfile()->isAdvertisedCapability(Headers::AcceptLanguage)) mLastRequest.header(h_AcceptLanguages) = mDum.getProfile()->getSupportedLanguages();
   if(mDum.getProfile()->isAdvertisedCapability(Headers::Supported)) mLastRequest.header(h_Supporteds) = mDum.getProfile()->getSupportedOptionTags();

   DebugLog ( << "BaseCreator::makeInitialRequest: " << mLastRequest);
}
