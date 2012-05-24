#if !defined(SdpCandidatePair_hxx)
#define SdpCandidatePair_hxx

#include "SdpCandidate.hxx"

namespace sdpcontainer
{

class SdpCandidatePair
{
public:

   typedef enum
   {
      CHECK_STATE_FROZEN,
      CHECK_STATE_WAITING,
      CHECK_STATE_INPROGRESS,
      CHECK_STATE_SUCCEEDED,
      CHECK_STATE_FAILED
   } SdpCandidatePairCheckState;
   static const char* SdpCandidatePairCheckStateString[];

   typedef enum
   {
      OFFERER_LOCAL,
      OFFERER_REMOTE
   } SdpCandidatePairOffererType;
   static const char* SdpCandidatePairOffererTypeString[];

   SdpCandidatePair(const SdpCandidate& localCandidate,
                    const SdpCandidate& remoteCandidate,
                    SdpCandidatePairOffererType offerer);

   SdpCandidatePair(const SdpCandidatePair& rSdpCandidatePair);

   virtual
   ~SdpCandidatePair();

   SdpCandidatePair& operator=(const SdpCandidatePair& rhs);
   bool operator<(const SdpCandidatePair& rhs) const;

   void setLocalCandidate(const SdpCandidate& localCandidate) { mLocalCandidate = localCandidate; resetPriority(); }
   void setRemoteCandidate(const SdpCandidate& remoteCandidate) { mRemoteCandidate = remoteCandidate; resetPriority(); }
   void setOfferer(const SdpCandidatePairOffererType offerer) { mOfferer = offerer; resetPriority(); }
   bool setCheckState(const SdpCandidatePairCheckState checkState);

   void toString(resip::Data& sdpCandidateString) const;

   const SdpCandidate& getLocalCandidate() const { return mLocalCandidate; }
   const SdpCandidate& getRemoteCandidate() const { return mRemoteCandidate; }
   SdpCandidatePairOffererType getOfferer() const { return mOfferer; }
   UInt64 getPriority() const { return mPriority; }
   SdpCandidatePairCheckState getCheckState() const { return mCheckState; }

private:
   void resetPriority();

   SdpCandidate mLocalCandidate;
   SdpCandidate mRemoteCandidate;
   SdpCandidatePairOffererType mOfferer;
   UInt64     mPriority;
   SdpCandidatePairCheckState mCheckState;

   friend EncodeStream& operator<<(EncodeStream& strm, const SdpCandidatePair& );
};

EncodeStream& operator<< ( EncodeStream& strm, const SdpCandidatePair& );

} // namespace

#endif  

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

