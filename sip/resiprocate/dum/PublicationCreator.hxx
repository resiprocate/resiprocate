#if !defined(RESIP_PUBLICATIONCREATOR_HXX)
#define RESIP_PUBLICATIONCREATOR_HXX

#include "BaseCreator.hxx"

namespace resip
{

class Uri;

/** @file PublicationCreator.hxx
 *   @todo This file is empty
 */

class PublicationCreator: public BaseCreator
{
   public:
      PublicationCreator(Uri& aor);

      virtual void dispatch(SipMessage& msg);
};
 
}

#endif
