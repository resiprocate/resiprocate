#ifndef CheckPrivacy_hxx
#define CheckPrivacy_hxx

#include "tfm/Event.hxx"
#include "resip/stack/SipMessage.hxx"

#include <memory>

class CheckPrivacy
{
   public:
      CheckPrivacy() = default;

      bool operator()(std::shared_ptr<Event> event) const;
};

#endif
