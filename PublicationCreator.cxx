#include "PublicationCreator.hxx"
#include "resiprocate/SipMessage.hxx"

using namespace resip;

PublicationCreator::PublicationCreator(DialogUsageManager& dum,
                                       const Uri& targetDocument, 
                                       const Contents& body, 
                                       const Data& eventType, 
                                       unsigned expireSeconds )
   : BaseCreator(dum)
{
   makeInitialRequest(NameAddr(targetDocument), PUBLISH);

   mLastRequest.header(h_Event).value() = eventType;
   mLastRequest.setContents(&body);
   mLastRequest.header(h_Expires).value() = expireSeconds;
}

