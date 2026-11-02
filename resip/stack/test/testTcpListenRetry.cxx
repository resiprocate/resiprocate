#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <climits>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef WIN32
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <unistd.h>
#endif

#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/TransactionMessage.hxx"
#include "rutil/FdPoll.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/Socket.hxx"

using namespace std;
using namespace resip;

// Only the epoll group honours FPEM_Edge; the poll and select groups keep
// reporting a listener with queued connections, so they cannot get stuck.
#ifdef RESIP_POLL_IMPL_EPOLL
static int failures = 0;

static void
check(bool condition, const char* what)
{
   cerr << (condition ? "ok   " : "FAIL ") << what << endl;
   if (!condition)
   {
      ++failures;
   }
}

// The transport passes every connection it accepts through this hook, so the
// count tells how many it accepted. While throwsLeft is positive, the hook
// closes the connection and throws instead, as a failing connection setup does.
static size_t configured = 0;
static int throwsLeft = 0;

static void
countSocket(Socket fd, int, const char*, int)
{
   if (throwsLeft > 0)
   {
      --throwsLeft;
      closeSocket(fd);
      throw Transport::Exception("connection setup failed", __FILE__, __LINE__);
   }
   ++configured;
}

// Exposes the listener descriptor, so a test can replace the file behind it.
class ListenerTransport : public TcpTransport
{
   public:
      ListenerTransport(Fifo<TransactionMessage>& fifo)
         : TcpTransport(fifo, 0, V4, Data("127.0.0.1"), &countSocket)
      {
      }

      Socket listener() const { return getListenerSocket(); }
};

static Socket
connectTo(int port)
{
   Socket client = ::socket(AF_INET, SOCK_STREAM, 0);
   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = htons(port);
   ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

   if (::connect(client, (sockaddr *)&addr, sizeof(addr)) != 0)
   {
      closeSocket(client);
      return INVALID_SOCKET;
   }

   return client;
}

// A loopback connect completes into the accept queue before the transport
// accepts it.
static vector<Socket>
queueConnections(int port, size_t count)
{
   vector<Socket> clients;
   for (size_t i = 0; i < count; ++i)
   {
      Socket client = connectTo(port);
      check(client != INVALID_SOCKET, "opened a client connection");
      if (client != INVALID_SOCKET)
      {
         clients.push_back(client);
      }
   }

   return clients;
}

static void
closeAll(const vector<Socket>& clients)
{
   for (size_t i = 0; i < clients.size(); ++i)
   {
      closeSocket(clients[i]);
   }
}

// The same cycle as TransportThread runs, which also catches what escapes it.
static void
pump(TcpTransport& transport, FdPollGrp& pollGrp)
{
   for (int i = 0; i < 20; ++i)
   {
      try
      {
         transport.process();
      }
      catch (BaseException&)
      {
      }
      pollGrp.waitAndProcess(10);
   }
}

static void
testDescriptorExhaustion(FdPollGrp& pollGrp)
{
   Fifo<TransactionMessage> fifo;
   // Port 0: the transport throws on a clash, and nothing here would catch it.
   TcpTransport transport(fifo, 0, V4, Data("127.0.0.1"), &countSocket);
   transport.setPollGrp(&pollGrp);
   const vector<Socket> clients = queueConnections(transport.port(), 3);

   // A soft limit equal to the lowest free descriptor makes every accept()
   // fail with EMFILE.
   rlimit saved;
   const bool haveLimit = ::getrlimit(RLIMIT_NOFILE, &saved) == 0;
   check(haveLimit, "read the descriptor limit");
   const int lowestFree = haveLimit ? ::open("/dev/null", O_RDONLY) : -1;
   check(lowestFree >= 0, "found the lowest free descriptor");
   if (lowestFree < 0)
   {
      closeAll(clients);
      transport.setPollGrp(0);
      return;
   }
   ::close(lowestFree);
   rlimit exhausted = saved;
   exhausted.rlim_cur = lowestFree;
   check(::setrlimit(RLIMIT_NOFILE, &exhausted) == 0, "lowered the descriptor limit");

   configured = 0;
   pump(transport, pollGrp);
   check(configured == 0, "no connection is accepted while descriptors are exhausted");
   check(transport.getTimeTillNextProcessMS() <= 25, "a pending retry asks to be processed soon");

   // No new connection arrives once descriptors are free again, as when the
   // accept queue is full and the kernel drops every new SYN.
   check(::setrlimit(RLIMIT_NOFILE, &saved) == 0, "restored the descriptor limit");
   pump(transport, pollGrp);
   check(configured == clients.size(), "the queued connections are accepted once descriptors are free");
   check(transport.getTimeTillNextProcessMS() == UINT_MAX, "nothing is pending once the queue is empty");

   closeAll(clients);
   transport.setPollGrp(0);
}

static void
testConnectionSetupThrows(FdPollGrp& pollGrp)
{
   Fifo<TransactionMessage> fifo;
   TcpTransport transport(fifo, 0, V4, Data("127.0.0.1"), &countSocket);
   transport.setPollGrp(&pollGrp);
   const vector<Socket> clients = queueConnections(transport.port(), 3);

   configured = 0;
   throwsLeft = 1;
   pump(transport, pollGrp);
   check(throwsLeft == 0, "setting up the first connection threw");
   check(configured == clients.size() - 1, "the connections queued behind it are still accepted");

   closeAll(clients);
   transport.setPollGrp(0);
}

static void
testListenerDescriptorReplaced(FdPollGrp& pollGrp)
{
   Fifo<TransactionMessage> fifo;
   ListenerTransport transport(fifo);
   transport.setPollGrp(&pollGrp);
   const vector<Socket> clients = queueConnections(transport.port(), 1);

   // Another component closes the listener and a new socket reuses its number.
   // The saved duplicate keeps the listener and its epoll registration alive.
   const Socket listener = transport.listener();
   const int saved = ::dup(listener);
   const Socket other = ::socket(AF_INET, SOCK_STREAM, 0);
   check(saved >= 0 && other != INVALID_SOCKET, "prepared the replacement socket");
   if (saved < 0 || other == INVALID_SOCKET)
   {
      if (saved >= 0)
      {
         ::close(saved);
      }
      if (other != INVALID_SOCKET)
      {
         closeSocket(other);
      }
      closeAll(clients);
      transport.setPollGrp(0);
      return;
   }
   check(::dup2(other, listener) == listener, "replaced the file behind the listener descriptor");
   closeSocket(other);

   configured = 0;
   pump(transport, pollGrp);
   check(configured == 0, "nothing is accepted through the replaced descriptor");
   check(transport.getTimeTillNextProcessMS() == UINT_MAX, "the transport stops retrying the listener");

   // Put the listener back: the poll group aborts when it cannot remove the
   // registration through the number.
   check(::dup2(saved, listener) == listener, "restored the listener descriptor");
   ::close(saved);
   closeAll(clients);
   transport.setPollGrp(0);
}
#endif

int
main()
{
#ifdef RESIP_POLL_IMPL_EPOLL
   FdPollGrp* pollGrp = FdPollGrp::create("epoll");
   testDescriptorExhaustion(*pollGrp);
   testConnectionSetupThrows(*pollGrp);
   testListenerDescriptorReplaced(*pollGrp);
   delete pollGrp;

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
#else
   cerr << "SKIP: needs the epoll poll group" << endl;
   return 0;
#endif
}
