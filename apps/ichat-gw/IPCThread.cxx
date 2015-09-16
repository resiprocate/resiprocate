#include "rutil/ResipAssert.h"
#include <iostream>
#include <sstream>
#include <string.h>

#ifdef WIN32
#include <WS2tcpip.h>
#else
#include <sys/fcntl.h>
#endif

#include "IPCThread.hxx"

using namespace gateway;
using namespace std;


#ifdef WIN32
   #define EADDRINUSE              WSAEADDRINUSE
   #define EWOULDBLOCK             WSAEWOULDBLOCK
   #define sleepMs(t) Sleep(t)
#else
   #define sleepMs(t) usleep(t*1000)
#endif


IPCThread::IPCThread(unsigned short localPort, unsigned short remotePort, IPCHandler* handler, IPCMutex* mutex) : 
    mLocalPort(localPort),
    mRemotePort(remotePort),
    mHandler(handler),
    mMutex(mutex)
{   
   resip_assert(handler!=0);
   mSocket = createIPCSocket("127.0.0.1",mLocalPort);
   if(mSocket == INVALID_SOCKET)
   {
      cerr << "IPCThread::IPCThread - could not create IPC udp socket on port=" << mLocalPort << endl;
   }
   else
   {
      // Set RemoteSockaddr to be same as local but with remote port
      memcpy(&mRemoteSockaddr, &mLocalSockaddr, sizeof(mRemoteSockaddr));
      sockaddr_in* anonv4 = (sockaddr_in*)&mRemoteSockaddr;
      anonv4->sin_port = htons(remotePort);

      cout << "IPCThread::IPCThread:  local port=" << mLocalPort << ", remote port=" << remotePort << endl;
   }
}

IPCThread::~IPCThread()
{
#if defined(WIN32)
   closesocket(mSocket);
#else
   close(mSocket); 
#endif
}

void 
IPCThread::sendIPCMsg(IPCMsg& msg)
{
   std::ostringstream oss;
   msg.encode(oss);
   mMutex->lock();
   mDataToSend.push(oss.str());
   mMutex->unlock();
}

Socket 
IPCThread::createIPCSocket(const std::string& printableAddr, unsigned int port)
{
   Socket fd;
   sockaddr_in* anonv4 = (sockaddr_in*)&mLocalSockaddr;

   fd = ::socket(PF_INET, SOCK_DGRAM, 0);
   
   if ( fd == INVALID_SOCKET )
   {
      int e = getErrno();
      cerr << "IPCThread::createIPCSocket - Failed to create socket: error=" << e << endl;
      return INVALID_SOCKET;
   }

   memset(anonv4, 0, sizeof(anonv4));
   anonv4->sin_family = AF_INET;
   anonv4->sin_port = htons(port);
   if (printableAddr.empty())
   {
      anonv4->sin_addr.s_addr = htonl(INADDR_ANY); 
   }
   else
   {
      anonv4->sin_addr.s_addr = inet_addr(printableAddr.c_str());
   }

   //cout << "IPCThread::createIPCSocket - Creating fd=" << (int)fd 
   //     << ", Binding to " << printableAddr << ":" << port << endl;
   
   if ( ::bind( fd, &mLocalSockaddr, sizeof(mLocalSockaddr)) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
      {
         cerr << "IPCThread::createIPCSocket - " << printableAddr << ":" << port << " already in use" << endl;
      }
      else
      {
         cerr << "IPCThread::createIPCSocket - Could not bind to " << printableAddr << ":" << port << ", error=" << e << endl;
      }
      return INVALID_SOCKET;
   }
   
   if(port == 0)
   {
      // If we used port 0, then query what port the OS allocated for us
      socklen_t len = sizeof(mLocalSockaddr);
      if(::getsockname(fd, &mLocalSockaddr, &len) == SOCKET_ERROR)
      {
         int e = getErrno();
         cerr <<"IPCThread::createIPCSocket - getsockname failed, error=" << e << endl;
         return INVALID_SOCKET;
      }
   }

#if defined(WIN32)
	unsigned long noBlock = 1;
	int errNoBlock = ioctlsocket( fd, FIONBIO , &noBlock );
	if ( errNoBlock != 0 )
	{
      cerr << "IPCThread::createIPCSocket - Could not make socket non-blocking" << endl;
      return INVALID_SOCKET;
	}
#else
	int flags  = fcntl( fd, F_GETFL, 0);
	int errNoBlock = fcntl(fd, F_SETFL, flags | O_NONBLOCK );
	if ( errNoBlock != 0 )
	{
      cerr << "IPCThread::createIPCSocket - Could not make socket non-blocking" << endl;
      return INVALID_SOCKET;
	}
#endif

   return fd;
}

void 
IPCThread::thread()
{
   fd_set read;
   fd_set write;
   fd_set except;
   struct timeval tv;

   if(mSocket == INVALID_SOCKET) return;

   while (!isShutdown())
   {
      tv.tv_sec = 1; // Note:  calling select on linux based systems can cause the contents of the passed in timeval struct to be modified, so we need to reset the time on each loop iteration
      tv.tv_usec = 0;

      FD_ZERO(&read);
      FD_ZERO(&write);
      FD_ZERO(&except);

      FD_SET(mSocket, &read);
      FD_SET(mSocket, &except);

      if(!mDataToSend.empty())
      {
         FD_SET(mSocket, &write);
      }

      int ret = ::select(mSocket+1, &read, &write, &except, &tv);
      if (ret > 0)
      {
         process(read, write);
      }
   }
   cout << "IPCThread::thread - IPCThread shutdown." << endl;
}

void 
IPCThread::process(fd_set& read_fdset, fd_set& write_fdset)
{
   // If data to send and write has signalled
   if(!mDataToSend.empty() && (FD_ISSET(mSocket, &write_fdset) != 0))
   {
      int count;
      mMutex->lock();
      std::string& dataToSend = mDataToSend.front();
      count = sendto(mSocket, 
                     dataToSend.data(), 
                     dataToSend.size(), 
                     0, // flags
                     &mRemoteSockaddr, sizeof(mRemoteSockaddr));
      mDataToSend.pop();
      mMutex->unlock();
      if ( count == SOCKET_ERROR )
      {
         int e = getErrno();
         cerr << "IPCThread::process - Failed (" << e << ") sending." << endl;
      }
   }
   if((FD_ISSET(mSocket, &read_fdset) != 0))
   {
      sockaddr readAddr;
      bool success = true;

      socklen_t slen = sizeof(readAddr);
      int len = recvfrom(mSocket,
                         mReadBuffer, 
                         UDP_IPC_BUFFER_SIZE,
                         0 /*flags */,
                         &readAddr, 
                         &slen);
      if ( len == SOCKET_ERROR )
      {
         int err = getErrno();
         if ( err != EWOULDBLOCK  )
         {
            cerr << "IPCThread::process - Error calling recvfrom: " << err << endl;
            success = false;
         }
      }

      if (len == 0 || len == SOCKET_ERROR)
      {
         cerr << "IPCThread::process - No data calling recvfrom: len=" << len << endl;
         success = false;
      }

      if (len+1 >= UDP_IPC_BUFFER_SIZE)
      {
         cerr << "IPCThread::process - Datagram exceeded max length "<<UDP_IPC_BUFFER_SIZE << endl;
         success = false;
      }

      if(success)
      {
         //cout << "IPCThread::process - Received a datagram of size=" << len << endl;
         IPCMsg msg(std::string(mReadBuffer, len));
         mHandler->onNewIPCMsg(msg);
      }
   }
}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
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

