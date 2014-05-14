
#ifndef _FlowManagerSipXSocket_hxx_
#define _FlowManagerSipXSocket_hxx_

#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif

#include "os/OsSocket.h"
#include "reflow/FlowManager.hxx"

using namespace flowmanager;

namespace recon
{

class FlowManagerSipXSocket : public OsSocket
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

    FlowManagerSipXSocket(Flow* flow, int tos);
    virtual ~FlowManagerSipXSocket();

/* ============================ MANIPULATORS ============================== */

    virtual OsSocket* getSocket();
    
    virtual int read(char* buffer, int bufferLength) ;
    virtual int read(char* buffer, int bufferLength,
            UtlString* ipAddress, int* port);
    virtual int read(char* buffer, int bufferLength,
            struct in_addr* ipAddress, int* port);
    virtual int read(char* buffer, int bufferLength, long waitMilliseconds);


    virtual int write(const char* buffer, int bufferLength);
    virtual int write(const char* buffer, int bufferLength,
                      const char* ipAddress, int port);
    virtual int write(const char* buffer, int bufferLength, 
                      long waitMilliseconds);

    virtual void close() { return; }
   //: Closes the socket

    virtual void makeNonblocking() { return; }
    virtual void makeBlocking() { return; }
   //: Make the connect and all subsequent operations blocking
   // By default the sockets are blocking.

/* ============================ ACCESSORS ================================= */

    virtual OsSocket::IpProtocolSocketType getIpProtocol() const { return OsSocket::CUSTOM; }
   //:Return the protocol type of this socket

   virtual UtlBoolean reconnect() { return TRUE; }
   //:Set up the connection again, assuming the connection failed

   virtual int getSocketDescriptor() const; 
   //:Return the socket descriptor
   // Warning: Use of this method risks the creation of platform-dependent
   // code.

   virtual void getLocalHostIp(UtlString* localHostAddress) const { *localHostAddress = "FlowManagerHost"; }
   //:Return this host's ip address
   // Returns the ip address for this host on which this socket is communicating
   // On multi-homed machines, this is the address to the NIC over which the
   // socket communicates. The format is of the form: xx.x.xxx.x
   
   virtual const UtlString& getLocalIp() const { static UtlString ipAddr="0.0.0.0"; return ipAddr; }
   //:Return this socket's Local Ip Address
      
   virtual int getLocalHostPort() const { return 0; }
   //:Return the local port number
   // Returns the port to which this socket is bound on this host.

   virtual void getRemoteHostName(UtlString* remoteHostName) const { *remoteHostName = "FlowManagerRemoteHost"; }
   //:Return remote host name
   // Returns a string containing the name of the host on which the socket
   // on the other end of this socket is bound. This may be the local
   // name, a fully qualified domain name or anything in between.


   virtual void getRemoteHostIp(struct in_addr* remoteHostAddress,
      int* remotePort = NULL) { printf("getRemoteHostIp\n"); return; }
   //:Return remote host ip address
   // Returns the ip address for the host on which the socket on the
   // other end of this socket is bound.

   virtual void getRemoteHostIp(UtlString* remoteHostAddress,
                        int* remotePort = NULL) { printf("getRemoteHostIp\n"); return; }
   //:Return remote host ip address
   // Returns the ip address for the host on which the socket on the
   // other end of this socket is bound. The format is of the form:
   // xx.x.xxx.x

   virtual int getRemoteHostPort() const { return 0; }
   //:Return the remote port number
   // Returns the port to which the socket on the other end of this socket
   // is bound.


/* ============================ INQUIRY =================================== */

   virtual UtlBoolean isOk() const { return TRUE; }
   //:Returns TRUE if this socket's descriptor is not the invalid descriptor

   virtual UtlBoolean isConnected() const { return TRUE; }
   //:Returns TRUE if this socket is connected

   virtual UtlBoolean isReadyToReadEx(long waitMilliseconds, UtlBoolean &rSocketError) const { return TRUE; }
   //:Poll if there are bytes to read
   // Returns TRUE if socket is read to read.
   // Returns FALSE if wait expires or socket error.
   // rSocketError returns TRUE is socket error occurred.

   virtual UtlBoolean isReadyToRead(long waitMilliseconds = 0) const { return TRUE; }
   //:Poll if there are bytes to read
   // Returns TRUE if socket is ready to read.
   // Returns FALSE if wait expires or socket error.

   virtual UtlBoolean isReadyToWrite(long waitMilliseconds = 0) const { return TRUE; }
   //:Poll if socket is able to write without blocking


/* //////////////////////////// PROTECTED ///////////////////////////////// */
protected:


/* //////////////////////////// PRIVATE /////////////////////////////////// */
private:
   Flow* mFlow;
};

}
/* ============================ INLINE METHODS ============================ */


/* ///////////////////////// HELPER CLASSES /////////////////////////////// */



#endif  // _FlowManagerSipXSocket_h_


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
