#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/BaseCreator.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

BaseCreator::BaseCreator(DialogUsageManager& dum, Identity &identity) : mDum(dum), mIdentity(identity)
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

Identity&
BaseCreator::getIdentity()
{
   return mIdentity;
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
   mLastRequest.header(h_From) = mIdentity.getDefaultFrom();
   mLastRequest.header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   mLastRequest.header(h_CallId).value() = Helper::computeCallId();

   NameAddr contact; // if no GRUU, let the stack fill in the contact 
   if (mIdentity.hasGruu(target.uri().getAor()))
   {
      contact = mIdentity.getGruu(target.uri().getAor());
      mLastRequest.header(h_Contacts).push_front(contact);
   }
   else
   {
      if (mIdentity.hasOverrideHostAndPort())
      {
         contact.uri() = mIdentity.getOverideHostAndPort();
      }
      contact.uri().user() = mIdentity.getDefaultFrom().uri().user();      
      mLastRequest.header(h_Contacts).push_front(contact);
   }
      
   Via via;
   mLastRequest.header(h_Vias).push_front(via);

   if(mIdentity.isAdvertisedCapability(Headers::Allow)) mLastRequest.header(h_Allows) = mDum.getMasterProfile()->getAllowedMethods();
   if(mIdentity.isAdvertisedCapability(Headers::AcceptEncoding)) mLastRequest.header(h_AcceptEncodings) = mDum.getMasterProfile()->getSupportedEncodings();
   if(mIdentity.isAdvertisedCapability(Headers::AcceptLanguage)) mLastRequest.header(h_AcceptLanguages) = mDum.getMasterProfile()->getSupportedLanguages();
   if(mIdentity.isAdvertisedCapability(Headers::Supported)) mLastRequest.header(h_Supporteds) = mDum.getMasterProfile()->getSupportedOptionTags();

   DebugLog ( << "BaseCreator::makeInitialRequest: " << mLastRequest);
}

