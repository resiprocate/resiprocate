#if !defined(RESIP_REGISTRATIONCREATOR_HXX)
#define RESIP_REGISTRATIONCREATOR_HXX

#include "resiprocate/dum/BaseCreator.hxx"

namespace resip
{

class DialogUsageManager;
class NameAddr;

/**
  This class creates registration requests on behalf of a client.
  It has nothing to do with server handling of registration requests.
 */

class RegistrationCreator : public BaseCreator 
{
   public:
      RegistrationCreator(DialogUsageManager& dum, const NameAddr& target, int RegistrationTime);
};
 
}

#endif
