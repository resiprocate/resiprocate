#include "resiprocate/NameAddr.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/os/Logger.hxx"
#include "tfm/RouteGuard.hxx"
#include "tfm/TestProxy.hxx"

#define RESIPROCATE_SUBSYSTEM Cathay::Subsystem::TEST

using resip::Uri;
using resip::Data;
using resip::NameAddr;

RouteGuard::RouteGuard(TestProxy& proxy, 
                       const resip::Data& matchingPattern,
                       const resip::Data& rewriteExpression, 
                       const resip::Data& method,
                       const resip::Data& event,
                       int priority,
                       int weight)
   : mProxy(proxy),
     mMatchingPattern(matchingPattern),
     mMethod(method),
     mEvent(event)
{
   cleanup();
   mProxy.addRoute(matchingPattern, rewriteExpression, method, event, priority, weight);
}


void RouteGuard::cleanup()
{
   mProxy.deleteRoute(mMatchingPattern, mMethod, mEvent);
}

RouteGuard::~RouteGuard()
{
   cleanup();
}
