#if !defined(RESIP_CLIENTPUBLICATION_HXX)
#define RESIP_CLIENTPUBLICATION_HXX

namespace resip
{

/** @file ClientPublication.hxx
 *   @todo This file is empty
 */

class ClientPublication: public BaseUsage 
{
      
   public:
      class Handle
      {
      };
      
      ClientPublication(DialogUsageManager& dum, const SipMessage& pub);
      
      void refresh(int expiration=0);
      void update(const Contents* body);
      void end();
};
 
}

#endif

   
