#if !defined(RESIP_STATELESS_HANDLER_HXX)
#define RESIP_STATELESS_HANDLER_HXX

#include "resiprocate/os/HashMap.hxx"
#include "resiprocate/DnsHandler.hxx"

namespace resip
{

class TransactionController;
class TransportSelector;

class StatelessHandler 
{
   public:
      StatelessHandler(TransactionController& c);
      void process();
      
   private:
      TransactionController& mController;
};

class StatelessMessage : public DnsHandler
{
   public:
      StatelessMessage(TransportSelector& selector, SipMessage* msg);
      ~StatelessMessage() {};
         
      void handle(DnsResult* result);

   private:
      TransportSelector& mSelector;
      SipMessage* mMsg;
};

}

#endif
