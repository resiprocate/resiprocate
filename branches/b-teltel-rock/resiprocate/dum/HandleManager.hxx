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

      virtual void shutdownWhenEmpty();
      //subclasses(for now DUM) overload this method to handle shutdown
   protected:
      virtual void shutdown();      
   private:
      friend class Handled;
      
      Handled::Id create(Handled* handled);
      void remove(Handled::Id id);

      typedef HashMap<Handled::Id, Handled*> HandleMap;
      HandleMap mHandleMap;
      bool mShuttingDown;      
      Handled::Id mLastId;
};
 
}

#endif
