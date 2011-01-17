
#include "TurnEndPoint.hxx"

#include "tfm/SequenceSet.hxx"
#include "tfm/ActionBase.hxx"

#include "tfm/TurnEvent.hxx"

#include "tfm/Expect.hxx"
#include "tfm/CommonAction.hxx"

#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

using namespace resip;
using namespace turn;

TurnEndPoint::TurnEndPoint(std::auto_ptr<turn::Stack> stack, 
                           int port,
                           const resip::GenericIPAddress& turnServer)
   : mInitialized(false),
     mStack(stack),
     mPort(port),
     mTurnServer(turnServer),
     mCode(0)
{
   registerWithTransportDriver();
}

TurnEndPoint::~TurnEndPoint()
{
   unregisterFromTransportDriver();
}

void
TurnEndPoint::init()
{
   if (mInitialized) return;

   mInitialized = true;
   mBinding = boost::dynamic_pointer_cast<TurnEndPoint::TurnBinding>(mStack->registerBinding(
                                                                        std::auto_ptr<turn::Binding>(new TurnEndPoint::TurnBinding(
                                                                                                        *this, *mStack,  mPort, mTurnServer))));
}

void TurnEndPoint::buildFdSet(FdSet& fdset)
{
   mStack->buildFdSet(fdset);
}

void TurnEndPoint::process(FdSet& fdset)
{
   mStack->process(fdset);
}

ActionBase* TurnEndPoint::bind()
{
   return new CommonAction(this, 
                           "Create binding",
                           boost::bind(&TurnEndPoint::bindDelegate, this));
}

ActionBase* TurnEndPoint::destroy()
{
   return new CommonAction(this,
                           "Tear down binding",
                           boost::bind(&TurnEndPoint::destroyDelegate, this));
}

ActionBase*
TurnEndPoint::sendTo(const resip::GenericIPAddress& ip, const resip::Data& data)
{
   return new CommonAction(this,
                           "Send data",
                           boost::bind(&TurnEndPoint::sendToDelegate, this, ip, data));
}

void
TurnEndPoint::bindDelegate()
{
   mBinding->create();
}

void
TurnEndPoint::destroyDelegate()
{
   mBinding->destroy();
}

void
TurnEndPoint::sendToDelegate(const resip::GenericIPAddress& ip, const Data& data)
{
   mBinding->sendTo(ip, data);
}

TestEndPoint::ExpectBase*
TurnEndPoint::expect(TurnEvent::Type type, int timeoutMs, ActionBase* expectAction)
{
   return new Expect(*this,
                     new EventMatcherSpecific<TurnEvent>(type),
                     timeoutMs,
                     expectAction); 
}

TestEndPoint::ExpectBase*
TurnEndPoint::expect(TurnEvent::Type type, ExpectPredicate* pred, int timeoutMs, ActionBase* expectAction)
{
   return new Expect(*this,
                     new EventMatcherSpecific<TurnEvent>(type),
                     pred,
                     timeoutMs,
                     expectAction);
}

TurnEndPoint::TurnBinding::TurnBinding(TurnEndPoint& endPoint, turn::Stack& stack, int port, const resip::GenericIPAddress& server)
   : Binding(stack, UDP, port, server),
     mEndPoint(endPoint)
{
}

void TurnEndPoint::TurnBinding::onCreated()
{
   DebugLog(<< "TurnEndPoint::TurnBinding::onCreated");
   mEndPoint.handleEvent(new TurnEvent(&mEndPoint, Turn_AllocateResponse));
}

void
TurnEndPoint::TurnBinding:: onCreationFailure(int code)\
{
   DebugLog(<< "TurnEndPoint::TurnBinding::onCreationFailure");
   mEndPoint.mCode = code;
   mEndPoint.handleEvent(new TurnEvent(&mEndPoint, Turn_AllocateErrorResponse));
}

void
TurnEndPoint::TurnBinding:: onActiveDestinationReady()
{
   DebugLog(<< "TurnEndPoint::TurnBinding::onActiveDestinationReady");
   mEndPoint.handleEvent(new TurnEvent(&mEndPoint, Turn_SetActiveDestinationResponse));
}

void
TurnEndPoint::TurnBinding:: onActiveDestinationFailure(int code)
{
   DebugLog(<< "TurnEndPoint::TurnBinding::onActiveDestinationFailure");
   mEndPoint.mCode = code;
   mEndPoint.handleEvent(new TurnEvent(&mEndPoint, Turn_SetActiveDestinationErrorResponse));
}

void
TurnEndPoint::TurnBinding:: onReceivedData(const resip::Data& data, const resip::GenericIPAddress& source)
{
}

void
TurnEndPoint::TurnBinding:: onBindingFailure(int code)
{
   DebugLog(<< "TurnEndPoint::TurnBinding::onBindingFailure");
   mEndPoint.mCode = code;
   mEndPoint.handleEvent(new TurnEvent(&mEndPoint, Turn_BindingFailure));
}

void
TurnEndPoint::TurnBinding:: onAlternateServer(const resip::GenericIPAddress& server)
{
   DebugLog(<< "TurnEndPoint::TurnBinding::onAlternateServer");
   mEndPoint.handleEvent(new TurnEvent(&mEndPoint, Turn_AlternateServer));
}

TurnEndPoint::CodeMatch*
codeMatch(TurnEndPoint& endPoint, int code)
{
   return new TurnEndPoint::CodeMatch(endPoint, code);
}
