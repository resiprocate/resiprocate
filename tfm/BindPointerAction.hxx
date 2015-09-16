#if !defined BindPointerAction_hxx
#define BindPointerAction_hxx

#include "rutil/Data.hxx"
#include <boost/function.hpp>
#include "tfm/ActionBase.hxx"
#include <XsLib/h/CXsRefPtr.h>

class TestEndPoint;

template<class T, class E>
class BindPointerAction : public ActionBase
{
   public:
      explicit BindPointerAction(TestEndPoint* tua, resip::Data action, T& ptr) : 
         mTestEndPoint(tua),
         mActionName(action),
         mPtr(ptr)
         {}
      virtual ~BindPointerAction() {}

      virtual void operator()(boost::shared_ptr<Event> event)
      {
         boost::shared_ptr<E> e
            = boost::dynamic_pointer_cast<E, Event>(event);

         resip_assert(e.get());

         mPtr->bind(e);
      }

      virtual void operator()() { resip_assert(0); }

      virtual resip::Data toString() const { return mActionName; }

   protected:
      TestEndPoint* mTestEndPoint;
      resip:: Data mActionName;
      T& mPtr;
};
#endif
