#if !defined(RESIP_REGISTRATIONCREATOR_HXX)
#define RESIP_REGISTRATIONCREATOR_HXX


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
