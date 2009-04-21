#include "repro/RRDecorator.hxx"

#include "repro/Proxy.hxx"

#include "resip/stack/Helper.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Tuple.hxx"

#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

namespace repro
{
RRDecorator::RRDecorator(const Proxy& proxy) :
   mProxy(proxy),
   mAddedRecordRoute(false)
{}

RRDecorator::RRDecorator(const RRDecorator& orig) :
   mProxy(orig.mProxy),
   mAddedRecordRoute(false)
{}

RRDecorator::~RRDecorator()
{}

void 
RRDecorator::decorateMessage(resip::SipMessage& request, 
                               const resip::Tuple &source,
                               const resip::Tuple &destination) 
{
   DebugLog(<<"Proxy::decorateMessage called.");
   resip::NameAddr rt;
   
   if(destination.onlyUseExistingConnection 
      || resip::InteropHelper::getRRTokenHackEnabled())
   {
      rt=mProxy.getRecordRoute();
      // .bwc. If our target has an outbound flow to us, we need to put a flow
      // token in a Record-Route.
      resip::Helper::massageRoute(request,rt);
      resip::Data binaryFlowToken;
      resip::Tuple::writeBinaryToken(destination,binaryFlowToken);
      
      // !bwc! TODO encrypt this binary token to self.
      rt.uri().user()=binaryFlowToken.base64encode();
   }
   else if(!request.empty(resip::h_RecordRoutes) 
            && mProxy.isMyUri(request.header(resip::h_RecordRoutes).front().uri())
            && !request.header(resip::h_RecordRoutes).front().uri().user().empty())
   {
      // .bwc. If we Record-Routed earlier with a flow-token, we need to
      // add a second Record-Route (to make in-dialog stuff work both ways)
      rt = mProxy.getRecordRoute();
      resip::Helper::massageRoute(request,rt);
   }
   
   // This pushes the Record-Route that represents the interface from
   // which the request is being sent
   //
   // .bwc. This shouldn't duplicate the previous Record-Route, since rt only
   // gets defined if we need to double-record-route. (ie, the source or the
   // target had an outbound flow to us). The only way these could end up the
   // same is if the target and source were the same entity.
   if (!rt.uri().host().empty())
   {
      request.header(resip::h_RecordRoutes).push_front(rt);
      mAddedRecordRoute=true;
      InfoLog (<< "Added outbound Record-Route: " << rt);
   }
}

void 
RRDecorator::rollbackMessage(resip::SipMessage& request) 
{
   if(mAddedRecordRoute)
   {
      assert(!request.header(resip::h_RecordRoutes).empty());
      request.header(resip::h_RecordRoutes).pop_front();
      mAddedRecordRoute=false;
   }
}

resip::MessageDecorator* 
RRDecorator::clone() const 
{
   return new RRDecorator(*this);
}

} // of namespace repro
