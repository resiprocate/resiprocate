#if !defined(DialPlanGuard_hxx)
#define DialPlanGuard_hxx

#include "rutil/Data.hxx"

class TestProxy;

class RouteGuard
{
   public:
      // service provider dialplan
      RouteGuard(TestProxy& proxy, 
                 const resip::Data& matchingPattern,
                 const resip::Data& rewriteExpression, 
                 const resip::Data& method = resip::Data::Empty,
                 const resip::Data& event = resip::Data::Empty,
                 int priority = 1,
                 int weight = 1);
      ~RouteGuard();      

      void cleanup();      
   private:
      TestProxy& mProxy;
      
      resip::Data mMatchingPattern;
      resip::Data mMethod;
      resip::Data mEvent;
};

#endif
