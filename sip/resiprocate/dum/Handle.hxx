#if !defined(RESIP_HANDLE_HXX)
#define RESIP_HANDLE_HXX

#include "resiprocate/dum/Handled.hxx"
#include "resiprocate/dum/HandleManager.hxx"

namespace resip
{

template <class T>
class Handle
{
   public:
      Handle(HandleManager& ham, Handled::Id id) : mHam(&ham), mId(id)
      {
      }

      Handle() : mHam(0)
      {
      }

      bool isValid() const
      {
         return mHam->isValidHandle(mId);
      }
      
      // throws if not found
      T* get()
      {
         return static_cast<T*>(mHam->getHandled(mId));
      }
      
      T* operator->()
      {
         return get();
      }

      Handled::Id getId() const
      {
         return mId;
      }

      static Handle<T> NotValid()
      {
         static Handle<T> notValid;
         return notValid;
      }
      

   protected:
      Handle(HandleManager& ham, Handled* handled) : mHam(&ham), mId(mHam->create(this)) 
      {
      }

      //for invalid handles

   private:
      HandleManager* mHam;

   protected:
      Handled::Id mId;

      friend class Handled;
};

}

#endif
