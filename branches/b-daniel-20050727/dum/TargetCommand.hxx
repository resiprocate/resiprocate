2#if !defined(RESIP_TARGETCOMMAND_HXX)
#define RESIP_TARGETCOMMAND_HXX

#include "resiprocate/dum/DumCommand.hxx"

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
            
            virtual void post()(std::auto_ptr<Message>)=0;

         protected:
            DialogUsageManager& mDum;
      };

      TargetCommand(Target& target, std::auto_ptr<Message> message);
      void execute();
      
   private:
      Target& mTarget;
      std::auto_ptr<Message> mMessage;
};

}

#endif
