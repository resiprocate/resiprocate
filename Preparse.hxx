#if !defined(PREPARSE_HXX)
#define PREPARSE_HXX

#include <sys/types.h>

#include "resiprocate/HeaderTypes.hxx"

#if defined(PP_DO_INLINES)
#define INLINE inline
#else
#define INLINE
#endif

namespace resip
{

class Transport;
class SipMessage;            // fwd decl

class Preparse
{
    public:        
        Preparse();
        
        typedef enum StateEnum 
        {
                NewMsg = 0,
                NewMsgCrLf,
                StartLine,
                StartLineCrLf,
                BuildHdr,
                EWSPostHdr,
                EWSPostColon,
                EmptyHdrCrLf,
                EmptyHdrCont,
                BuildData,
                BuildDataCrLf,
                CheckCont,
                CheckEndHdr,
                InQ,
                InQEsc,
                InAng,
                InAngQ,
                InAngQEsc,
                EndMsg,
                nPreparseStates
        }
        State;

        typedef short Action;

        typedef struct 
        {
                State nextState;
                int workMask;
        }
        Edge;

        typedef enum
        {
            dContinuous = 0,
            dCommaSep,
            nDisp
        }
        Disposition;
        
        /// New interface.
        /// -- return non-zero when preparse error.
        int process(SipMessage& msg,
                    char * buffer,
                    size_t length);

        /// Reset internal state for a new message.
        void reset();

        /// Return number of bytes used in last call to 
        //  process. -- used bytes are bytes that have
        //  been looked at by the PP FSM.
        INLINE size_t nBytesUsed();
        
        // Return offset for safe discard of data
        // -- it is safe to discard bytes from the buffer
        //    from this offset onward when copying to the 
        //    new buffer.
        INLINE size_t nDiscardOffset();
        
        /// Return true if any data was assigned to msg
        /// in last call to process()
        //   dataAssigned -- Pointers into this chunk were passed into
        //   the SipMessage (msg).  This data is now REQUIRED to be associated
        //   with th SipMessage msg.  Can be used (with caution) for avoiding
        //   needless copies while dealing with the preparser.
        INLINE bool isDataAssigned();
        
        /// Return true if headers were complete in last
        /// call to process()
        //   headersComplete -- the preparser has detected the end of the
        //   SipMessage headers for this message.  discard is set such that
        //   the body of the SipMessage starts after 'discard' characters.

        INLINE bool isHeadersComplete();
        
        /// Return true if the headers and/or body were fragmented.
        //   fragmented -- more data is needed; call process() again
        //   with the same buffer, concatenated with more data from
        //   the wire; set 'start' to the appropriate value for the
        //   new buffer.  IF dataAssigned is also set on return from
        //   ::process, you MUST assign this buffer to the SipMessage
        //   and the new buffer call for fragment processing has to include
        //   all the data from the current buffer.
        INLINE bool isFragmented();

    private:

        static Edge ***mTransitionTable;
        void InitStatePreparseStateTable();

        void AE( State start, int disposition, int ch,
                 State next, int workMask);

        void AE( State start, int disposition, const char* charset,
                 State next, int workMask);
        
        void ResetMachine();
        // Reset all offsets and state vars to known initial state.

        Disposition mDisposition;
        // the disposition of this machine, a function
        // of the mHeader enum
      
        State mState;
        
        // State of machine.  This is manipulated
        // oddly for fragmentation; see the code.
        // !ah! or not.  See code.

        // Collection of stateful offsets needed to cope with restarting
        // PP::process(...) for fragmented messages.

        size_t mHeaderOff;
        size_t mHeaderLength;
        Headers::Type mHeaderType;
        // NOTE: Header information IF known header means the mHeaderOff and
        // mHeaderLength might be bogus if there was a fragmented packet
        // boundary since the characters that made up the header name.
        // Unknow headers will NOT be fragmented.
        
        size_t mAnchorBegOff;
        size_t mAnchorEndOff;

        short mStatus;
        size_t mStart;
        size_t mDiscard;
        size_t mUsed;
        
};

namespace PreparseConst
{
  const Preparse::Action actNone         = 0;
  const Preparse::Action actAdd          = (1 << 0);
  const Preparse::Action actBack         = (1 << 1);
  const Preparse::Action actFline        = (1 << 2);
  const Preparse::Action actReset        = (1 << 3);
  const Preparse::Action actHdr          = (1 << 4);
  const Preparse::Action actData         = (1 << 5);
  const Preparse::Action actBad          = (1 << 6);
  const Preparse::Action actEndHdrs      = (1 << 7);
  const Preparse::Action actDiscard      = (1 << 8);
  const Preparse::Action actDiscardKnown = (1 << 9);
  const Preparse::Action actFlat         = (1 << 10);
  const Preparse::Action actEmptyHdr     = (1 << 11);
// --
}
#if defined(PP_DO_INLINES)
#include "resiprocate/PreparseInlines.cxx"
#endif
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
