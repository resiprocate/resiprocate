#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "repro/RRDecorator.hxx"

#include "repro/Proxy.hxx"

#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Transport.hxx"

#include "rutil/Logger.hxx"
#include "rutil/TransportType.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using resip::ExtensionParameter;
using resip::NameAddr;
using resip::UDP;
using resip::h_RecordRoutes;
using resip::h_Paths;
using resip::h_RequestLine;
using resip::h_Routes;
using resip::p_comp;

namespace repro
{
RRDecorator::RRDecorator(const Proxy& proxy,
                         const resip::Tuple& receivedTransportTuple,
                         const resip::NameAddr &receivedTransportRecordRoute,
                         bool alreadySingleRecordRouted,
                         bool hasInboundFlowToken,
                         bool forceRecordRouteEnabled,
                         bool doPath,
                         bool isOriginalSenderBehindNAT) :
   mProxy(proxy),
   mAddedRecordRoute(0),
   mAlreadySingleRecordRouted(alreadySingleRecordRouted),
   mHasInboundFlowToken(hasInboundFlowToken),
   mForceRecordRouteEnabled(forceRecordRouteEnabled),
   mDoPath(doPath),
   mIsOriginalSenderBehindNAT(isOriginalSenderBehindNAT),
   mReceivedTransportTuple(receivedTransportTuple),
   mReceivedTransportRecordRoute(receivedTransportRecordRoute)
{}

RRDecorator::~RRDecorator()
{}

void 
RRDecorator::decorateMessage(resip::SipMessage& request, 
                               const resip::Tuple &source,
                               const resip::Tuple &destination,
                               const resip::Data& sigcompId) 
{
   DebugLog(<<"Proxy::decorateMessage called.");
   resip::NameAddr rt;

   if(isTransportSwitch(source))
   {
      if(mAlreadySingleRecordRouted)
      {
         singleRecordRoute(request, source, destination, sigcompId);
      }
      else
      {
         doubleRecordRoute(request, source, destination, sigcompId);
      }
   }
   else
   {
      // We might still want to record-route in this case; if we need an 
      // outbound flow token or we've already added an inbound flow token
      if(outboundFlowTokenNeeded(request, source, destination, sigcompId) ||
         mHasInboundFlowToken)  // or we have an inbound flow
      {
         resip_assert(mAlreadySingleRecordRouted);
         singleRecordRoute(request, source, destination, sigcompId);
      }
   }

   static ExtensionParameter p_drr("drr");
   resip::NameAddrs* routes=0;
   if(mDoPath)
   {
      routes=&(request.header(resip::h_Paths));
   }
   else
   {
      routes=&(request.header(resip::h_RecordRoutes));
   }

   if(routes->size() > 1 && 
      mAddedRecordRoute && 
      routes->front().uri().exists(p_drr))
   {
      // .bwc. It is possible that we have duplicate Record-Routes at this 
      // point, if we have done a transport switch but both transports use the 
      // same FQDN.
      resip::NameAddrs::iterator second = ++(routes->begin());
      if(*second == routes->front())
      {
         // Duplicated record-routes; pare down to a single one.
         routes->pop_front();
         --mAddedRecordRoute;
         routes->front().uri().remove(p_drr);
      }
   }
}

void 
RRDecorator::singleRecordRoute(resip::SipMessage& request, 
                               const resip::Tuple &source,
                               const resip::Tuple &destination,
                               const resip::Data& sigcompId)
{
   resip::NameAddr rt;
   // .bwc. outboundFlowTokenNeeded means that we are assuming that whoever is
   // just downstream will remain in the call-path throughout the dialog.
   if(outboundFlowTokenNeeded(request, source, destination, sigcompId))
   {
      if(isSecure(destination.getType()))
      {
         rt = mProxy.getRecordRoute(destination.mTransportKey);
         rt.uri().scheme()="sips";
      }
      else
      {
         // .bwc. It is safe to put ip+port+proto here, since we have an 
         // existing flow to the next hop.
         rt.uri().host()=resip::Tuple::inet_ntop(source);
         rt.uri().port()=source.getPort();
         rt.uri().param(resip::p_transport)=resip::Tuple::toDataLower(source.getType());
      }
      // .bwc. If our target has an outbound flow to us, we need to put a flow
      // token in a Record-Route.
      resip::Helper::massageRoute(request,rt);
      resip::Data binaryFlowToken;
      resip::Tuple::writeBinaryToken(destination, binaryFlowToken, Proxy::FlowTokenSalt);
      
      rt.uri().user()=binaryFlowToken.base64encode();
   }
   else
   {
      // No need for a flow-token; just use an ordinary record-route.
      rt = mProxy.getRecordRoute(destination.mTransportKey);
      resip::Helper::massageRoute(request,rt);
   }

#ifdef USE_SIGCOMP
   if(mProxy.compressionEnabled() && !sigcompId.empty())
   {
      rt.uri().param(p_comp)="sigcomp";
   }
#endif

   static ExtensionParameter p_drr("drr");
   rt.uri().param(p_drr);

   resip::NameAddrs* routes=0;
   if(mDoPath)
   {
      routes=&(request.header(resip::h_Paths));
      InfoLog(<< "Adding outbound Path: " << rt);
   }
   else
   {
      routes=&(request.header(resip::h_RecordRoutes));
      InfoLog(<< "Adding outbound Record-Route: " << rt);
   }

   resip_assert(routes->size() > 0);
   routes->front().uri().param(p_drr);
   routes->push_front(rt);
   ++mAddedRecordRoute;
}

void 
RRDecorator::doubleRecordRoute(resip::SipMessage& request, 
                               const resip::Tuple &source,
                               const resip::Tuple &destination,
                               const resip::Data& sigcompId)
{
   // We only use this on transport switch when we have not yet Record-Routed.
   // If we needed a flow-token in the inbound Record-Route, it would have been 
   // added already.
   resip::NameAddr rt(mReceivedTransportRecordRoute);
   resip::Helper::massageRoute(request,rt);
   if(mDoPath)
   {
      request.header(h_Paths).push_front(rt);
   }
   else
   {
      request.header(h_RecordRoutes).push_front(rt);
   }
   ++mAddedRecordRoute;
   singleRecordRoute(request, source, destination, sigcompId);
}

bool 
RRDecorator::isTransportSwitch(const resip::Tuple& sendingFrom)
{
   if(mForceRecordRouteEnabled)
   {
      // If we are forcing record routes to be added, then DRR on any transport switch
      return mReceivedTransportTuple.mTransportKey != sendingFrom.mTransportKey;
   }
   else
   {
      // If record routing is not forced then only DRR if we are switching transport types or
      // protocol versions, since the interfaces themselves may all be equally reachable
      // !slg! - could make this behavior more configurable
      return sendingFrom.getType() != mReceivedTransportTuple.getType() ||
             sendingFrom.ipVersion() != mReceivedTransportTuple.ipVersion();
   }
}

bool 
RRDecorator::outboundFlowTokenNeeded(resip::SipMessage &msg, 
                                     const resip::Tuple &source,
                                     const resip::Tuple &destination,
                                     const resip::Data& sigcompId)
{
   return (destination.onlyUseExistingConnection            // destination is an outbound target
           || resip::InteropHelper::getRRTokenHackEnabled() // or the token is enabled
           || mIsOriginalSenderBehindNAT                    // or the nat detection hack is enabled
           || !sigcompId.empty());                          // or we are routing to a SigComp transport 
                                                            // ?slg? For Sigcomp are we guaranteed to always have 
                                                            // single RR at this point?  If not, then strangeness 
                                                            // will happen when singleRecordRoute adds a ;drr param
}

void 
RRDecorator::rollbackMessage(resip::SipMessage& request) 
{
   resip::NameAddrs* routes=0;
   if(mDoPath)
   {
      routes=&(request.header(resip::h_Paths));
   }
   else
   {
      routes=&(request.header(resip::h_RecordRoutes));
   }

   while(mAddedRecordRoute--)
   {
      resip_assert(!routes->empty());
      routes->pop_front();
   }

   if(mAlreadySingleRecordRouted)
   {
      // Make sure we remove the drr param if it is there.
      static ExtensionParameter p_drr("drr");
      routes->front().uri().remove(p_drr);
   }
}

resip::MessageDecorator* 
RRDecorator::clone() const 
{
   return new RRDecorator(*this);
}

} // of namespace repro
