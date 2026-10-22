#include "resip/dum/TargetCommand.hxx"
#include "resip/dum/DumFeature.hxx"
#include "resip/dum/DumFeatureChain.hxx"
#include "resip/stack/Message.hxx"
#include "rutil/WinLeakCheck.hxx"

#include <utility>

using namespace resip;
using namespace std;

class GuardFeature : public DumFeature
{
   public:
      GuardFeature(DialogUsageManager& dum, TargetCommand::Target& target)
         : DumFeature(dum, target)
      {}

      virtual ProcessingResult process(Message* msg)
      {
         return DumFeature::FeatureDone;
      }
};

DumFeatureChain::DumFeatureChain(DialogUsageManager& dum,
                                 FeatureList features,
                                 TargetCommand::Target& target)
   : mFeatures(std::move(features))
{
   mFeatures.emplace_back(std::make_shared<GuardFeature>(dum, target));
   for (FeatureList::size_type i = 0; i < mFeatures.size(); ++i)
   {
      mActiveFeatures.push_back(true);
   }
}

DumFeatureChain::ProcessingResult DumFeatureChain::process(Message* msg)
{
   FeatureList::iterator feat = mFeatures.begin();
   vector<bool>::iterator active = mActiveFeatures.begin();
   bool stop = false;

   DumFeature::ProcessingResult pres = DumFeature::FeatureDone;
   do
   {
      if (*active)
      {
         pres = (*feat)->process(msg);

         switch(pres)
         {
            case DumFeature::EventTaken:
               stop = true;
               break;
            case DumFeature::FeatureDone:
               *active = false;
               break;
            case DumFeature::FeatureDoneAndEventDone:
            case DumFeature::FeatureDoneAndEventTaken: //??
            case DumFeature::ChainDoneAndEventTaken:
            case DumFeature::ChainDoneAndEventDone:
               *active = false;
               stop = true;
               break;
         }

         if (static_cast<unsigned int>(pres) & DumFeature::EventDoneBit)
         {
            delete msg;
            unsigned int bits = pres;
            bits ^= DumFeature::EventDoneBit;
            bits |= DumFeature::EventTaken;
            pres = static_cast<DumFeature::ProcessingResult>(bits);
         }
      }

      active++;
      feat++;
   }
   while(!stop && feat != mFeatures.end());


   unsigned int chainBits = 0;
   if (static_cast<unsigned int>(pres) & DumFeature::ChainDoneBit || feat == mFeatures.end())
   {
      chainBits |= ChainDoneBit;
   }

   if (static_cast<unsigned int>(pres) & DumFeature::EventTakenBit)
   {
      chainBits |= EventTakenBit;
   }

   return static_cast<DumFeatureChain::ProcessingResult>(chainBits);
}
