#if !defined(RESIP_PUBLICATIONCREATOR_HXX)
#define RESIP_PUBLICATIONCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class Contents;

class PublicationCreator: public BaseCreator
{
   public:
      PublicationCreator(DialogUsageManager& dum, 
                         const NameAddr& targetDocument, 
                         const NameAddr& from,
                         const Contents& body, 
                         const Data& eventType, 
                         unsigned expireSeconds );
};
 
}

#endif
