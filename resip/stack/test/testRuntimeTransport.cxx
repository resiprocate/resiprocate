#include <atomic>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

#include "rutil/Logger.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/GeneralCongestionManager.hxx"
#include "resip/stack/EventStackThread.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/UdpTransport.hxx"

using namespace resip;

struct Observation
{
   std::mutex mutex;
   std::condition_variable changed;
   std::thread::id registrationThread;
   std::thread::id processingThread;
   std::thread::id destructionThread;
   bool processed = false;
   bool destroyed = false;
   unsigned destructions = 0;
   unsigned received = 0;
   bool deliverUp = false;   // pass received messages on to the TransactionController
   CongestionManager* congestionManager = nullptr;
   unsigned congestionManagerSets = 0;
};

class ObservedTransport : public UdpTransport
{
public:
   ObservedTransport(Fifo<TransactionMessage>& fifo, Observation& observation)
      : UdpTransport(fifo, 0, V4, StunDisabled, Data("127.0.0.1")),
        mObservation(observation)
   {
   }

   ~ObservedTransport() override
   {
      std::lock_guard<std::mutex> lock(mObservation.mutex);
      mObservation.destructionThread = std::this_thread::get_id();
      mObservation.destroyed = true;
      ++mObservation.destructions;
      mObservation.changed.notify_all();
   }

   void setPollGrp(FdPollGrp* group) override
   {
      UdpTransport::setPollGrp(group);
      if(group)
      {
         std::lock_guard<std::mutex> lock(mObservation.mutex);
         mObservation.registrationThread = std::this_thread::get_id();
      }
   }

   void process() override
   {
      UdpTransport::process();
      std::lock_guard<std::mutex> lock(mObservation.mutex);
      mObservation.processingThread = std::this_thread::get_id();
      mObservation.processed = true;
      mObservation.changed.notify_all();
   }

   void pushRxMsgUp(SipMessage* message) override
   {
      // Parsing and basicCheck() have already run on the selector thread.
      // Unless asked otherwise, consume the message here to keep this test
      // independent of transactions.
      if(mObservation.deliverUp)
      {
         UdpTransport::pushRxMsgUp(message);
      }
      else
      {
         delete message;
      }
      std::lock_guard<std::mutex> lock(mObservation.mutex);
      ++mObservation.received;
      mObservation.changed.notify_all();
   }

   void setCongestionManager(CongestionManager* manager) override
   {
      UdpTransport::setCongestionManager(manager);
      std::lock_guard<std::mutex> lock(mObservation.mutex);
      mObservation.congestionManager = manager;
      ++mObservation.congestionManagerSets;
   }

private:
   Observation& mObservation;
};

static int failures = 0;

// Generous, so that slow or sanitizer builds do not fail on timing
static const std::chrono::seconds Wait(10);

static void check(bool ok, const char* message)
{
   std::cout << (ok ? "PASS " : "FAIL ") << message << std::endl;
   failures += !ok;
}

static void checkSingleThread(bool alreadyProcessing)
{
   Observation observation;
   SipStack stack;
   if(alreadyProcessing)
   {
      stack.process(0);
   }
   std::unique_ptr<ObservedTransport> transport(new ObservedTransport(stack.stateMacFifo(), observation));
   ObservedTransport* raw = transport.get();
   stack.addTransport(std::move(transport));
   const unsigned key = raw->getKey();
   check(observation.registrationThread == std::thread::id(),
         alreadyProcessing ? "running single-thread stack defers registration" :
                             "single-thread stack defers registration before processing starts");
   for(int i = 0; i < 20 && !observation.processed; ++i)
   {
      stack.process(0);
   }
   check(observation.processed, "single-thread processing registers the transport");
   stack.removeTransport(key);
   for(int i = 0; i < 20 && !observation.destroyed; ++i)
   {
      stack.process(0);
   }
   check(observation.destroyed, "single-thread queued removal completes");
}

static void checkThreadedTransport(SipStack& stack, Observation& observation,
                                   const char* registrationMessage)
{
   std::unique_ptr<ObservedTransport> transport(new ObservedTransport(stack.stateMacFifo(), observation));
   ObservedTransport* raw = transport.get();
   stack.addTransport(std::move(transport));
   const unsigned key = raw->getKey();
   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const bool completed = observation.changed.wait_for(lock, Wait,
         [&] { return observation.processed; });
      check(completed && observation.registrationThread == observation.processingThread &&
            observation.processingThread != std::this_thread::get_id(),
            registrationMessage);
   }
   stack.removeTransport(key);
   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const bool completed = observation.changed.wait_for(lock, Wait,
         [&] { return observation.destroyed; });
      check(completed && observation.destructionThread == observation.processingThread,
            "threaded stack: removal completes on its processing thread");
   }
}

static void checkInternalThreads(bool alreadyProcessing)
{
   Observation observation;
   SipStack stack;
   stack.run();
   if(alreadyProcessing)
   {
      stack.getTimeTillNextProcessMS();
   }
   checkThreadedTransport(stack, observation,
      alreadyProcessing ? "run(): registration uses selector thread after timer query" :
                          "run(): registration uses selector thread before timer query");
   stack.shutdownAndJoinThreads();
}

// Transports added before run() are registered by the selector thread too.
static void checkAddedBeforeRun()
{
   Observation observation;
   SipStack stack;
   stack.addTransport(std::unique_ptr<ObservedTransport>(new ObservedTransport(stack.stateMacFifo(), observation)));
   stack.run();
   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const bool completed = observation.changed.wait_for(lock, Wait,
         [&] { return observation.processed; });
      check(completed && observation.registrationThread == observation.processingThread &&
            observation.processingThread != std::this_thread::get_id(),
            "run(): a transport added before run() is registered on the selector thread");
   }
   stack.shutdownAndJoinThreads();
}

static void checkEventThread()
{
   Observation observation;
   EventStackSimpleMgr manager(nullptr);
   SipStackOptions options;
   SipStack& stack = manager.createStack(options);
   // Add right after starting the thread, before its first timer query
   manager.getThread().run();
   checkThreadedTransport(stack, observation,
      "EventStackThread: registration uses the event thread");
   manager.getThread().shutdown();
   manager.getThread().join();
}

static void checkQueuedChanges(bool remove, bool process)
{
   Observation observation;
   Fifo<TransactionMessage> fifo;
   DnsStub dns;
   Compression compression(Compression::NONE);
   std::unique_ptr<FdPollGrp> poll(FdPollGrp::create());
   {
      TransportSelector selector(fifo, nullptr, dns, compression, false);
      selector.setPollGrp(poll.get());
      std::unique_ptr<ObservedTransport> transport(new ObservedTransport(fifo, observation));
      transport->setKey(1);
      selector.addTransport(std::move(transport), true);
      if(remove)
      {
         // Both requests are pending before the selector can consume either.
         // Looking at shutdown state would mistake the first request for REMOVE.
         selector.removeTransport(1);
      }
      if(process)
      {
         selector.process();
         check(observation.destructions == 1 && !observation.processed,
               "queued ADD then REMOVE destroys once before transport processing");
      }
   }
   check(observation.destructions == 1,
         remove ? "pending removal has exactly one owner during selector destruction" :
                  "pending addition has exactly one owner during selector destruction");
}

static void checkImmediateRemoval()
{
   Observation observation;
   SipStack stack;
   stack.run();
   std::unique_ptr<ObservedTransport> transport(new ObservedTransport(stack.stateMacFifo(), observation));
   ObservedTransport* raw = transport.get();
   stack.addTransport(std::move(transport));
   stack.removeTransport(raw->getKey());
   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const bool completed = observation.changed.wait_for(lock, Wait,
         [&] { return observation.destroyed; });
      check(completed && observation.destructions == 1 &&
            observation.destructionThread == observation.registrationThread &&
            observation.destructionThread != std::this_thread::get_id(),
            "run(): immediate add/remove completes once on the selector thread");
   }
   stack.shutdownAndJoinThreads();
}

static void checkRemovalWhileReceiving()
{
   const Socket sender = ::socket(AF_INET, SOCK_DGRAM, 0);
   if(sender == INVALID_SOCKET)
   {
      check(false, "run(): create UDP sender for removal under traffic");
      return;
   }
   if(!makeSocketNonBlocking(sender))
   {
      closeSocket(sender);
      check(false, "run(): make UDP sender nonblocking");
      return;
   }

   Observation observation;
   SipStack stack;
   stack.run();
   std::unique_ptr<ObservedTransport> transport(new ObservedTransport(stack.stateMacFifo(), observation));
   const Tuple destination = transport->getTuple();
   ObservedTransport* raw = transport.get();
   stack.addTransport(std::move(transport));
   const unsigned key = raw->getKey();

   // ACK still passes basicCheck() during shutdown, and exercises its read
   // of mShuttingDown without generating a 503 response for each datagram.
   const char packet[] =
      "ACK sip:callee@127.0.0.1 SIP/2.0\r\n"
      "Via: SIP/2.0/UDP 127.0.0.1:9;branch=z9hG4bK-runtime\r\n"
      "To: <sip:callee@127.0.0.1>;tag=callee\r\n"
      "From: <sip:caller@127.0.0.1>;tag=caller\r\n"
      "Call-ID: runtime-transport\r\n"
      "CSeq: 1 ACK\r\n"
      "Max-Forwards: 70\r\n"
      "Content-Length: 0\r\n\r\n";
   std::atomic<bool> stop(false);
   std::thread traffic([&]
   {
      while(!stop.load())
      {
         for(int i = 0; i < 8; ++i)
         {
            ::sendto(sender, packet, sizeof(packet) - 1, 0,
                     &destination.getSockaddr(), destination.length());
         }
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
   });

   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const bool received = observation.changed.wait_for(lock, Wait,
         [&] { return observation.received >= 8; });
      check(received, "run(): UDP traffic reaches basicCheck() before removal");
   }
   // Keep sending until the selector has actually destroyed the transport.
   stack.removeTransport(key);
   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const bool removed = observation.changed.wait_for(lock, Wait,
         [&] { return observation.destroyed; });
      check(removed && observation.destructions == 1 &&
            observation.destructionThread == observation.registrationThread,
            "run(): removal during UDP traffic destroys once on the selector thread");
   }
   stop = true;
   traffic.join();
   closeSocket(sender);
   stack.shutdownAndJoinThreads();
}

// A transport queued for addition is not in the TransportSelector yet, so it
// must get the CongestionManager in effect when it is added, not the one in
// effect when addTransport() was called.
static void checkCongestionManagerBinding(bool clearWhileQueued)
{
   Observation observation;
   GeneralCongestionManager manager(GeneralCongestionManager::WAIT_TIME, 200);
   {
      SipStack stack;
      if(clearWhileQueued)
      {
         stack.setCongestionManager(&manager);
      }
      stack.addTransport(std::unique_ptr<ObservedTransport>(new ObservedTransport(stack.stateMacFifo(), observation)));
      stack.setCongestionManager(clearWhileQueued ? nullptr : &manager);
      for(int i = 0; i < 20 && !observation.processed; ++i)
      {
         stack.process(0);
      }
      // Single-threaded: no lock, since setCongestionManager() below takes it
      check(observation.processed && observation.congestionManager == (clearWhileQueued ? nullptr : &manager),
            clearWhileQueued ? "a manager cleared while the addition is queued is not bound" :
                               "a manager set while the addition is queued is bound");
      if(clearWhileQueued)
      {
         check(observation.congestionManagerSets == 0,
               "a queued transport never holds the cleared manager");
      }
      stack.setCongestionManager(nullptr);
   }
}

// run() after processing has registered a transport, while it receives:
// the selector may start delivering to the TransactionController fifo while
// run() sets up the TransactionController thread.
static void checkRunWhileReceiving()
{
   const Socket sender = ::socket(AF_INET, SOCK_DGRAM, 0);
   if(sender == INVALID_SOCKET)
   {
      check(false, "run(): create UDP sender for start-up under traffic");
      return;
   }
   if(!makeSocketNonBlocking(sender))
   {
      closeSocket(sender);
      check(false, "run(): make UDP sender for start-up under traffic nonblocking");
      return;
   }
   Observation observation;
   observation.deliverUp = true;
   SipStack stack;
   std::unique_ptr<ObservedTransport> transport(new ObservedTransport(stack.stateMacFifo(), observation));
   const Tuple destination = transport->getTuple();
   stack.addTransport(std::move(transport));
   for(int i = 0; i < 20 && !observation.processed; ++i)
   {
      stack.process(0);
   }
   const char packet[] =
      "ACK sip:callee@127.0.0.1 SIP/2.0\r\n"
      "Via: SIP/2.0/UDP 127.0.0.1:9;branch=z9hG4bK-runtime-start\r\n"
      "To: <sip:callee@127.0.0.1>;tag=callee\r\n"
      "From: <sip:caller@127.0.0.1>;tag=caller\r\n"
      "Call-ID: runtime-transport-start\r\n"
      "CSeq: 1 ACK\r\n"
      "Max-Forwards: 70\r\n"
      "Content-Length: 0\r\n\r\n";
   std::atomic<bool> stop(false);
   std::thread traffic([&]
   {
      while(!stop.load())
      {
         ::sendto(sender, packet, sizeof(packet) - 1, 0,
                  &destination.getSockaddr(), destination.length());
         std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }
   });
   std::this_thread::sleep_for(std::chrono::milliseconds(20));
   stack.run();
   {
      std::unique_lock<std::mutex> lock(observation.mutex);
      const unsigned before = observation.received;
      const bool received = observation.changed.wait_for(lock, Wait,
         [&] { return observation.received >= before + 8; });
      check(received, "run(): traffic received before run() keeps flowing after it");
   }
   stop = true;
   traffic.join();
   closeSocket(sender);
   stack.shutdownAndJoinThreads();
}

int main()
{
   initNetwork();
   Log::initialize(Log::Cout, Log::None, Data::Empty);
   checkSingleThread(false);
   checkSingleThread(true);
   checkInternalThreads(false);
   checkInternalThreads(true);
   checkAddedBeforeRun();
   checkEventThread();
   checkQueuedChanges(true, true);
   checkQueuedChanges(false, false);
   checkQueuedChanges(true, false);
   checkImmediateRemoval();
   checkRemovalWhileReceiving();
   checkCongestionManagerBinding(false);
   checkCongestionManagerBinding(true);
   checkRunWhileReceiving();
   return failures == 0 ? 0 : 1;
}
