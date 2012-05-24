#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include <iostream>
#include <string>
#include <asio.hpp>
#include <asio/ssl.hpp>
#include <rutil/ThreadIf.hxx>
#include <rutil/Logger.hxx>

#include "../../StunTuple.hxx"
#include "../../StunMessage.hxx"
#include "../TurnTcpSocket.hxx"
#include "../TurnTlsSocket.hxx"
#include "../TurnUdpSocket.hxx"

using namespace reTurn;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

//#define CONTINUOUSTESTMODE 
#define NO_AUTHENTICATION

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

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
      bool connected = false;

      // Receive Data
      rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      while((!rc || rc.value() == asio::error::operation_aborted) && !isShutdown())
      {
         if(!rc)
         {
            if(!connected)
            {
               turnSocket.connect(sourceAddress.to_string(), sourcePort);
               connected = true;
            }
            InfoLog( << "PEER: Received data from " << sourceAddress.to_string() << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]");
            turnSocket.send(buffer, size);
         }
         size = sizeof(buffer);
         rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort);
      }

      if(rc)
      {
         if(rc.value() != asio::error::operation_aborted)
         {
            ErrLog( << "PEER: Receive error: " << rc.message());
         }
      }
   }
private:
};

#define MAX_RUNS 1000
int main(int argc, char* argv[])
{
   try
   {
      if (argc != 3)
      {
         std::cerr << "Usage: TestClient <turn host> <turn port>\n";
         return 1;
      }
      unsigned int port = resip::Data(argv[2]).convertUnsignedLong();

      asio::error_code rc;
      char username[256] = "test";
      char password[256] = "1234";
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

      TurnUdpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 0);
      //TurnTcpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 0);
      //TurnTlsSocket turnSocket(false /* validateServerCertificateHostname */, asio::ip::address::from_string("127.0.0.1"), 0); 
      //port=5349;

      // Connect to Stun/Turn Server
      rc = turnSocket.connect(argv[1], port);
      if(rc)
      {
         std::cout << "CLIENT: Error calling connect: rc=" << rc.message() << std::endl;
         return 1;
      }

      // Set the username and password
      turnSocket.setUsernameAndPassword(username, password);

      // Test bind request
      rc = turnSocket.bindRequest();
      if(rc)
      {
         InfoLog(<< "CLIENT: Error calling bindRequest: rc=" << rc.message() << ", value=" << rc.value());
         return 1;
      }
      else
      {
         InfoLog( << "CLIENT: Bind Successful!  Reflexive=" << turnSocket.getReflexiveTuple());
      }

      // Test allocation
      rc = turnSocket.createAllocation(30,       // TurnSocket::UnspecifiedLifetime, 
                                       TurnSocket::UnspecifiedBandwidth, 
                                       StunMessage::PropsPortPair,
                                       TurnSocket::UnspecifiedToken,
                                       StunTuple::UDP);
      if(rc)
      {
         InfoLog( << "CLIENT: Error creating allocation: rc=" << rc.message());
      }
      else
      {
         InfoLog( << "CLIENT: Allocation Successful!  Relay=" << turnSocket.getRelayTuple() 
            << " Reflexive=" << turnSocket.getReflexiveTuple() 
            << " Lifetime=" << turnSocket.getLifetime() 
            << " Bandwidth=" << turnSocket.getBandwidth());

         char buffer[1024];
         unsigned int size = sizeof(buffer);
         asio::ip::address sourceAddress;
         unsigned short sourcePort;

         // Test Data sending and receiving over allocation
         resip::Data turnData("This test is for wrapped Turn Data!");
         InfoLog(<< "CLIENT: Sending: " << turnData);
         turnSocket.sendTo(asio::ip::address::from_string("127.0.0.1"), 2000, turnData.c_str(), turnData.size());

         turnData = "This test should be a Channel Data message in TCP/TLS but not in UDP - since ChannelBindResponse is not yet received.";
         InfoLog( << "CLIENT: Sending: " << turnData);
         turnSocket.setActiveDestination(asio::ip::address::from_string("127.0.0.1"), 2000);
         turnSocket.send(turnData.c_str(), turnData.size());

         // Receive Data
         while(!(rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort)))
         {
            InfoLog(<< "CLIENT: Received data from " << sourceAddress.to_string() << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]");
            size = sizeof(buffer);
         }
         if(rc)
         {
            if(rc.value() != asio::error::operation_aborted)
            {
               InfoLog( << "CLIENT: Receive error: [" << rc.value() << "] " << rc.message());
            }          
         }

#ifdef CONTINUOUSTESTMODE
         while(!rc || rc.value() == asio::error::operation_aborted) {
#endif
         turnData = "This test is for ChannelData message!";
         InfoLog( << "CLIENT: Sending: " << turnData);
         turnSocket.send(turnData.c_str(), turnData.size());       


         while(!(rc=turnSocket.receive(buffer, size, 1000, &sourceAddress, &sourcePort)))
         {
            InfoLog(<< "CLIENT: Received data from " << sourceAddress.to_string() << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]");
            size = sizeof(buffer);
         }
         if(rc)
         {
            if(rc.value() != asio::error::operation_aborted)
            {
               InfoLog( << "CLIENT: Receive error: [" << rc.value() << "] " << rc.message());
            }
         }
#ifdef CONTINUOUSTESTMODE
         sleepSeconds(1);
         }//end while
#endif
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

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */
