#ifndef RESIP_DumFeature_HXX
#define RESIP_DumFeature_HXX 

namespace resip
{

class DialogUsageManager;
class Message;

class DumFeature
{
   public:      
      DumFeature(DialogUsageManager& dum);
      virtual ~DumFeature() {}
      
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
      
      virtual ProcessingResult process(Message* msg) = 0;      
   protected:
      DialogUsageManager& mDum;      
};
 
}

#endif
