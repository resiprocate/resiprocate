#if !defined(PREPARSE_HXX)
#define PREPARSE_HXX

#include <sys/types.h>
#include <limits.h>

#include <sipstack/HeaderTypes.hxx>


namespace Vocal2
{
   class SipMessage;            // fwd decl

   namespace PreparseStateTable
   {
      // Our actions

      const int actNil   = 0;
      const int actAdd   = (1 << 0);
      const int actBack  = (1 << 1);
      const int actFline = (1 << 2);
      const int actReset = (1 << 3);
      const int actHdr   = (1 << 4);
      const int actData  = (1 << 5);
      const int actBad   = (1 << 6);
      const int actEndHdrs = (1 << 7);

  
      typedef enum {
         NewMsg = 0,
         NewMsgCrLf,
         StartLine,
         StartLineCrLf,
         BuildHdr,
         EWSPostHdr,
         EWSPostColon,
         BuildData,
         BuildDataCrLf,
         CheckCont,
         CheckEndHdr,
         InQ,
         InQEsc,
         InAng,
         InAngQ,
         InAngQEsc,
         Done,
         lastStateMarker
      } State;

      typedef enum {
         dCommaSep,
         dContinuous,
         lastDispositionMarker
      } Disposition;

      const int nStates = lastStateMarker;
      const int nDisp   = 2;
      const int nOct    = UCHAR_MAX+1;
    
      /// Our sizes
    
      typedef struct 
      {
            State nextState;
            int workMask;
      } Edge;
  
      void InitStatePreparseStateTable();

   }


   class Preparse
   {
      public:
         Preparse(SipMessage& sipMsg, const char * buffer, size_t length);
         Preparse(SipMessage& sipMsg);
        
         bool process();

         void addBuffer(const char * buffer, size_t length);

      private:
         SipMessage& mSipMessage;
                                // we are pre-parsing this message

         const char * mBuffer;	// the char buffer
         size_t mLength;		// 

         PreparseStateTable::Disposition mDisposition;
         // the disposition of this machine, a function
         // of the mHeader enum
      
         PreparseStateTable::State mState;
         // the state of the machine we are in

         const char * mPtr;		// the current traversal pointer
      
         const char * mHeader;	// the current Header string
         size_t mHeaderLength;  // length of header
         Headers::Type mHeaderType; // the enum for the currently
                                // active header
         

         const char * mAnchorBeg;		// A curious place we anchored.
         // The location of the last actReset.
         const char * mAnchorEnd;

         bool mDone;
        
   };
 
}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
