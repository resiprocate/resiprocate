#if !defined(RESIP_CLIENTREGISTRATION_HXX)
#define RESIP_CLIENTREGISTRATION_HXX


namespace resip
{

class ClientRegistration: public BaseUsage
{
   public:
      
      class Handle
      {
      };
      
      const NameAddrs& getMyContacts();
      const NameAddrs& getAllContacts();
      void addBinding(const NameAddr& contact);
      void addBinding(const NameAddr& to, const NameAddr& contact);
      void removeBinding(const NameAddr& contact);
      void removeBinding(const NameAddr& to, const NameAddr& contact);
      void removeAll();
      void removeMyBindings();
      void requestRefresh();
      
      const NameAddrs& myContacts();
      const NameAddrs& allContacts();
      
   private:
      NameAddrs mMyContacts;
      NameAddrs mAllContacts;
      UInt64    mExpirationTime;
};
 
}

#endif
