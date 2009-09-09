#ifndef RRDecorator_Include_Guard
#define RRDecorator_Include_Guard

#include "resip/stack/MessageDecorator.hxx"

namespace repro
{
class Proxy;

class RRDecorator : public resip::MessageDecorator
{
   public:
     explicit RRDecorator(const Proxy& proxy);
     RRDecorator(const RRDecorator& proxy);
     virtual ~RRDecorator();

/////////////////// Must implement unless abstract ///

      virtual void decorateMessage(resip::SipMessage &msg, 
                                    const resip::Tuple &source,
                                    const resip::Tuple &destination,
                                    const resip::Data& sigcompId) ;
      virtual void rollbackMessage(resip::SipMessage& msg) ;
      virtual MessageDecorator* clone() const ;

   private:
      const Proxy& mProxy;
      bool mAddedRecordRoute;

      //disabled
      RRDecorator();
}; // class RRDecorator

} // namespace resip

#endif // include guard
