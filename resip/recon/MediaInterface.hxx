#if !defined(MediaInterface_hxx)
#define MediaInterface_hxx

#if (_MSC_VER >= 1600)
#include <stdint.h>       // Use Visual Studio's stdint.h
#define _MSC_STDINT_H_    // This define will ensure that stdint.h in sipXport tree is not used
#endif
#include <os/OsMsgDispatcher.h>
#include <mi/CpMediaInterface.h>
#include "HandleTypes.hxx"

namespace recon
{
class ConversationManager;

// Wrapper class to allow CpMediaIterface to be stored in a SharedPtr.
// Note:  CpMediaIterface cannot be directly stored in a SharePtr because 
//        the destructor is private and the release() call must be used 
//        to destroy the object.
class MediaInterface : public OsMsgDispatcher
{
public:
   MediaInterface(ConversationManager& conversationManager, ConversationHandle ownerConversationHandle, CpMediaInterface* mediaInterface);
   ~MediaInterface() { mMediaInterface->release(); }
   CpMediaInterface* getInterface() { return mMediaInterface; }
private:
   virtual OsStatus post(const OsMsg& msg);

   ConversationManager& mConversationManager;
   ConversationHandle mOwnerConversationHandle;
   CpMediaInterface* mMediaInterface;
};

}

#endif


/* ====================================================================

 Copyright (c) 2010, SIP Spectrum, Inc.
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
