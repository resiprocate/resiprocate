
#ifndef _FlowManagerSipXSocket_hxx_
#define _FlowManagerSipXSocket_hxx_

#include "FlowManager.hxx"

using namespace flowmanager;

namespace recon
{

class FlowManagerSipXSocket 
{
/* //////////////////////////// PUBLIC //////////////////////////////////// */
public:

    FlowManagerSipXSocket(Flow* flow);
    virtual ~FlowManagerSipXSocket();

/* ============================ MANIPULATORS ============================== */

    
    virtual int read(char* buffer, int bufferLength) ;
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

   //:Set up the connection again, assuming the connection failed

   virtual int getSocketDescriptor() const; 
   //:Return the socket descriptor
   // Warning: Use of this method risks the creation of platform-dependent
   // code.



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
