#ifndef RESIP_DumFeature_HXX
#define RESIP_DumFeature_HXX 

#include <memory>
#include "resip/dum/TargetCommand.hxx"

namespace resip
{

class DialogUsageManager;
class Message;

class DumFeature
{
   public:      
      DumFeature(DialogUsageManager& dum, TargetCommand::Target& target);
      virtual ~DumFeature();
      
      enum ProcessingResultMask
      {
         EventDoneBit = 1 << 0,
         EventTakenBit = 1 << 1,
         FeatureDoneBit = 1 << 2,
         ChainDoneBit = 1 << 3
      };                              

      //legal combinations
      enum ProcessingResult
      {
         EventTaken = EventTakenBit,
         FeatureDone = FeatureDoneBit,
         FeatureDoneAndEventDone = FeatureDoneBit | EventDoneBit,
         FeatureDoneAndEventTaken = FeatureDoneBit | EventTakenBit,
         ChainDoneAndEventDone = ChainDoneBit | EventDoneBit,
         ChainDoneAndEventTaken = ChainDoneBit | EventTakenBit
      };
      
      // !bwc! We absolutely, positively, MUST NOT throw here. This is because 
      // in DialogUsageManager::process(), we do not know if a DumFeature has 
      // taken ownership of msg until we get a return. If we throw, the 
      // ownership of msg is unknown. This is unacceptable.
      virtual ProcessingResult process(Message* msg) = 0;
      virtual void postCommand(std::auto_ptr<Message> message);

   protected:
      DialogUsageManager& mDum;
      TargetCommand::Target& mTarget;
};
 
}

#endif
