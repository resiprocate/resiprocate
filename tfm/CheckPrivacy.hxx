#ifndef CheckPrivacy_hxx
#define CheckPrivacy_hxx

#include "tfm/Event.hxx"
#include "resip/stack/SipMessage.hxx"                                         
#include <boost/shared_ptr.hpp>


class CheckPrivacy
{
   public:
      CheckPrivacy()
      {
      }

      bool operator()(boost::shared_ptr<Event> event) const;
};

#endif
