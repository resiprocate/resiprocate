#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/StatisticsManager.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/SipStack.hxx"

using namespace resip;
using std::vector;

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSACTION

StatisticsManager::StatisticsManager(SipStack& stack, unsigned long intervalSecs) 
   : StatisticsMessage::Payload(),
     mStack(stack),
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
   tuFifoSize = mStack.mTransactionController.getTuFifoSize();
   transportFifoSizeSum = mStack.mTransactionController.sumTransportFifoSizes();
   transactionFifoSize = mStack.mTransactionController.getTransactionFifoSize();
   activeTimers = mStack.mTransactionController.getTimerQueueSize();
   activeClientTransactions = mStack.mTransactionController.getNumClientTransactions();
   activeServerTransactions = mStack.mTransactionController.getNumServerTransactions();   

   static StatisticsMessage::AtomicPayload appStats;
   appStats.loadIn(*this);

   // let the app do what it wants with it
   mStack.post(StatisticsMessage(appStats));
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
