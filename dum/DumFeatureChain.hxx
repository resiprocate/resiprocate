#ifndef RESIP_DumFeatureChain_HXX
#define RESIP_DumFeatureChain_HXX 

#include <vector>
#include "rutil/SharedPtr.hxx"

namespace resip
{

class DumFeature;

class DumFeatureChain
{
   public: 
      typedef std::vector<SharedPtr<DumFeature> > FeatureList;
      
      enum ProcessingResultMask
      {
         EventTakenBit = 1 << 0, //don't pass on, don't delete event
         ChainDoneBit = 1 << 1 //if true chain can be deleted
      };                              

      //legal combinations
      enum ProcessingResult
      {
         EventTaken = EventTakenBit, //don't delete event
         ChainDone = ChainDoneBit,  //event not consumed by chain
         ChainDoneAndEventTaken = ChainDoneBit | EventTakenBit
      };

      DumFeatureChain(DialogUsageManager& dum, const FeatureList& features, TargetCommand::Target& target);
     
      ProcessingResult process(Message* msg);      

   private:       
      // std::bit_vector mActiveFeatures;  //vector<bool> is the correct way on most platforms
      std::vector<bool> mActiveFeatures;
      FeatureList mFeatures;
};
 
}

#endif


