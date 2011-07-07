#include "tfm/StunEndPoint.hxx"

#include "tfm/SequenceSet.hxx"
#include "tfm/ActionBase.hxx"

#include "tfm/StunEvent.hxx"

#include "tfm/Expect.hxx"
#include "tfm/CommonAction.hxx"

#include "rutil/Data.hxx"
#include "rutil/Logger.hxx"

#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace resip;
using namespace std;

StunEndPoint::StunEndPoint(const Data& primaryIp,
                           const Data& secondaryIp,
                           int primaryPort,
                           int secondaryPort)
   : mStunServer(this, primaryIp, secondaryIp, primaryPort, secondaryPort)
{
   mAddr.host() = primaryIp;
   mAddr.port() = primaryPort;
   mResponseAddr.host() = primaryIp;
   mResponseAddr.port() = 8000;
}

StunEndPoint::~StunEndPoint()
{
   unregisterFromTransportDriver();
}

int 
StunEndPoint::init()
{
   int rc = mStunServer.init();
   registerWithTransportDriver();

   return rc;
}

void
StunEndPoint::clean()
{
   mRequests.clear();
   mResponseAddr.port() = 8000;
}

void
StunEndPoint::close()
{
   mStunServer.close();
}

Data
StunEndPoint::getName() const
{
   return "StunEndPoint";
}

void
StunEndPoint::buildFdSet(FdSet& fdset)
{
   mStunServer.buildFdSet(fdset); 
}

void 
StunEndPoint::process(FdSet& fdset)
{
   mStunServer.process(fdset);
}

void
StunEndPoint::generateBindingResponseDelegate()
{
   mStunServer.sendStunResponse(mRequests.front());
   mRequests.pop_front();
}

void
StunEndPoint::generateBindingResponseDelegate(const Uri& mappedAddr)
{
   mStunServer.sendStunResponse(mRequests.front(), mappedAddr);
   mRequests.pop_front();
}

void
StunEndPoint::generateSymmetricBindingResponseDelegate()
{
   mStunServer.sendStunResponse(mRequests.front(), mResponseAddr);
   mRequests.pop_front();
   mResponseAddr.port()++;
}

void
StunEndPoint::generateAllocateResponseDelegate()
{
   mStunServer.sendTurnAllocateResponse(mRequests.front());
   mRequests.pop_front();
}

void
StunEndPoint::generateAllocateErrorResponse300Delegate(const Data& ip, int port)
{
   mStunServer.sendTurnAllocateErrorResponse300(mRequests.front(), ip, port);
   mRequests.pop_front();
}

void
StunEndPoint::generateAllocateErrorResponseDelegate(int code)
{
   mStunServer.sendTurnAllocateErrorResponse(mRequests.front(), code);
   mRequests.pop_front();
}

void
StunEndPoint::generateSendResponseDelegate()
{
   mStunServer.sendTurnSendResponse(mRequests.front());
   mRequests.pop_front();
}

void
StunEndPoint::generateSetActiveDestinationResponseDelegate()
{
   mStunServer.sendTurnSetActiveDestinationResponse(mRequests.front());
   mRequests.pop_front();
}

ActionBase*
StunEndPoint::generateBindingResponse()
{
   return new CommonAction(this, 
                           "Generate Binding Response",
                           boost::bind(&StunEndPoint::generateBindingResponseDelegate, this));
}

ActionBase*
StunEndPoint::generateBindingResponse(const Uri& mappedAddress)
{
   return new CommonAction(this, 
                           "Generate Binding Response",
                           boost::bind(&StunEndPoint::generateBindingResponseDelegate, this, mappedAddress));
}

ActionBase*
StunEndPoint::generateSymmetricBindingResponse()
{
   return new CommonAction(this, 
                           "Generate Symmetric Binding Response",
                           boost::bind(&StunEndPoint::generateSymmetricBindingResponseDelegate, this));
}

ActionBase*
StunEndPoint::generateAllocateResponse()
{
   DebugLog(<< "generating TURN Allocate Response");
   return new CommonAction(this,
                           "Generate Allocate Response",
                           boost::bind(&StunEndPoint::generateAllocateResponseDelegate, this));
}

ActionBase*
StunEndPoint::generateAllocateErrorResponse300(const Data& ip, int port)
{
   return new CommonAction(this,
                           "Generate Allocate Error Response",
                           boost::bind(&StunEndPoint::generateAllocateErrorResponse300Delegate, this, ip, port));
}

ActionBase*
StunEndPoint::generateAllocateErrorResponse(int code)
{
   return new CommonAction(this,
                           "Generate Allocate Error Response",
                           boost::bind(&StunEndPoint::generateAllocateErrorResponseDelegate, this, code));
}

ActionBase*
StunEndPoint::generateSendResponse()
{
   return new CommonAction(this,
                           "Generate Send Response",
                           boost::bind(&StunEndPoint::generateSendResponseDelegate, this));
}

ActionBase*
StunEndPoint::generateSetActiveDestinationResponse()
{
   return new CommonAction(this,
                           "Generate Set Active Destination Response",
                           boost::bind(&StunEndPoint::generateSetActiveDestinationResponseDelegate, this));
}

TestEndPoint::ExpectBase*
StunEndPoint::expect(StunEvent::Type type, int timeoutMs, ActionBase* expectAction)
{
   return new Expect(*this,
                     new EventMatcherSpecific<StunEvent>(type),
                     timeoutMs,
                     expectAction); 
}

void 
StunEndPoint::onBindingRequest(boost::shared_ptr<StunRequestContext> request)
{
   DebugLog(<< "StunEndPoint::onBindingRequestReceived()");
   mRequests.push_back(request);
   handleEvent(new StunEvent(this, Stun_BindingRequest));
}

void
StunEndPoint::onUnknownRequest(boost::shared_ptr<StunRequestContext> request)
{
   DebugLog(<< "StunEndPoint::onUnknownRequestReceived()");
}

void
StunEndPoint::onParseMessageFailed()
{
   DebugLog(<< "StunEndPoint::onParseMessageFailed()");
}

void
StunEndPoint::onReceiveMessageFailed()
{
   DebugLog(<< "StunEndPoint::onReceiveMessageFailed()");
}

void
StunEndPoint::onResponseError()
{
   DebugLog(<< "StunEndPoint::onResponseErro()");
}

void
StunEndPoint::onAllocateRequest(boost::shared_ptr<StunRequestContext> request)
{
   DebugLog(<< "StunEndPoint::onAllocateRequestReceived()");
   //mRequest = request;
   mRequests.push_back(request);
   handleEvent(new StunEvent(this, Turn_AllocateRequest));
}

void
StunEndPoint::onSendRequest(boost::shared_ptr<StunRequestContext> request)
{
   DebugLog(<< "StunEndPoint::onSendRequest()");
//   mRequests.push_back(request);
//   handleEvent(new StunEvent(this, Turn_SendRequest));
}

void
StunEndPoint::onSetActiveDestinationRequest(boost::shared_ptr<StunRequestContext> request)
{
   DebugLog(<< "StunEndPoint::onSetActiveDestinationRequest()");
   mRequests.push_back(request);
   handleEvent(new StunEvent(this, Turn_SetActiveDestinationRequest));
}
