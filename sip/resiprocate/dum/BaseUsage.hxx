#if !defined(RESIP_BASEUSAGE_HXX)
#define RESIP_BASEUSAGE_HXX

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/MethodTypes.hxx"

namespace resip
{

class DialogUsageManager;
class Dialog;
class DumTimeout;
class SipMessage;
class NameAddr;

class BaseUsage
{
   public:
      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg,
                      const Data& file,
                      int line)
               : BaseException(msg, file, line)
            {}
            
            virtual const char* name() const {return "BaseUsage::Exception";}
      };

      class Handle
      {
         public:
            typedef unsigned long Id; // make this a UInt64, fix the hash
         protected:
            Handle(DialogUsageManager& dum);

            bool isValid() const;
            // throws if not found
            BaseUsage* get();
            BaseUsage* operator->();
            Id mId;
         private:
            friend class BaseUsage;            
            DialogUsageManager* mDum;
            friend class DialogUsageManager;
            friend class Dialog;
            static Id getNext();
      };

      // to send a request on an existing dialog (made from make... methods above)
      virtual void send(const SipMessage& request);
      
      DialogUsageManager& dum();
      Dialog& dialog();
      
      virtual void dispatch(const SipMessage& msg) = 0;
      virtual void dispatch(const DumTimeout& timer) = 0;
      BaseUsage::Handle getBaseHandle() { return mHandle; }
   protected:
      friend class DialogUsageManager;
      BaseUsage(DialogUsageManager& dum, Dialog& dialog);
      virtual ~BaseUsage();

      DialogUsageManager& mDum;
      Dialog& mDialog;
      BaseUsage::Handle mHandle;
};
 
}

#endif
