#ifndef RRDecorator_Include_Guard
#define RRDecorator_Include_Guard

#include "resip/stack/MessageDecorator.hxx"
#include "resip/stack/Tuple.hxx"


namespace resip
{
class Transport;
}

namespace repro
{
class Proxy;

class RRDecorator : public resip::MessageDecorator
{
   public:
      RRDecorator(const Proxy& proxy,
                  const resip::Transport* receivedTransport,
                  bool alreadySingleRecordRouted,
                  bool hasInboundFlowToken,
                  bool forceRecordRouteEnabled,
                  bool doPath=false,
                  bool isOriginalSenderBehindNAT=false);
      virtual ~RRDecorator();

/////////////////// Must implement unless abstract ///

      virtual void decorateMessage(resip::SipMessage &msg, 
                                    const resip::Tuple &source,
                                    const resip::Tuple &destination,
                                    const resip::Data& sigcompId) ;
      virtual void rollbackMessage(resip::SipMessage& msg) ;
      virtual MessageDecorator* clone() const ;

   private:
      void singleRecordRoute(resip::SipMessage &msg, 
                                    const resip::Tuple &source,
                                    const resip::Tuple &destination,
                                    const resip::Data& sigcompId);
      void doubleRecordRoute(resip::SipMessage &msg, 
                                    const resip::Tuple &source,
                                    const resip::Tuple &destination,
                                    const resip::Data& sigcompId);
      bool isTransportSwitch(const resip::Tuple& sendingFrom);
      bool outboundFlowTokenNeeded(resip::SipMessage &msg, 
                                    const resip::Tuple &source,
                                    const resip::Tuple &destination,
                                    const resip::Data& sigcompId);
      const Proxy& mProxy;
      int mAddedRecordRoute;
      bool mAlreadySingleRecordRouted;
      bool mHasInboundFlowToken;
      bool mForceRecordRouteEnabled;
      bool mDoPath;
      const bool mIsOriginalSenderBehindNAT;
      const resip::Transport* mReceivedTransport;

      //disabled
      RRDecorator();
}; // class RRDecorator

} // namespace resip

#endif // include guard
