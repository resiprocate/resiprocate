#ifndef GENERAL_CONGESTION_MANAGER_HXX
#define GENERAL_CONGESTION_MANAGER_HXX

#include "rutil/CongestionManager.hxx"

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
      GeneralCongestionManager();
      virtual ~GeneralCongestionManager();

      /**
         Add a fifo to the collection of fifos monitored by this 
         CongestionManager
         @param fifo The fifo to add
         @param metric The type of metric that will be used to define this 
            fifo's congestion state.
            - SIZE : Based solely on the number of messages in the fifo
            - TIME_DEPTH : Based on the age of the oldest (front-most) message 
               in the fifo.
            - WAIT_TIME : Based on the expected wait time for the fifo; this is 
               calculated by multiplying the size by the average service time.

         @param maxTolerance The maximum tolerance for the given metric; this 
            determines when the RejectionBehavior changes
            - 0-80 percent of max tolerance -> NORMAL
            - 80-100 percent of max tolerance -> REJECTING_NEW_WORK
            - >100 percent of max tolerance -> REJECTING_NON_ESSENTIAL
      */
      virtual void registerFifo(resip::FifoStatsInterface* fifo,
                              MetricType metric,
                              UInt16 maxTolerance ); 

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
   private:
      /**
         @brief Returns the percent of maximum tolerances that this queue is at.
      */
      virtual UInt16 getCongestionPercent(const FifoStatsInterface* fifo) const;

      typedef struct
      {
         FifoStatsInterface* fifo;
         MetricType metric;
         UInt32 maxTolerance;
      } FifoInfo; // !bwc! TODO pick a better name

      std::vector<FifoInfo> mFifos;
      UInt16 mRejectionThresholds[REJECTING_NON_ESSENTIAL+1];
};
}

#endif
