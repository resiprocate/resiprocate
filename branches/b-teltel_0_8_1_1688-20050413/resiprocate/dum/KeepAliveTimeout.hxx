#if !defined(RESIP_KEEPALIVE_TIMEOUT_HXX)
#define RESIP_KEEPALIVE_TIMEOUT_HXX 


#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/os/Tuple.hxx"

namespace resip
{

class KeepAliveTimeout : public ApplicationMessage
{
   public:
      
      KeepAliveTimeout(const Tuple& target);
      KeepAliveTimeout(const KeepAliveTimeout&);      
      ~KeepAliveTimeout();
      
      const Tuple& target() const { return mTarget; }
      Message* clone() const;
      Data brief() const;
      
      virtual std::ostream& encode(std::ostream& strm) const;
   private:
      Tuple mTarget;
};

}

#endif
