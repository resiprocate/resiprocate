#if !defined(RESIP_REGISTRATIONCREATOR_HXX)
#define RESIP_REGISTRATIONCREATOR_HXX

/** @file RegistrationCreator.hxx
 *  @todo This file is empty
 */

namespace resip
{

class RegistrationCreator : public BaseCreator {

  public:
    RegistrationCreator(const NameAddrs& desiredBindings);

  private:
    NameAddrs mDesiredBindings;
};
 
}

#endif
