#if !defined CommonAction_hxx
#define CommonAction_hxx

#include "rutil/Data.hxx"
#include <boost/function.hpp>
#include "tfm/ActionBase.hxx"

class TestEndPoint;

class CommonAction : public ActionBase
{
   public:
      typedef boost::function<void (void) > Functor;

      explicit CommonAction(TestEndPoint* tua, resip::Data action, Functor f) : 
         mTestEndPoint(tua),
         mFunctor(f),
         mActionName(action)
         {}
      virtual ~CommonAction() {}

      virtual void operator()(boost::shared_ptr<Event> event) { (*this)(); }
      virtual void operator()() { mFunctor(); }

      virtual resip::Data toString() const { return mActionName; }

   protected:
      TestEndPoint* mTestEndPoint;
      Functor mFunctor;
      resip:: Data mActionName;
};
#endif
