#include "PublicationCreator.hxx"
#include "resiprocate/SipMessage.hxx"

using namespace resip;

PublicationCreator::PublicationCreator(DialogUsageManager& dum,
                                       const NameAddr& target, 
                                       Identity& identity,
                                       const Contents& body, 
                                       const Data& eventType, 
                                       unsigned expireSeconds )
   : BaseCreator(dum, identity)
{
   makeInitialRequest(target, PUBLISH);

   mLastRequest.header(h_Event).value() = eventType;
   mLastRequest.setContents(&body);
   mLastRequest.header(h_Expires).value() = expireSeconds;
}

