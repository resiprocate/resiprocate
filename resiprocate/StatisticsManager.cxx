#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/StatisticsManager.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TransactionController.hxx"

using namespace resip;
using std::vector;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatisticsManager::StatisticsManager(TransactionController& transController,
                                     unsigned long intervalSecs) 
   : StatisticsMessage::Payload(),
     mTransactionController(transController),
     mInterval(intervalSecs*1000),
     mNextPoll(Timer::getTimeMs() + mInterval)
{}

void 
StatisticsManager::setInterval(unsigned long intervalSecs)
{
   mInterval = intervalSecs * 1000;
}

void 
StatisticsManager::poll()
{
   // get snapshot data now..
   tuFifoSize = mTransactionController.getTuFifoSize();
   transportFifoSizeSum = mTransactionController.sumTransportFifoSizes();
   transactionFifoSize = mTransactionController.getTransactionFifoSize();
   activeTimers = mTransactionController.getTimerQueueSize();
   activeClientTransactions = mTransactionController.getNumClientTransactions();
   activeServerTransactions = mTransactionController.getNumServerTransactions();   

   static StatisticsMessage::AtomicPayload appStats;
   appStats.loadIn(*this);

   // let the app do what it wants with it
   mTransactionController.post(StatisticsMessage(appStats));
}

void 
StatisticsManager::process()
{
   if (Timer::getTimeMs() >= mNextPoll)
   {
      poll();
      mNextPoll += mInterval;
   }
}

bool
StatisticsManager::sent(SipMessage* msg, bool retrans)
{
   MethodTypes met = msg->header(h_CSeq).method();

   if (msg->isRequest())
   {
      if (retrans)
      {
         ++requestsRetransmitted;
         ++requestsRetransmittedByMethod[met];
      }
      ++requestsSent;
      ++requestsSentByMethod[met];
   }
   else if (msg->isResponse())
   {
      int code = msg->header(h_StatusLine).statusCode();
      if (code < 0 || code >= MaxCode)
      {
         code = 0;
      }

      if (retrans)
      {
         ++responsesRetransmitted;
         ++responsesRetransmittedByMethod[met];
         ++responsesRetransmittedByMethodByCode[met][code];
      }
      ++responsesSent;
      ++responsesSentByMethod[met];
      ++responsesSentByMethodByCode[met][code];
   }

   return false;
}

bool
StatisticsManager::received(SipMessage* msg)
{
   MethodTypes met = msg->header(h_CSeq).method();

   if (msg->isRequest())
   {
      ++requestsReceived;
      ++requestsReceivedByMethod[met];
   }
   else if (msg->isResponse())
   {
      ++responsesReceived;
      ++responsesReceivedByMethod[met];
      int code = msg->header(h_StatusLine).statusCode();
      if (code < 0 || code >= MaxCode)
      {
         code = 0;
      }
      ++responsesReceivedByMethodByCode[met][code];
   }

   return false;
}
