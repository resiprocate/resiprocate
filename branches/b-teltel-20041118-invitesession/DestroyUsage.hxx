#if !defined(RESIP_DESTORYUSAGE_HXX)
#define RESIP_DESTORYUSAGE_HXX 

#include <iosfwd>
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class DestroyUsage : public ApplicationMessage
{
   public:
      DestroyUsage(BaseUsageHandle target);
      ~DestroyUsage();

      Message* clone() const;
      void destroy();
      
      virtual Data brief() const;
      virtual std::ostream& encode(std::ostream& strm) const;
      
   private:
      BaseUsageHandle mHandle;
};

}

#endif
