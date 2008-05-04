// SYSTEM INCLUDES
#include <assert.h>
#include <stdio.h>
#ifndef _WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#endif

// APPLICATION INCLUDES
#include "Flow.hxx"
#include "FlowManagerSipXSocket.hxx"

using namespace std;

// Constructor
FlowManagerSipXSocket::FlowManagerSipXSocket(Flow* flow) 
        : OsSocket(),
        mFlow(flow)
{    
}


// Destructor
FlowManagerSipXSocket::~FlowManagerSipXSocket()
{
}

OsSocket* FlowManagerSipXSocket::getSocket()
{
    assert(false);
    return 0;
}

int FlowManagerSipXSocket::getSocketDescriptor() const
{ 
   assert(mFlow);
   return mFlow->getSelectSocketDescriptor();
}

int FlowManagerSipXSocket::read(char* buffer, int bufferLength)
{
   //cout << "read: bufferlen=" << bufferLength << endl;
   assert(mFlow);
   unsigned int len = bufferLength;
   if(mFlow->receive(buffer, len, 0))
   {
      //cout << "read: done: len=0" << endl;
      return 0;
   }

   //cout << "read: done: len=" << len << endl;
   return len;
}

int FlowManagerSipXSocket::read(char* buffer, int bufferLength,
       UtlString* ipAddress, int* port)
{
   asio::ip::address receivedAddress;
   unsigned short receivedPort=0;

   //cout << "read(get address): bufferlen=" << bufferLength << endl;  // **********
   assert(mFlow);

   unsigned int len = bufferLength;
   if(mFlow->receive(buffer, len, 0, &receivedAddress, &receivedPort))
   {
      //cout << "read(get address): done, len=0" << endl;
      return 0;
   }

   if (ipAddress)
   {
      *ipAddress = receivedAddress.to_string().c_str();
   }

   if (port)
   {
       *port = (int)receivedPort ;
   }
   //cout << "read(get address): done, len=" << len << endl;

   return len;
}

int FlowManagerSipXSocket::read(char* buffer, int bufferLength,
       struct in_addr* ipAddress, int* port)
{
    int iRC ;
    int iReceivedPort ;
    UtlString receivedIp ;

    iRC = read(buffer, bufferLength, &receivedIp, &iReceivedPort) ;
    if (ipAddress)
        ipAddress->s_addr = inet_addr(receivedIp) ;

    if (port)
        *port = iReceivedPort ;

    return iRC ;
}


int FlowManagerSipXSocket::read(char* buffer, int bufferLength, long waitMilliseconds)
{        
   //cout << "read: bufferlen=" << bufferLength << ", waitMilliseconds=" << waitMilliseconds << "ms" << endl;
   assert(mFlow);
   unsigned int len = bufferLength;
   if(!mFlow->receive(buffer, len, waitMilliseconds))
   {
      return len;
   }
   else
   {
      return 0;
   }
}

int FlowManagerSipXSocket::write(const char* buffer, int bufferLength)
{
    //cout << "write: bufferlen=" << bufferLength << endl;  // *********
    assert(mFlow);
    mFlow->send((char *)buffer, bufferLength);
    return 0;
}

int FlowManagerSipXSocket::write(const char* buffer, 
                               int bufferLength,
                               const char* ipAddress, 
                               int port)
{
   //cout << "write: bufferlen=" << bufferLength << ", address=" << ipAddress << ", port=" << port << endl;
   assert(mFlow);
   mFlow->sendTo(asio::ip::address::from_string(ipAddress), port, (char*)buffer, bufferLength);
   return 0;
}

int FlowManagerSipXSocket::write(const char* buffer, int bufferLength, 
                               long waitMilliseconds)
{
    //cout << "write: bufferlen=" << bufferLength << ", waitMilliseconds=" << waitMilliseconds << endl;
    assert(0);
    mFlow->send((char*)buffer, bufferLength);  // !SLG! We don't have a timed out send???  Not used by sipX anyway
    return 0;
}


/* ====================================================================

 Original contribution Copyright (C) 2008 Plantronics, Inc.
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
