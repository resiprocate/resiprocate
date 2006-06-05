#if !defined(Dum_Factory_hxx)
#define Dum_Factory_hxx

namespace resip
{

class InviteSessionFactory
{
   public:
      InviteSession* create(const SipMessage& msg)=0;
};

class SubscriptionFactory
{
   public:
      ServerSubscription* create(const SipMessage& msg)=0;
};

class PublicationFactory
{
   public:
      ServerPublication* create(const SipMessage& msg)=0;
};

class RegistrationFactory
{
   public:
      ServerRegistration* create(const SipMessage& msg)=0;
};

class PrdTransactionFactory
{
   public:
      PrdServerTransaction* create(const SipMessage& msg)=0;
};

   

#endif
