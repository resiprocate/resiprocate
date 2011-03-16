#include "repro/RRDecorator.hxx"

#include "repro/Proxy.hxx"

#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Tuple.hxx"

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

namespace repro
{
RRDecorator::RRDecorator(const Proxy& proxy,
                         bool doPath,
                         bool isOriginalSenderBehindNAT) :
   mProxy(proxy),
   mAddedRecordRoute(false),
   mDoPath(doPath),
   mIsOriginalSenderBehindNAT(isOriginalSenderBehindNAT)
{}

RRDecorator::RRDecorator(const RRDecorator& orig) :
   mProxy(orig.mProxy),
   mAddedRecordRoute(false),
   mDoPath(orig.mDoPath),
   mIsOriginalSenderBehindNAT(orig.mIsOriginalSenderBehindNAT)
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
   resip::NameAddrs* routes=0;
   if(mDoPath)
   {
      if(!request.empty(resip::h_Paths))
      {
         routes=&(request.header(resip::h_Paths));
      }
   }
   else
   {
      if(!request.empty(resip::h_RecordRoutes))
      {
         routes=&(request.header(resip::h_RecordRoutes));
      }
   }

   // .bwc. Any of these cases means that we are assuming that whoever is
   // just downstream will remain in the call-path throughout the dialog.
   if(destination.onlyUseExistingConnection 
      || resip::InteropHelper::getRRTokenHackEnabled()
      || mIsOriginalSenderBehindNAT
      || !sigcompId.empty())
   {
      if(destination.getType()==resip::TLS || 
         destination.getType()==resip::DTLS)
      {
         rt = mProxy.getRecordRoute();
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
   else if(routes
            && mProxy.isMyUri(routes->front().uri())
            && !(routes->front().uri().user().empty()))
   {
      // .bwc. If we Record-Routed earlier with a flow-token, we need to
      // add a second Record-Route (to make in-dialog stuff work both ways)
      rt = mProxy.getRecordRoute();
      resip::Helper::massageRoute(request,rt);
   }
   
#ifdef USE_SIGCOMP
   if(mRequestContext.mProxy.compressionEnabled() && !sigcompId.empty())
   {
      rt.uri().param(p_comp)="sigcomp";
   }
#endif

   // This pushes the Record-Route that represents the interface from
   // which the request is being sent
   //
   // .bwc. This shouldn't duplicate the previous Record-Route, since rt only
   // gets defined if we need to double-record-route. (ie, the source or the
   // target had an outbound flow to us). The only way these could end up the
   // same is if the target and source were the same entity.
   if (!rt.uri().host().empty())
   {
      static resip::ExtensionParameter p_drr("drr");
      rt.uri().param(p_drr);
      if(mDoPath)
      {
         request.header(resip::h_Paths).front().uri().param(p_drr);
         request.header(resip::h_Paths).push_front(rt);
         InfoLog (<< "Added outbound Path: " << rt);
      }
      else
      {
         request.header(resip::h_RecordRoutes).front().uri().param(p_drr);
         request.header(resip::h_RecordRoutes).push_front(rt);
         InfoLog (<< "Added outbound Record-Route: " << rt);
      }
      mAddedRecordRoute=true;
   }
}

void 
RRDecorator::rollbackMessage(resip::SipMessage& request) 
{
   if(mAddedRecordRoute)
   {
      if(mDoPath)
      {
         assert(!request.header(resip::h_Paths).empty());
         request.header(resip::h_Paths).pop_front();
      }
      else
      {
         assert(!request.header(resip::h_RecordRoutes).empty());
         request.header(resip::h_RecordRoutes).pop_front();
      }
      mAddedRecordRoute=false;
   }
}

resip::MessageDecorator* 
RRDecorator::clone() const 
{
   return new RRDecorator(*this);
}

} // of namespace repro
