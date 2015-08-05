#include "SdpCandidatePair.hxx"

using namespace sdpcontainer;

const char* SdpCandidatePair::SdpCandidatePairCheckStateString[] =
{
   "FROZEN",
   "WAITING",
   "INPROGRESS",
   "SUCCEEDED",
   "FAILED"
};

const char* SdpCandidatePair::SdpCandidatePairOffererTypeString[] =
{
   "LOCAL", 
   "REMOTE"
};

// Constructor
SdpCandidatePair::SdpCandidatePair(const SdpCandidate& localCandidate,
                                   const SdpCandidate& remoteCandidate,
                                   SdpCandidatePairOffererType offerer)
: mLocalCandidate(localCandidate)
, mRemoteCandidate(remoteCandidate)
, mOfferer(offerer)
{
   resetPriority();
   mCheckState = CHECK_STATE_FROZEN;
}

// Copy constructor
SdpCandidatePair::SdpCandidatePair(const SdpCandidatePair& rhs)
{
   operator=(rhs); 
}

// Destructor
SdpCandidatePair::~SdpCandidatePair()
{
}

// Assignment operator
SdpCandidatePair&
SdpCandidatePair::operator=(const SdpCandidatePair& rhs)
{
   if (this == &rhs)            // handle the assignment to self case
      return *this;

   // Assign values
   mLocalCandidate = rhs.mLocalCandidate;
   mRemoteCandidate = rhs.mRemoteCandidate;
   mOfferer = rhs.mOfferer;
   mPriority = rhs.mPriority;
   mCheckState = rhs.mCheckState;

   return *this;
}

bool SdpCandidatePair::setCheckState(const SdpCandidatePairCheckState checkState)
{
   bool stateChangeSuccess=false;
   switch(mCheckState)
   {
   case CHECK_STATE_FROZEN:
      switch(checkState)
      {
      case CHECK_STATE_WAITING:
      case CHECK_STATE_INPROGRESS:
         mCheckState = checkState;
         stateChangeSuccess = true;
         break;
      default:
         resip_assert(false);
      }
      break;
   case CHECK_STATE_WAITING:
      switch(checkState)
      {
      case CHECK_STATE_INPROGRESS:
         mCheckState = checkState;
         stateChangeSuccess = true;
         break;
      default:
         resip_assert(false);
      }
      break;
   case CHECK_STATE_INPROGRESS:
      switch(checkState)
      {
      case CHECK_STATE_SUCCEEDED:
      case CHECK_STATE_FAILED:
         mCheckState = checkState;
         stateChangeSuccess = true;
         break;
      default:
         resip_assert(false);
      }
      break;
   case CHECK_STATE_SUCCEEDED:
   case CHECK_STATE_FAILED:
   default:
      resip_assert(false);
      break;
   }
   return stateChangeSuccess;
}

bool SdpCandidatePair::operator<(const SdpCandidatePair& rhs) const
{
   if(mPriority != rhs.mPriority)
   {
      return mPriority > rhs.mPriority;  // We want to order a list of these from highest priority to lowest - so condition is reversed
   }
   
   if(mCheckState != rhs.mCheckState)
   {
      return mCheckState < rhs.mCheckState;
   }

   if(mLocalCandidate != rhs.mLocalCandidate)
   {
      return mLocalCandidate < rhs.mLocalCandidate;
   }

   if(mRemoteCandidate != rhs.mRemoteCandidate)
   {
      return mRemoteCandidate < rhs.mRemoteCandidate;
   } 

   return false;   // equal
}

#define sdpMin(a,b) (a < b ? a : a > b ? b : a)
#define sdpMax(a,b) (a > b ? a : a < b ? b : a)
void SdpCandidatePair::resetPriority()
{
   UInt64 offererPriority = mOfferer == OFFERER_LOCAL ? mLocalCandidate.getPriority() : mRemoteCandidate.getPriority();
   UInt64 answererPriority = mOfferer == OFFERER_LOCAL ? mRemoteCandidate.getPriority() : mLocalCandidate.getPriority();
   mPriority = (sdpMin(offererPriority, answererPriority)<<32) + 
               (sdpMax(offererPriority, answererPriority)<<1) + 
               (offererPriority > answererPriority ? 1 : 0);
}

EncodeStream& 
sdpcontainer::operator<<( EncodeStream& strm, const SdpCandidatePair& sdpCandidatePair)
{
   strm << "SdpCandidatePair:" << std::endl
        << "  Priority: " << sdpCandidatePair.mPriority << std::endl
        << "  State: " << SdpCandidatePair::SdpCandidatePairCheckStateString[sdpCandidatePair.mCheckState] << std::endl
        << "  Offerer: " << SdpCandidatePair::SdpCandidatePairOffererTypeString[sdpCandidatePair.mOfferer] << std::endl
        << "  " << sdpCandidatePair.mLocalCandidate
        << "  " << sdpCandidatePair.mRemoteCandidate;

   return strm;
}


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

