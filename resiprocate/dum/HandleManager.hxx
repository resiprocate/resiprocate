#if !defined(RESIP_HandleManager_HXX)
#define RESIP_HandleManager_HXX

#include "resiprocate/os/HashMap.hxx"
#include "resiprocate/dum/Handled.hxx"

namespace resip
{

class HandleManager
{
   public:
      HandleManager();
      virtual ~HandleManager();

      bool isValidHandle(Handled::Id) const;
      Handled* getHandled(Handled::Id) const;

   private:
      friend class Handled;
      
      Handled::Id create(Handled* handled);
      void remove(Handled::Id id);

      typedef HashMap<Handled::Id, Handled*> HandleMap;
      HandleMap mHandleMap;
      Handled::Id mLastId;
};
 
}

#endif
