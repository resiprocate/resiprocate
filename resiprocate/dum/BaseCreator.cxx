#include "resiprocate/Helper.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/MasterProfile.hxx"
#include "resiprocate/dum/BaseCreator.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

BaseCreator::BaseCreator(DialogUsageManager& dum, UserProfile &userProfile) : mDum(dum), mUserProfile(userProfile)
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

UserProfile&
BaseCreator::getUserProfile()
{
   return mUserProfile;
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
   mLastRequest.header(h_From) = mUserProfile.getDefaultFrom();
   mLastRequest.header(h_From).param(p_tag) = Helper::computeTag(Helper::tagSize);
   mLastRequest.header(h_CallId).value() = Helper::computeCallId();

   NameAddr contact; // if no GRUU, let the stack fill in the contact 
   if (mUserProfile.hasGruu(target.uri().getAor()))
   {
      contact = mUserProfile.getGruu(target.uri().getAor());
      mLastRequest.header(h_Contacts).push_front(contact);
   }
   else
   {
      if (mUserProfile.hasOverrideHostAndPort())
      {
         contact.uri() = mUserProfile.getOverrideHostAndPort();
      }
      contact.uri().user() = mUserProfile.getDefaultFrom().uri().user();
      const Data& instanceId = mUserProfile.getInstanceId();
      if (!instanceId.empty())
      {
         contact.uri().param(p_Instance) = instanceId;
      }
      mLastRequest.header(h_Contacts).push_front(contact);
   }
      
   Via via;
   mLastRequest.header(h_Vias).push_front(via);

   if(mUserProfile.isAdvertisedCapability(Headers::Allow)) mLastRequest.header(h_Allows) = mDum.getMasterProfile()->getAllowedMethods();
   if(mUserProfile.isAdvertisedCapability(Headers::AcceptEncoding)) mLastRequest.header(h_AcceptEncodings) = mDum.getMasterProfile()->getSupportedEncodings();
   if(mUserProfile.isAdvertisedCapability(Headers::AcceptLanguage)) mLastRequest.header(h_AcceptLanguages) = mDum.getMasterProfile()->getSupportedLanguages();
   if(mUserProfile.isAdvertisedCapability(Headers::Supported)) mLastRequest.header(h_Supporteds) = mDum.getMasterProfile()->getSupportedOptionTags();

   DebugLog ( << "BaseCreator::makeInitialRequest: " << mLastRequest);
}

