#if !defined BindHandle_hxx
#define BindHandle_hxx

#include "resip/dum/Handles.hxx"
#include "rutil/Data.hxx"
#include <boost/function.hpp>
#include "tfm/ActionBase.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

class TestEndPoint;

template<class T>
class BindHandle : public ActionBase
{
   public:
      BindHandle(TestEndPoint* tua, resip::Data action, typename T::HandleType& handle) : 
         mTestEndPoint(tua),
         mActionName(action),
         mHandle(handle)
         {}
      virtual ~BindHandle() {}

      virtual void operator()(boost::shared_ptr<Event> event) 
      {
         boost::shared_ptr<T> k = boost::dynamic_pointer_cast<T, Event>(event);
         StackLog(<< "Binding handle... handle Id is " << k->getHandle().getId());
         mHandle = k->getHandle();
      }

      virtual void operator()() { resip_assert(0); }

      virtual resip::Data toString() const { return mActionName; }

   protected:
      bool match(boost::shared_ptr<Event> event) const;

      TestEndPoint* mTestEndPoint;
      resip:: Data mActionName;
      typename T::HandleType& mHandle;
};

template<class T1>
class BindHandleInvite : public ActionBase
{
   public:
      BindHandleInvite(TestEndPoint* tua, resip::Data action, typename T1::HandleType& handle, resip::InviteSessionHandle& sessionHandle) : 
         mTestEndPoint(tua),
         mActionName(action),
         mHandle(handle),
         mSessionHandle(sessionHandle)
      {
      }

      virtual ~BindHandleInvite() {}

      virtual void operator()(boost::shared_ptr<Event> event) 
      {
         boost::shared_ptr<T1> k = boost::dynamic_pointer_cast<T1, Event>(event);
         StackLog(<< "Binding invite session  handle... handle Id is " << k->getHandle().getId());
         mHandle = k->getHandle();
         resip_assert(mHandle.isValid());
         mSessionHandle = mHandle->getSessionHandle();
         resip_assert(mSessionHandle.isValid());
      }

      virtual void operator()() { resip_assert(0); }

      virtual resip::Data toString() const { return mActionName; }

   protected:
      bool match(boost::shared_ptr<Event> event) const;

      TestEndPoint* mTestEndPoint;
      resip:: Data mActionName;
      typename T1::HandleType& mHandle; // ClientInviteSessionHandle or ServerInviteSessionHandle
      //typename T2::HanldeType& mSessionHandle; // InviteSessionHandle
      resip::InviteSessionHandle& mSessionHandle;

};

#undef RESIPROCATE_SUBSYSTEM
#endif
