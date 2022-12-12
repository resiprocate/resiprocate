#if !defined(ParkOrbit_hxx)
#define ParkOrbit_hxx

#include <deque>
#include <resip/stack/Uri.hxx>
#include "../HandleTypes.hxx"

namespace mohparkserver
{
class Server;

class ParticipantOrbitInfo
{
public:
   ParticipantOrbitInfo(recon::ParticipantHandle participantHandle, const resip::Uri& parkedUri, const resip::Uri& parkerUri) : 
      mParticipantHandle(participantHandle), mParkedUri(parkedUri), mParkerUri(parkerUri) {} 
   recon::ParticipantHandle mParticipantHandle;
   uint64_t mAllocationTime;
   resip::Uri mParkedUri;
   resip::Uri mParkerUri;
};

class ParkOrbit
{
public:
   ParkOrbit(Server& server, unsigned long orbit, unsigned long maxParkTime, const resip::Uri& musicFilename);
   virtual ~ParkOrbit(); 

   bool addParticipant(recon::ParticipantHandle participantHandle, const resip::Uri& parkedUri, const resip::Uri& parkerUri);
   bool removeParticipant(recon::ParticipantHandle participantHandle);

   recon::ParticipantHandle getNextQueuedParticipant();
   unsigned long getOrbit() { return mOrbit; }
   unsigned long getNumParticipants() { return (unsigned long)mParticipants.size(); }
   recon::ConversationHandle getConversationHandle() { return mConversationHandle; }

   bool onMaxParkTimeout(recon::ParticipantHandle participantHandle);

private:
   friend class ParkManager;
   Server& mServer;
   unsigned long mOrbit;
   unsigned long mMaxParkTime;
   recon::ConversationHandle mConversationHandle;
   typedef std::deque<ParticipantOrbitInfo*> ParticipantQueue;
   ParticipantQueue mParticipants;
};
 
}

#endif

/* ====================================================================

 Copyright (c) 2011, SIP Spectrum, Inc.
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

