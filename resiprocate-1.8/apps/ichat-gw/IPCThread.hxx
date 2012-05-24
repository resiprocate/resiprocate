#if !defined(IPCThread_hxx)
#define IPCThread_hxx 

#include <queue>
#include <vector>
#include <string>
#include <sstream>
#include <errno.h>

#ifndef WIN32
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <stdlib.h>
#endif

#include "Thread.hxx"

#define UDP_IPC_BUFFER_SIZE 4192

namespace gateway
{

#ifndef WIN32
typedef int Socket;
#define INVALID_SOCKET (-1)
#define SOCKET_ERROR   (-1)
inline int getErrno() { return errno; }
#else
typedef SOCKET Socket;
inline int getErrno() { return WSAGetLastError(); }
#endif

class IPCMutex
{
public:
   IPCMutex() {}
   virtual ~IPCMutex() {}
   virtual void lock() = 0;
   virtual void unlock() = 0;
};

class IPCMsg
{
public:
   IPCMsg() {};
   IPCMsg(const std::string& msg) 
   {
      unsigned int start = 0;
      for(unsigned int i = 0; i < msg.size(); i++)
      {
         if(msg.at(i) == '\r')
         {
            mArgs.push_back(msg.substr(start,i-start));
            start = i+1;
         }
      }
   }
   void addArg(const std::string& arg) { mArgs.push_back(arg); }
   void addArg(unsigned int arg) 
   {
      std::ostringstream oss;
      oss << arg;
      mArgs.push_back(oss.str()); 
   }
   std::ostream& encode(std::ostream& strm) const 
   { 
      std::vector<std::string>::const_iterator it = mArgs.begin();
      for(;it != mArgs.end();it++)
      {
         strm << *it << "\r";  // Note: using /r since there is a /n character at the start of the iChat call blob
      }
      strm.flush();
      return strm; 
   }
   const std::vector<std::string>& getArgs() const { return mArgs; }
private:
   std::vector<std::string> mArgs;
};

class IPCHandler
{
public:
   virtual ~IPCHandler() {}
   virtual void onNewIPCMsg(const IPCMsg& msg) = 0;
};

class IPCThread : public Thread
{      
public:
   IPCThread(unsigned short localPort, unsigned short remotePort, IPCHandler* handler, IPCMutex* mutex);
   virtual ~IPCThread();

   void sendIPCMsg(IPCMsg& msg);

protected:

private:
   virtual void thread();

   void process(fd_set& read_fdset, fd_set& write_fdset);

   Socket createIPCSocket(const std::string& printableAddr, unsigned int port);
   char mReadBuffer[UDP_IPC_BUFFER_SIZE+1];

   sockaddr mLocalSockaddr;
   sockaddr mRemoteSockaddr;
   Socket mSocket;
   unsigned short mLocalPort;
   unsigned short mRemotePort;
   //resip::Mutex mIPCMutex;
   std::queue<std::string> mDataToSend;

   IPCHandler* mHandler;
   IPCMutex* mMutex;
};

}

#endif  

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

