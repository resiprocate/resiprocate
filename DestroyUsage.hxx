#if !defined(RESIP_DESTORYUSAGE_HXX)
#define RESIP_DESTORYUSAGE_HXX 

#include <iosfwd>
#include "resiprocate/ApplicationMessage.hxx"
#include "resiprocate/dum/Handles.hxx"

namespace resip
{

class Dialog;
class DialogSet;

class DestroyUsage : public ApplicationMessage
{
   public:
      DestroyUsage(BaseUsageHandle target);
      DestroyUsage(Dialog* dialog);
      DestroyUsage(DialogSet* dialogSet);
      
      ~DestroyUsage();

      Message* clone() const;
      void destroy();
      
      virtual Data brief() const;
      virtual std::ostream& encode(std::ostream& strm) const;
      
   private:
      DestroyUsage(const DestroyUsage& other);
      
      BaseUsageHandle mHandle;
      DialogSet* mDialogSet;
      Dialog* mDialog;
};

}

#endif
