#ifndef RESIP_StatisticsManager_hxx
#define RESIP_StatisticsManager_hxx

#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/StatisticsMessage.hxx"

// !dlb! part of the build script
#define RESIP_STATISTICS(_x) _x
//#define RESIP_STATISTICS(_x)

namespace resip
{

class SipMessage;
class TransactionController;

class StatisticsManager : public StatisticsMessage::Payload
{
   public:
      // not implemented
      typedef enum
      {
         TransportFifoSize,
         TUFifoSize,
         ActiveTimers,
         OpenTcpConnections,
         ActiveNonInviteTransactions,
         ActiveInviteTransactions,
         PendingDnsQueries,
         StatsMemUsed
      } Measurement;
      
      StatisticsManager(TransactionController& transactionController,
                        unsigned long intervalSecs=60);

      void process();

   private:
      friend class TransactionState;
      bool sent(SipMessage* msg, bool retrans);
      bool received(SipMessage* msg);

      void poll(); // force an update
      void setInterval(unsigned long intvSecs); // needs to be called through the fifo somehow

      TransactionController& mTransactionController;
      UInt64 mInterval;
      UInt64 mNextPoll;
};

}

#endif

