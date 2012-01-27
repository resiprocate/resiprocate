#include "rutil/GeneralCongestionManager.hxx"

#include "rutil/AbstractFifo.hxx"
#include "rutil/EsLogger.hxx"

namespace resip
{
GeneralCongestionManager::GeneralCongestionManager()
{
   // !bwc! TODO allow these to be configured.
   mRejectionThresholds[NORMAL]=0;
   mRejectionThresholds[REJECTING_NEW_WORK]=80;
   mRejectionThresholds[REJECTING_NON_ESSENTIAL]=100;
}

GeneralCongestionManager::~GeneralCongestionManager()
{}

void
GeneralCongestionManager::registerFifo(resip::FifoStatsInterface* fifo,
                                       MetricType metric,
                                       UInt16 maxTolerance)
{
   FifoInfo info;
   info.fifo=fifo;
   info.metric=metric;
   info.maxTolerance=maxTolerance;
   mFifos.push_back(info);
   fifo->setRole(mFifos.size()-1);
   fifo->setCongestionManager(this);
}

void 
GeneralCongestionManager::unregisterFifo(resip::FifoStatsInterface* fifo)
{
   if(fifo->getRole() < mFifos.size())
   {
      mFifos[fifo->getRole()].fifo=0;
   }
}

CongestionManager::RejectionBehavior 
GeneralCongestionManager::getRejectionBehavior(const FifoStatsInterface *fifo) const
{
   UInt16 percent=getCongestionPercent(fifo);

   // .bwc. If we're keeling over, we exit this sooner.
   if(percent > mRejectionThresholds[REJECTING_NON_ESSENTIAL])
   {
      return REJECTING_NON_ESSENTIAL;
   }
   else if(percent > mRejectionThresholds[REJECTING_NEW_WORK])
   {
      return REJECTING_NEW_WORK;
   }
   else
   {
      return NORMAL;
   }
}

void 
GeneralCongestionManager::logCurrentState() const
{
   ES_INFO(estacado::SBF_QUEUEING,"FIFO STATISTICS");
   for(std::vector<FifoInfo>::const_iterator i=mFifos.begin();
         i!=mFifos.end();++i)
   {
      if(i->fifo)
      {
         const FifoStatsInterface& fifo=*(i->fifo);
         ES_INFO(estacado::SBF_QUEUEING,fifo.getDescription() <<" - "
                     " SIZE: " << fifo.size()
                     << " TIME DEPTH: " << fifo.timeDepth()  
                     << " EXP WAIT: " << fifo.expectedWaitTimeMilliSec()
                     << " AVG SERVICE TIME (micro-sec): "
                     << fifo.averageServiceTimeMicroSec());
      }
   }
}

UInt16
GeneralCongestionManager::getCongestionPercent(const FifoStatsInterface* fifo) const
{
   if(fifo->getRole() >= mFifos.size())
   {
      assert(0);
      return 0;
   }
   
   const FifoInfo& info = mFifos[fifo->getRole()];
   assert(info.fifo==fifo);
   switch(info.metric)
   {
      case SIZE:
         return resipDiv(100*fifo->size(),info.maxTolerance);
      case TIME_DEPTH:
         return resipDiv(100*(UInt32)(fifo->timeDepth()),info.maxTolerance);
      case WAIT_TIME:
         return resipDiv(100*(UInt32)(fifo->expectedWaitTimeMilliSec()),info.maxTolerance);
      default:
         assert(0);
         return 0;
   }
   return 0;
}

}
