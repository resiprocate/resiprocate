#ifndef GENERAL_CONGESTION_MANAGER_HXX
#define GENERAL_CONGESTION_MANAGER_HXX

#include "rutil/CongestionManager.hxx"
#include "rutil/Mutex.hxx"

#include <vector>

namespace resip
{
/**
   This is a general-purpose congestion manager. It is not conscious of the 
   roles of the fifos it manages, or the relationships between these 
   fifos/roles. In other words, if a crucial fifo is fully congested, this will 
   have no bearing on whether the other fifos in the system are considered 
   congested. In many cases, this is a workable restriction, but it may be 
   desirable to implement a subclass of CongestionManager that takes the system 
   as a whole into account when determining whether a specific fifo should be 
   considered congested. For more on implementing such a congestion manager, see
   the class documentation for CongestionManager.

   @ingroup message_passing
*/
class GeneralCongestionManager : public CongestionManager
{
   public:
      /**
         Enumeration of congestion metric types this class supports.
      */
      typedef enum
      {
         SIZE=0, //!< The current size of the fifo
         TIME_DEPTH, //!< The age of the oldest item in the fifo
         WAIT_TIME //!< The expected service time (the time it takes for new items to be serviced) This is the recommended metric.
      } MetricType;

      /**
         @param defaultMetric The type of metric that will be used to define 
            a fifo's congestion state, by default:
            - SIZE : Based solely on the number of messages in the fifo
            - TIME_DEPTH : Based on the age of the oldest (front-most) message 
               in the fifo.
            - WAIT_TIME : Based on the expected wait time for the fifo; this is 
               calculated by multiplying the size by the average service time. 
               This is the recommended metric.

         @param maxTolerance The default maximum tolerance for the given metric; 
            this determines when the RejectionBehavior changes
            - 0-80 percent of max tolerance -> NORMAL
            - 80-100 percent of max tolerance -> REJECTING_NEW_WORK
            - >100 percent of max tolerance -> REJECTING_NON_ESSENTIAL
      */
      GeneralCongestionManager(MetricType defaultMetric,
                                 UInt32 defaultMaxTolerance);
      virtual ~GeneralCongestionManager();

      /**
         Update the metric type and tolerances of a given fifo that the 
            GeneralCongestionManager is already aware of.
         @param fifoDescription The description of the fifo that we are 
            modifying the tolerances of.  Specify as empty to adjust all
            registered fifos.
         @param metric The type of metric that will be used to define this 
            fifo's congestion state.
            - SIZE : Based solely on the number of messages in the fifo
            - TIME_DEPTH : Based on the age of the oldest (front-most) message 
               in the fifo.
            - WAIT_TIME : Based on the expected wait time for the fifo; this is 
               calculated by multiplying the size by the average service time. 
               This is the recommended metric.

         @param maxTolerance The maximum tolerance for the given metric; this 
            determines when the RejectionBehavior changes
            - 0-80 percent of max tolerance -> NORMAL
            - 80-100 percent of max tolerance -> REJECTING_NEW_WORK
            - >100 percent of max tolerance -> REJECTING_NON_ESSENTIAL
         @return True iff the tolerances were successfully adjusted (this can 
            fail if this GeneralCongestionManager does not know about a fifo by 
            this name; make sure you have either called getCongestionPercent() 
            or registerFifo() for this fifo first).
         @note Setting the CongestionManager for the resiprocate stack will 
            cause getCongestionPercent() to be called for every fifo in the 
            stack; you may then adjust the tolerances as you wish.
      */
      virtual bool updateFifoTolerances(const resip::Data& fifoDescription,
                                          MetricType metric,
                                          UInt32 maxTolerance );

      /**
         Add a fifo to the collection of fifos monitored by this 
         CongestionManager, with a specified metric and maximum tolerance.
         @param fifo The fifo that we are registering.
         @param metric The type of metric that will be used to define this 
            fifo's congestion state.
            - SIZE : Based solely on the number of messages in the fifo
            - TIME_DEPTH : Based on the age of the oldest (front-most) message 
               in the fifo.
            - WAIT_TIME : Based on the expected wait time for the fifo; this is 
               calculated by multiplying the size by the average service time. 
               This is the recommended metric.

         @param maxTolerance The maximum tolerance for the given metric; this 
            determines when the RejectionBehavior changes
            - 0-80 percent of max tolerance -> NORMAL
            - 80-100 percent of max tolerance -> REJECTING_NEW_WORK
            - >100 percent of max tolerance -> REJECTING_NON_ESSENTIAL
      */
      virtual void registerFifo(resip::FifoStatsInterface* fifo,
                                 MetricType metric,
                                 UInt32 maxTolerance );

      /**
         Add a fifo to the collection of fifos monitored by this 
         CongestionManager, with the default metric and maximum tolerance.
         @param fifo The fifo that we are registering.
      */
      virtual void registerFifo(resip::FifoStatsInterface* fifo)
      {
         registerFifo(fifo, mDefaultMetric, mDefaultMaxTolerance);
      }

      /**
         Remove a fifo from the collection of fifos monitored by this 
         CongestionManager
         @param fifo - fifo to remove
      */
      virtual void unregisterFifo(resip::FifoStatsInterface* fifo);

      /**
         Gets the current RejectionBehavior for this fifo, based only on the
         congestion-state of this fifo (ie; this ignores the congestion-state
         of other fifos in the system).
         
         For how this function determines congestion-state, see registerFifo().       */
      virtual RejectionBehavior getRejectionBehavior(const FifoStatsInterface *fifo) const;

      virtual void logCurrentState() const;
      virtual EncodeStream& encodeCurrentState(EncodeStream& strm) const;

   private:
      /**
         @brief Returns the percent of maximum tolerances that this queue is at.
      */
      virtual UInt16 getCongestionPercent(const FifoStatsInterface* fifo) const;
      virtual RejectionBehavior getRejectionBehaviorInternal(const FifoStatsInterface *fifo) const;

      virtual EncodeStream& encodeFifoStats(const FifoStatsInterface& fifoStats, EncodeStream& strm) const;

      typedef struct
      {
         FifoStatsInterface* fifo;
         volatile MetricType metric;
         volatile UInt32 maxTolerance;
      } FifoInfo; // !bwc! TODO pick a better name

      std::vector<FifoInfo> mFifos;
      // !slg! would love to get rid of the following mutex - but we need to protect  
      //       threads querying the congestion stats and make sure runtime transport 
      //       additions are safe (ie: registerFifo and unregisterFifo being called 
      //       when the fifos are in full motion.
      mutable Mutex mFifosMutex;
      UInt16 mRejectionThresholds[REJECTING_NON_ESSENTIAL+1];
      MetricType mDefaultMetric;
      UInt32 mDefaultMaxTolerance;

      // disabled
      GeneralCongestionManager();
      GeneralCongestionManager(const GeneralCongestionManager& orig);
      GeneralCongestionManager& operator=(const GeneralCongestionManager& rhs);
};
}

#endif

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
