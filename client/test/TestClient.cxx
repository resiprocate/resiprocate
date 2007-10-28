#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include <iostream>
#include <string>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <rutil/ThreadIf.hxx>

#include "../../StunTuple.hxx"
#include "../../StunMessage.hxx"
#include "../TurnTcpSocket.hxx"
#include "../TurnTlsSocket.hxx"
#include "../TurnUdpSocket.hxx"

using namespace reTurn;
using namespace std;

#define NO_AUTHENTICATION

// Simple UDP Echo Server
class TurnPeer : public resip::ThreadIf
{
public:
   TurnPeer() {}

   virtual ~TurnPeer() {}

   virtual void thread()
   {
      asio::error_code rc;
      TurnUdpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 2000);

      char buffer[1024];
      unsigned int size = sizeof(buffer);
      asio::ip::address sourceAddress;
      unsigned short sourcePort;
      bool activeDestSet = false;

      // Receive Data
      rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      while((rc == 0 || rc == asio::error::operation_aborted) && !isShutdown())
      {
         if(rc == 0)
         {
            if(!activeDestSet)
            {
               turnSocket.setActiveDestination(sourceAddress, sourcePort);
               activeDestSet = true;
            }
            std::cout << "PEER: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]" << std::endl;
            turnSocket.send(buffer, size);
         }
         size = sizeof(buffer);
         rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      }

      if(rc != 0)
      {
         if(rc != asio::error::operation_aborted)
         {
            std::cout << "PEER: Receive error: " << rc.message() << std::endl;
         }
      }
   }
private:
};

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: stunTestClient <host> <port>\n";
      return 1;
    }
    unsigned int port = resip::Data(argv[2]).convertUnsignedLong();

    asio::error_code rc;
    char username[256] = "";
    char password[256] = "";
    TurnPeer turnPeer;
    turnPeer.run();

#ifndef NO_AUTHENTICATION
    {  // Connect via TLS, get SharedSecret, and disconnect
      TurnTlsSocket tlsSocket(asio::ip::address::from_string("127.0.0.1"), 40001);
      rc = tlsSocket.requestSharedSecret(asio::ip::address::from_string(argv[1]), 
                                         port.convertUnsignedLong()+1,
                                         username, sizeof(username),
                                         password, sizeof(password));
    }

    if(rc != 0)
    {
       std::cout << "Error getting shared secret: rc=" << rc..message() << std::endl;
       return 1;
    }

    std::cout << "CLIENT: SharedSecret obtained:  Username=" << username 
              << " Password=" << password
              << std::endl;
#endif

    //TurnUdpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 40000);
    //TurnUdpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 40000, true /* disable turn framing */); port--;
    //TurnTcpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 40000);
    TurnTlsSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 40000); port++;

    rc = turnSocket.createAllocation(asio::ip::address::from_string(argv[1]), 
                                     port, 
                                     username, 
                                     password, 
                                     TurnSocket::UnspecifiedLifetime, 
                                     TurnSocket::UnspecifiedBandwidth, 
                                     StunMessage::PortPropsEvenPair,
                                     TurnSocket::UnspecifiedPort,
                                     StunTuple::UDP);
    if(rc != 0)
    {
       std::cout << "CLIENT: Error creating allocation: rc=" << rc.message() << std::endl;
    }
    else
    {
       std::cout << "CLIENT: Allocation Successful!  Relay=" << turnSocket.getRelayTuple() 
                 << " Reflexive=" << turnSocket.getReflexiveTuple() 
                 << " Lifetime=" << turnSocket.getLifetime() 
                 << " Bandwidth=" << turnSocket.getBandwidth() 
                 << std::endl;

       char buffer[1024];
       unsigned int size = sizeof(buffer);
       asio::ip::address sourceAddress;
       unsigned short sourcePort;

       resip::Data turnData("This test is for wrapped Turn Data!");
       cout << "CLIENT: Sending: " << turnData << endl;
       turnSocket.sendTo(asio::ip::address::from_string("127.0.0.1"), 2000, turnData.c_str(), turnData.size());

       turnData = "This test should be framed in TCP/TLS but not in UDP - since ChannelConfirmed is not yet received.";
       cout << "CLIENT: Sending: " << turnData << endl;
       turnSocket.setActiveDestination(asio::ip::address::from_string("127.0.0.1"), 2000);
       turnSocket.send(turnData.c_str(), turnData.size());
      
       // Receive Data
       while((rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort)) == 0)
       {
          std::cout << "CLIENT: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]" << std::endl;
          size = sizeof(buffer);
       }
       if(rc != 0)
       {
          if(rc != asio::error::operation_aborted)
          {
            std::cout << "CLIENT: Receive error: " << rc.message() << std::endl;
          }          
       }

       turnData = "This test is for framed turn data!";
       cout << "CLIENT: Sending: " << turnData << endl;
       turnSocket.send(turnData.c_str(), turnData.size());       

       while((rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort)) == 0)
       {
          std::cout << "CLIENT: Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]" << std::endl;
          size = sizeof(buffer);
       }
       if(rc != 0)
       {
          if(rc != asio::error::operation_aborted)
          {
             std::cout << "CLIENT: Receive error: " << rc.message() << std::endl;
          }
       }
      
       turnSocket.destroyAllocation();
    }

    turnPeer.shutdown();
    turnPeer.join();
  }
  catch (std::exception& e)
  {
    std::cerr << "Exception: " << e.what() << "\n";
  }

  return 0;
}


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

