#include <cstdlib>
#include <iostream>

#include "resip/stack/SendData.hxx"
#include "resip/stack/Transport.hxx"
#include "rutil/Fifo.hxx"

using namespace std;
using namespace resip;

static int failures = 0;

static void
check(int actual, int expected, const char* what)
{
   const bool ok = (actual == expected);
   cerr << (ok ? "ok   " : "FAIL ") << what << " (got " << actual << ", wanted " << expected << ")" << endl;
   if (!ok)
   {
      ++failures;
   }
}

// CS5 and CS3, with the bytes they occupy once the ECN bits are left clear.
static const int CS5 = 40;
static const int CS5_BYTE = 160;
static const int CS3_BYTE = 96;

/**
   The transport hierarchy is abstract, and the class the ToS state machine
   yields does not depend on any socket. Follow testConnectionBase.cxx and stand
   up the smallest concrete transport the base class will accept.
*/
class FakeTosTransport : public Transport
{
   public:
      FakeTosTransport(Fifo<TransactionMessage>& rxFifo)
         : Transport(rxFifo, 0, V4, Data::Empty, Data::Empty),
           mListener(::socket(AF_INET, SOCK_DGRAM, 0))
      {}

      ~FakeTosTransport() { closeSocket(mListener); }

      virtual TransportType transport() const { return UDP; }
      virtual bool isReliable() const { return false; }
      virtual bool isDatagram() const { return true; }
      virtual void process() {}
      virtual void process(FdSet&) {}
      virtual void buildFdSet(FdSet&) {}
      virtual void invokeAfterSocketCreationFunc() const
      {
         ++mTraversals;

         // Stand in for the thread reading the configuration, which is free to
         // land a new value while the owning thread is walking the sockets.
         if (mClearDuringTraversal)
         {
            mClearDuringTraversal = false;
            const_cast<FakeTosTransport*>(this)->setDscp(-1);
         }
      }
      virtual void send(std::unique_ptr<SendData>) {}
      virtual void shutdown() {}
      virtual bool shutdownComplete() const { return true; }
      virtual bool isFinished() const { return true; }
      virtual void setPollGrp(FdPollGrp*) {}
      virtual bool shareStackProcessAndSelect() const { return false; }
      virtual void startOwnProcessing() {}
      virtual bool hasDataToSend() const { return false; }
      virtual unsigned int getFifoSize() const { return 0; }

      // The drain is called by the transport's own processing cycle, not by its
      // users; reach it here to drive the request-then-drain sequence.
      using Transport::drainDscpRefresh;

      int traversals() const { return mTraversals; }
      int listenerReads() const { return mListenerReads; }

      // Asks for setDscp(-1) from inside the next traversal.
      void clearDuringNextTraversal() { mClearDuringTraversal = true; }

   protected:
      virtual Socket getListenerSocket() const
      {
         ++mListenerReads;
         return mListener;
      }

   private:
      Socket mListener;
      mutable int mTraversals { 0 };
      mutable int mListenerReads { 0 };
      mutable bool mClearDuringTraversal { false };
};

static int
readTos(Socket fd)
{
   int tos = -1;
   socklen_t len = sizeof(tos);
   ::getsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&tos, &len);
   return tos;
}

// Marking a socket is what records that a mark was made, so a test that wants
// that state has to go through here rather than set the flag.
static void
markOneSocket(Transport& transport)
{
   Socket fd = ::socket(AF_INET, SOCK_DGRAM, 0);
   transport.applySocketOptions(fd);
   closeSocket(fd);
}

int
main()
{
   // Windows opens no socket before WSAStartup, and nothing here has run the
   // stack that would call it.
   initNetwork();

   Fifo<TransactionMessage> fifo;

   {
      FakeTosTransport t(fifo);
      check(t.effectiveDscp(), -1, "a transport nobody configured writes nothing");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(-1);
      check(t.effectiveDscp(), -1, "configured off, never applied: writes nothing");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      check(t.effectiveDscp(), CS5, "configured on: writes the configured class");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(0);
      check(t.effectiveDscp(), 0, "zero is a value, not an absence");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      markOneSocket(t);
      t.setDscp(-1);
      check(t.effectiveDscp(), 0, "cleared after applying: writes 0 back");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      markOneSocket(t);
      t.setDscp(-1);
      t.drainDscpRefresh();
      check(t.effectiveDscp(), -1, "already cleared: writes nothing a second time");
   }

   // A class outside the range is refused where it enters, so it never reaches
   // a socket and cannot log once per accepted connection.
   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      check(t.setDscp(64) ? 1 : 0, 0, "a class above 63 is refused");
      check(t.effectiveDscp(), CS5, "and the transport keeps the class it had");

      t.drainDscpRefresh();
      check(t.traversals(), 1, "a refused class walks nothing of its own");
   }

   {
      FakeTosTransport t(fifo);
      check(t.setDscp(-2) ? 1 : 0, 0, "a class below -1 is refused");
      check(t.effectiveDscp(), -1, "and leaves the transport unmarked");
   }

   // The per-socket entry point every transport routes through.
   {
      // Seeded, not fresh: a fresh socket already reads 0, so an implementation
      // that wrote 0 here would pass while breaking the no-op guarantee.
      FakeTosTransport t(fifo);
      Socket fd = ::socket(AF_INET, SOCK_DGRAM, 0);
      int preset = CS3_BYTE;
      ::setsockopt(fd, IPPROTO_IP, IP_TOS, (const char *)&preset, sizeof(preset));
      t.applySocketOptions(fd);
      check(readTos(fd), preset, "no class set: the socket is left exactly as it was");
      closeSocket(fd);
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      Socket fd = ::socket(AF_INET, SOCK_DGRAM, 0);
      t.applySocketOptions(fd);
      check(readTos(fd), CS5_BYTE, "a class set: the socket carries the byte it shifts to");
      closeSocket(fd);
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      markOneSocket(t);
      t.setDscp(-1);
      Socket fd = ::socket(AF_INET, SOCK_DGRAM, 0);
      int marked = CS5_BYTE;
      ::setsockopt(fd, IPPROTO_IP, IP_TOS, (const char *)&marked, sizeof(marked));
      t.applySocketOptions(fd);
      check(readTos(fd), 0, "cleared transport: the socket is written back to 0");
      closeSocket(fd);
   }

   // The request is raised on the thread that changes the value and served on
   // the transport's own, so the drain does the work and only when it was asked.
   {
      FakeTosTransport t(fifo);
      t.drainDscpRefresh();
      check(t.traversals(), 0, "nothing set: the drain walks nothing");
      check(t.listenerReads(), 0, "and reports nothing");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      t.drainDscpRefresh();
      check(t.traversals(), 1, "a new class makes the drain walk the sockets once");
      check(t.listenerReads(), 1, "and report what the listener carries");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      t.drainDscpRefresh();
      t.drainDscpRefresh();
      check(t.traversals(), 1, "one request is served once, not on every cycle");
      check(t.listenerReads(), 1, "and reported once, not on every cycle");
   }

   // A reload hands every transport the class from the configuration, whether
   // or not the file changed it. The walk is skipped; the read-back line is
   // owed on every reload, so it is raised either way.
   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      t.drainDscpRefresh();
      t.setDscp(CS5);
      t.drainDscpRefresh();
      check(t.traversals(), 1, "the same class set again walks nothing");
      check(t.listenerReads(), 2, "but still reports, which the reload has to log");
   }

   // A service that never asked for a marking must see the library behave
   // exactly as it does today: the user's socket callback is not re-invoked
   // behind its back, and nothing is logged. This holds whether it left the
   // value alone or set it to -1 explicitly.
   {
      FakeTosTransport t(fifo);
      t.drainDscpRefresh();
      check(t.traversals(), 0, "a transport never given a class is not walked");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(-1);
      t.drainDscpRefresh();
      check(t.traversals(), 0, "a transport set to -1 is not walked either");
      check(t.listenerReads(), 0, "and is not read back either");
   }

   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      t.drainDscpRefresh();
      t.setDscp(-1);
      t.drainDscpRefresh();
      check(t.traversals(), 2, "clearing a class that was written still walks");
      check(t.listenerReads(), 2, "and the undo pass reports the reset");
   }

   // A value that changes while the traversal runs must be recorded as what the
   // pass wrote, not as what arrived during it: the sockets it already visited
   // carry the old class, and only the request the change raised reaches them.
   {
      FakeTosTransport t(fifo);
      t.setDscp(CS5);
      t.clearDuringNextTraversal();
      t.drainDscpRefresh();
      check(t.effectiveDscp(), 0, "cleared mid-traversal: there is still a mark to undo");

      t.drainDscpRefresh();
      check(t.traversals(), 2, "the change that landed mid-traversal is served next");
      check(t.effectiveDscp(), -1, "and the undo pass leaves nothing behind");
   }

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
}
