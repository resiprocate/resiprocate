#if !defined(RESIP_TARGETCOMMAND_HXX)
#define RESIP_TARGETCOMMAND_HXX

#include "resip/dum/DumCommand.hxx"
#include "rutil/SharedPtr.hxx"

namespace resip
{

class DialogUsageManager;

class TargetCommand : public DumCommand
{
   public:
      class Target
      {
         public:
            Target(DialogUsageManager& dum) : mDum(dum) 
            {
            }
            virtual ~Target()=0;
            virtual void post(SharedPtr<Message>)=0;

         protected:
            DialogUsageManager& mDum;
      };

      TargetCommand(Target& target, SharedPtr<Message> message);
      TargetCommand(const TargetCommand&);
      void executeCommand();


      Message* clone() const;
      std::ostream& encode(std::ostream& strm) const;
      std::ostream& encodeBrief(std::ostream& strm) const;
      
   private:
      Target& mTarget;
      mutable SharedPtr<Message> mMessage;
};

}

#endif
