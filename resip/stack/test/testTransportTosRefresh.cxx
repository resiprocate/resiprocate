#include <cstdlib>
#include <cstring>
#include <iostream>
#include <vector>

#ifndef WIN32
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#endif

#include "resip/stack/SendData.hxx"
#include "resip/stack/TcpTransport.hxx"
#include "resip/stack/TransactionMessage.hxx"
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

// Every socket the transport configures passes through the user's hook, and the
// ToS byte is already on it by then. Recording the descriptor here is what lets
// the test read the value the socket actually holds.
static vector<Socket> configured;

static void
recordSocket(Socket fd, int, const char*, int)
{
   configured.push_back(fd);
}

static int
readTos(Socket fd)
{
   int tos = -1;
   socklen_t len = sizeof(tos);
   ::getsockopt(fd, IPPROTO_IP, IP_TOS, (char *)&tos, &len);
   return tos;
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

static Socket
listenOn(int& port)
{
   Socket peer = ::socket(AF_INET, SOCK_STREAM, 0);
   sockaddr_in addr;
   memset(&addr, 0, sizeof(addr));
   addr.sin_family = AF_INET;
   addr.sin_port = 0;
   ::inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

   if (::bind(peer, (sockaddr *)&addr, sizeof(addr)) != 0 || ::listen(peer, 4) != 0)
   {
      closeSocket(peer);
      return INVALID_SOCKET;
   }

   socklen_t len = sizeof(addr);
   ::getsockname(peer, (sockaddr *)&addr, &len);
   port = ntohs(addr.sin_port);

   return peer;
}

static void
pump(TcpTransport& transport)
{
   for (int i = 0; i < 40; ++i)
   {
      FdSet fdset;
      transport.buildFdSet(fdset);
      fdset.selectMilliSeconds(10);
      transport.process(fdset);
   }
}

int
main()
{
   const size_t CONNECTIONS = 3;

   Fifo<TransactionMessage> fifo;
   // Port 0: the transport throws on a clash, and nothing here would catch it.
   TcpTransport transport(fifo, 0, V4, Data("127.0.0.1"), &recordSocket);
   const int port = transport.port();

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

   pump(transport);
   check(configured.size() >= CONNECTIONS,
         "the transport configured the listener and every accepted connection");

   // What the refresh has to reach: sockets that already existed when the value
   // changed, not only the listener.
   configured.clear();
   transport.setDscp(40);
   pump(transport);

   check(configured.size() >= CONNECTIONS + 1,
         "the refresh reached the listener and each established connection");

   bool everySocketCarriesIt = !configured.empty();
   for (size_t i = 0; i < configured.size(); ++i)
   {
      const int carried = readTos(configured[i]);
      if (carried != 160)
      {
         everySocketCarriesIt = false;
         cerr << "     fd " << configured[i] << " carries " << carried << endl;
      }
   }
   check(everySocketCarriesIt, "every socket the refresh touched holds the new byte");

   // Clearing travels the same path, or a dialog established earlier keeps its
   // old marking for the life of the connection.
   configured.clear();
   transport.setDscp(-1);
   pump(transport);

   bool everySocketCleared = !configured.empty();
   for (size_t i = 0; i < configured.size(); ++i)
   {
      if (readTos(configured[i]) != 0)
      {
         everySocketCleared = false;
      }
   }
   check(everySocketCleared, "clearing the value writes 0 back to those same sockets");

   int peerPort = 0;
   Socket peer = listenOn(peerPort);
   check(peer != INVALID_SOCKET, "opened a peer for the transport to dial");

   transport.setDscp(40);
   pump(transport);

   configured.clear();
   Tuple destination(Data("127.0.0.1"), peerPort, V4, TCP);
   transport.send(transport.makeSendData(destination, Data("OPTIONS sip:x SIP/2.0\r\n\r\n"),
                                         Data("tid-outgoing")));
   pump(transport);

   check(!configured.empty(), "the transport configured the connection it opened");

   bool outgoingCarriesIt = !configured.empty();
   for (size_t i = 0; i < configured.size(); ++i)
   {
      if (readTos(configured[i]) != 160)
      {
         outgoingCarriesIt = false;
      }
   }
   check(outgoingCarriesIt, "a connection the transport opens carries the byte");

   configured.clear();
   transport.setDscp(24);
   pump(transport);

   bool outgoingRefreshed = !configured.empty();
   for (size_t i = 0; i < configured.size(); ++i)
   {
      if (readTos(configured[i]) != 96)
      {
         outgoingRefreshed = false;
      }
   }
   check(outgoingRefreshed, "the refresh reaches a connection the transport opened");

   closeSocket(peer);
   for (size_t i = 0; i < clients.size(); ++i)
   {
      closeSocket(clients[i]);
   }

   cerr << (failures == 0 ? "PASS" : "FAILED") << endl;

   return failures == 0 ? 0 : 1;
}
