#include "rutil/GeneralCongestionManager.hxx"

#include "rutil/AbstractFifo.hxx"
#include "rutil/Lock.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::STATS

namespace resip
{
GeneralCongestionManager::GeneralCongestionManager(MetricType defaultMetric,
                                                   UInt32 defaultMaxTolerance) :
   mDefaultMetric(defaultMetric),
   mDefaultMaxTolerance(defaultMaxTolerance)
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
                                       UInt32 maxTolerance)
{
   Lock lock(mFifosMutex);
   FifoInfo info;
   info.fifo=fifo;
   info.metric=metric;
   info.maxTolerance=maxTolerance;
   mFifos.push_back(info);
   fifo->setRole((UInt8)mFifos.size()-1);
}

void 
GeneralCongestionManager::unregisterFifo(resip::FifoStatsInterface* fifo)
{
   Lock lock(mFifosMutex);
   if(fifo->getRole() < mFifos.size())
   {
      mFifos[fifo->getRole()].fifo=0;
   }
}

bool 
GeneralCongestionManager::updateFifoTolerances(
                                          const resip::Data& fifoDescription,
                                          MetricType metric,
                                          UInt32 maxTolerance )
{
   Lock lock(mFifosMutex);
   for(std::vector<FifoInfo>::iterator i=mFifos.begin(); i!=mFifos.end(); ++i)
   {
      if(i->fifo && // ensure fifo isn't 0'd out from unregister call
         (fifoDescription.empty() || isEqualNoCase(i->fifo->getDescription(), fifoDescription)))
      {
         i->maxTolerance=UINT_MAX;  // Set temporarily to UINT_MAX, so that we don't inadvertantly reject a request while the metric and tolerance are being changed.
         i->metric=metric;
         i->maxTolerance=maxTolerance;
         if(!fifoDescription.empty()) return true;
      }
   }
   return false || fifoDescription.empty();
}

CongestionManager::RejectionBehavior 
GeneralCongestionManager::getRejectionBehavior(const FifoStatsInterface *fifo) const
{
   Lock lock(mFifosMutex);
   return getRejectionBehaviorInternal(fifo);
}

CongestionManager::RejectionBehavior 
GeneralCongestionManager::getRejectionBehaviorInternal(const FifoStatsInterface *fifo) const
{
   // !bwc! We need to also keep an eye on memory usage, and push back if it 
   // looks like we're going to start hitting swap sometime soon.

   UInt16 percent=getCongestionPercent(fifo);

   // .bwc. We exit this sooner the more congested we are.
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
   Lock lock(mFifosMutex);
   WarningLog(<<"FIFO STATISTICS");
   for(std::vector<FifoInfo>::const_iterator i=mFifos.begin();
         i!=mFifos.end();++i)
   {
      if(i->fifo)
      {
         const FifoStatsInterface& fifo=*(i->fifo);
         Data buffer;
         DataStream strm(buffer);
         encodeFifoStats(fifo, strm);
         WarningLog(<< buffer);
      }
   }
}

EncodeStream& 
GeneralCongestionManager::encodeCurrentState(EncodeStream& strm) const
{
   Lock lock(mFifosMutex);
   for(std::vector<FifoInfo>::const_iterator i=mFifos.begin();
         i!=mFifos.end();++i)
   {
      if(i->fifo)
      {
         const FifoStatsInterface& fifo=*(i->fifo);
         encodeFifoStats(fifo, strm);
         strm << std::endl;
      }
   }
   strm.flush();
   return strm;
}

UInt16
GeneralCongestionManager::getCongestionPercent(const FifoStatsInterface* fifo) const
{
   if(fifo->getRole() >= mFifos.size())
   {
      resip_assert(0);
      return 0;
   }

   const FifoInfo& info = mFifos[fifo->getRole()];
   resip_assert(info.fifo==fifo);
   switch(info.metric)
   {
      case SIZE:
         return resipIntDiv(100*(UInt16)fifo->getCountDepth(),info.maxTolerance);
      case TIME_DEPTH:
         return resipIntDiv(100*(UInt32)(fifo->getTimeDepth()),info.maxTolerance);
      case WAIT_TIME:
         return resipIntDiv(100*(UInt32)(fifo->expectedWaitTimeMilliSec()),info.maxTolerance);
      default:
         resip_assert(0);
         return 0;
   }
   return 0;
}

EncodeStream&
GeneralCongestionManager::encodeFifoStats(const FifoStatsInterface& fifoStats, EncodeStream& strm) const
{
   CongestionManager::RejectionBehavior behavior = getRejectionBehaviorInternal(&fifoStats);
   const FifoInfo& info = mFifos[fifoStats.getRole()];
   strm <<fifoStats.getDescription()
      << ": Size=" << fifoStats.getCountDepth()
      << " TimeDepth(sec)=" << fifoStats.getTimeDepth()  
      << " ExpWait(msec)=" << fifoStats.expectedWaitTimeMilliSec()
      << " AvgSvcTime(usec)="
      << fifoStats.averageServiceTimeMicroSec()
      << " Metric=" 
      << (info.metric == WAIT_TIME ? "WAIT_TIME" :
          info.metric == TIME_DEPTH ? "TIME_DEPTH" :
                                      "SIZE")
      << " MaxTolerance="
      << info.maxTolerance
      << " CurBehavior="
      << (behavior == NORMAL ? "NORMAL" : 
          behavior == REJECTING_NEW_WORK ? "REJECTING_NEW_WORK" : 
                                           "REJECTING_NON_ESSENTIAL");
   strm.flush();
   return strm;
}

}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
