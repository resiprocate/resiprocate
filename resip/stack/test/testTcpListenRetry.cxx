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

// Only the epoll group honours FPEM_Edge; the poll and select groups keep
// reporting a listener with queued connections, so they cannot get stuck.
#ifdef RESIP_POLL_IMPL_EPOLL
// The transport passes every connection it accepts through this hook, so the
// count tells how many it accepted.
static size_t configured = 0;

static void
countSocket(Socket, int, const char*, int)
{
   ++configured;
}

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

// The same cycle as TransportThread runs.
static void
pump(TcpTransport& transport, FdPollGrp& pollGrp)
{
   for (int i = 0; i < 20; ++i)
   {
      transport.process();
      pollGrp.waitAndProcess(10);
   }
}
#endif

int
main()
{
#ifdef RESIP_POLL_IMPL_EPOLL
   const size_t CONNECTIONS = 3;

   FdPollGrp* pollGrp = FdPollGrp::create("epoll");
   Fifo<TransactionMessage> fifo;
   // Port 0: the transport throws on a clash, and nothing here would catch it.
   TcpTransport transport(fifo, 0, V4, Data("127.0.0.1"), &countSocket);
   transport.setPollGrp(pollGrp);
   const int port = transport.port();

   // A loopback connect completes into the accept queue before the transport
   // accepts it.
   vector<Socket> clients;
   for (size_t i = 0; i < CONNECTIONS; ++i)
   {
      Socket client = connectTo(port);
      check(client != INVALID_SOCKET, "opened a client connection");
      if (client != INVALID_SOCKET)
      {
         clients.push_back(client);
      }
   }

   // A soft limit equal to the lowest free descriptor makes every accept()
   // fail with EMFILE.
   rlimit saved;
   check(::getrlimit(RLIMIT_NOFILE, &saved) == 0, "read the descriptor limit");
   const int lowestFree = ::open("/dev/null", O_RDONLY);
   ::close(lowestFree);
   rlimit exhausted = saved;
   exhausted.rlim_cur = lowestFree;
   check(::setrlimit(RLIMIT_NOFILE, &exhausted) == 0, "lowered the descriptor limit");

   configured = 0;
   pump(transport, *pollGrp);
   check(configured == 0, "no connection is accepted while descriptors are exhausted");

   // No new connection arrives once descriptors are free again, as when the
   // accept queue is full and the kernel drops every new SYN.
   check(::setrlimit(RLIMIT_NOFILE, &saved) == 0, "restored the descriptor limit");
   pump(transport, *pollGrp);
   check(configured == CONNECTIONS, "the queued connections are accepted once descriptors are free");

   for (size_t i = 0; i < clients.size(); ++i)
   {
      closeSocket(clients[i]);
   }

   transport.setPollGrp(0);
   delete pollGrp;
#else
   cerr << "SKIP: needs the epoll poll group" << endl;
   return 0;
#endif

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
}
