#include <vector>

#include "resiprocate/dum/DumFeature.hxx"
#include "resiprocate/dum/DumFeatureChain.hxx"
#include "resiprocate/Message.hxx"

using namespace resip;
using namespace std;

DumFeatureChain::DumFeatureChain(const FeatureList& features)
  :mFeatures(features)
{
  for (FeatureList::size_type i = 0; i < mFeatures.size(); ++i)
  {
    mActiveFeatures.push_back(true);
  }
}

DumFeatureChain::ProcessingResult  DumFeatureChain::process(Message* msg)
{
   FeatureList::iterator feat = mFeatures.begin();
   bit_vector::iterator active = mActiveFeatures.begin();

   //bool eventDone = !mFeatures.empty();
   //bool featureDone = false;   
   bool stop = false;

   DumFeature::ProcessingResult pres = DumFeature::FeatureDoneAndEventDone;
   do
   {
      if (*active)
      {
	 //featureDone = false;         
         DumFeature::ProcessingResult pres = (*feat)->process(msg);

         if (pres & DumFeature::EventDoneBit)
         {
            delete msg;
         }

         switch(pres)
         {
	 case DumFeature::EventTaken:
	   stop = true;
	   break;
	 case DumFeature::FeatureDone:
	   *active = false;
	   break;
	 case DumFeature::FeatureDoneAndEventDone:
	 case DumFeature::FeatureDoneAndEventTaken:
	 case DumFeature::ChainDoneAndEventTaken:
	 case DumFeature::ChainDoneAndEventDone:
	   *active = false;
	   stop = true;
	   break;
         }
      }
      //else
      //{
      //featureDone = true;
      //}
      active++;
      feat++;      
   }
   while(!stop && feat != mFeatures.end() );

   DumFeatureChain::ProcessingResult res = DumFeatureChain::ChainDone;

   if (pres & DumFeature::EventTakenBit)
   {
     if (pres & DumFeature::FeatureDoneBit || 
	 pres & DumFeature::ChainDoneBit)
     {
       res = DumFeatureChain::ChainDoneAndEventTaken;
     }
     else
     {
       res = DumFeatureChain::EventTaken;
     }
   }

   return res;
}
