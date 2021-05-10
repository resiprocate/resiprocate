#if !defined(SipXMediaInterface_hxx)
#define SipXMediaInterface_hxx

#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif
#include <os/OsMsgDispatcher.h>
#include <mi/CpMediaInterface.h>
#include <map>
#include <rutil/Mutex.hxx>
#include "HandleTypes.hxx"

namespace recon
{
class ConversationManager;
class FlowManagerSipXSocket;

// Wrapper class to allow CpMediaIterface to be stored in a SharedPtr.
// Note:  CpMediaIterface cannot be directly stored in a SharePtr because 
//        the destructor is private and the release() call must be used 
//        to destroy the object.
// Also manages a map of sipX connectionId to recon ParticipantHandles to
// make event generation quick and easy.
class SipXMediaInterface : public OsMsgDispatcher
{
public:
   SipXMediaInterface(ConversationManager& conversationManager, CpMediaInterface* mediaInterface);
   ~SipXMediaInterface() { mMediaInterface->release(); }

   CpMediaInterface* getInterface() { return mMediaInterface; }
   void allowLoggingDTMFDigits(bool allowLogging) { mAllowLoggingDTMFDigits = allowLogging; }

   // This version of createConnection is used when using FlowManager
   OsStatus createConnection(int& connectionId, ParticipantHandle partHandle, FlowManagerSipXSocket* rtpSocket, FlowManagerSipXSocket* rtcpSocket, bool isMulticast = false);
   // This version of createConnection is used when FlowManager is disabled (see DISABLE_FLOWMANAGER_IF_NO_NAT_TRAVERSAL define)
   OsStatus createConnection(int& connectionId, ParticipantHandle partHandle, const char* localAddress, int localPort);
   void updateConnectionIdToPartipantHandleMapping(int connectionId, ParticipantHandle partHandle);
   OsStatus deleteConnection(int connectionId);
   void setMediaOperationPartipantHandle(ParticipantHandle partHandle) { mLastMediaOperationParticipantHandle = partHandle; }

private:
   ParticipantHandle getParticipantHandleForConnectionId(int connectionId);
   virtual OsStatus post(const OsMsg& msg);

   ConversationManager& mConversationManager;
   CpMediaInterface* mMediaInterface;
   bool mAllowLoggingDTMFDigits;

   // Used to raise DtmfEvent with the source participant handle
   resip::Mutex mConnectionIdToParticipantHandleMapMutex;
   std::map<int, ParticipantHandle> mConnectionIdToPartipantHandleMap;

   // Used to raise MediaEvent with the source participant handle
   ParticipantHandle mLastMediaOperationParticipantHandle;
};

}

#endif


/* ====================================================================

 Copyright (c) 2010-2021, SIP Spectrum, Inc.
 Copyright (c) 2021, Daniel Pocock https://danielpocock.com
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
