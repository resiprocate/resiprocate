#if !defined(RESIP_TARGETCOMMAND_HXX)
#define RESIP_TARGETCOMMAND_HXX

#include <memory>
#include "resip/dum/DumCommand.hxx"

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
            virtual void post(std::auto_ptr<Message>)=0;

         protected:
            DialogUsageManager& mDum;
      };

      TargetCommand(Target& target, std::auto_ptr<Message> message);
      TargetCommand(const TargetCommand&);
      void executeCommand();


      Message* clone() const;
      EncodeStream& encode(EncodeStream& strm) const;
      EncodeStream& encodeBrief(EncodeStream& strm) const;
      
   private:
      Target& mTarget;
      mutable std::auto_ptr<Message> mMessage;
};

}

#endif
