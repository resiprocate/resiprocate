#if !defined CommonAction_hxx
#define CommonAction_hxx

#include "rutil/Data.hxx"
#include "tfm/ActionBase.hxx"

#include <functional>
#include <utility>

class TestEndPoint;

class CommonAction : public ActionBase
{
   public:
      typedef std::function<void(void)> Functor;

      explicit CommonAction(TestEndPoint* tua, resip::Data action, Functor f) : 
         mTestEndPoint(tua),
         mFunctor(std::move(f)),
         mActionName(action)
         {}
      virtual ~CommonAction() = default;

      virtual void operator()(std::shared_ptr<Event> event) { (*this)(); }
      virtual void operator()() { mFunctor(); }

      virtual resip::Data toString() const { return mActionName; }

   protected:
      TestEndPoint* mTestEndPoint;
      Functor mFunctor;
      resip:: Data mActionName;
};
#endif
