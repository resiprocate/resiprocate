#ifdef WIN32
#pragma warning(disable : 4267)
#endif

#include <iostream>
#include <string>
#include <asio.hpp>
#include <asio/ssl.hpp>

#include "../StunTuple.hxx"
#include "../StunMessage.hxx"
#include "../TurnTcpSocket.hxx"
#include "../TurnTlsSocket.hxx"
#include "../TurnUdpSocket.hxx"

using namespace reTurn;

int main(int argc, char* argv[])
{
  try
  {
    if (argc != 3)
    {
      std::cerr << "Usage: stunTestClient <host> <port>\n";
      return 1;
    }
    resip::Data port(argv[2]);

    asio::error_code rc;
    char username[256];
    char password[256];

    {  // Connect via TLS, get SharedSecret, and disconnect
      TurnTlsSocket tlsSocket(asio::ip::address::from_string("127.0.0.1"), 40001);
      rc = tlsSocket.requestSharedSecret(asio::ip::address::from_string(argv[1]), 
                                         port.convertUnsignedLong()+1,
                                         username, sizeof(username),
                                         password, sizeof(password));
    }

    if(rc != 0)
    {
       std::cout << "Error getting shared secret: rc=" << rc.value() << std::endl;
       return 1;
    }

    std::cout << "SharedSecret obtained:  Username=" << username 
              << " Password=" << password
              << std::endl;

    TurnUdpSocket turnSocket(asio::ip::address::from_string("127.0.0.1"), 40000);

    rc = turnSocket.createAllocation(asio::ip::address::from_string(argv[1]), 
                                     port.convertUnsignedLong(), 
                                     username, 
                                     password, 
                                     TurnSocket::UnspecifiedLifetime, 
                                     TurnSocket::UnspecifiedBandwidth, 
                                     StunMessage::PortPropsEvenPair);
    if(rc != 0)
    {
       std::cout << "Error creating allocation: rc=" << rc.value() << std::endl;
    }
    else
    {
       std::cout << "Allocation Successful!  Relay=" << turnSocket.getRelayTuple() 
                 << " Reflexive=" << turnSocket.getReflexiveTuple() 
                 << " Lifetime=" << turnSocket.getLifetime() 
                 << " Bandwidth=" << turnSocket.getBandwidth() 
                 << std::endl;
       resip::Data turnData("This test is for wrapped Turn Data!");
       turnSocket.sendTo(asio::ip::address::from_string("127.0.0.1"), 2000, turnData.c_str(), turnData.size());
       turnSocket.setActiveDestination(asio::ip::address::from_string("127.0.0.1"), 2000);
       turnData = "This test is for framed turn data!";
       turnSocket.send(turnData.c_str(), turnData.size());

       // Receive Data
       char buffer[1024];
       unsigned int size = sizeof(buffer);
       asio::ip::address sourceAddress;
       unsigned short sourcePort;
       if(turnSocket.receive(buffer, size, &sourceAddress, &sourcePort) == 0)
       {
          std::cout << "Received data from " << sourceAddress << ":" << sourcePort << " - [" << resip::Data(buffer, size).c_str() << "]" << std::endl;
          turnSocket.send(buffer, size);
          size = sizeof(buffer);
       }

       turnSocket.clearActiveDestination();
       turnSocket.destroyAllocation();
    }
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

