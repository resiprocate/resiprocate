#if !defined(RESIP_REGISTRATIONCREATOR_HXX)
#define RESIP_REGISTRATIONCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class DialogUsageManager;

class RegistrationCreator : public BaseCreator 
{
   public:
      RegistrationCreator(DialogUsageManager& dum, const NameAddr& aor);
};
 
}

#endif
