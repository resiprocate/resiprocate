#if !defined(PREPARSE_HXX)
#define PREPARSE_HXX

#include <sys/types.h>
#include <limits.h>

#include "sip2/sipstack/HeaderTypes.hxx"

namespace Vocal2
{

class Transport;

class SipMessage;            // fwd decl


  
typedef enum {
    dCommaSep,
    dContinuous,
    lastDispositionMarker
} Disposition;

const int nDisp   = 2;
const int nOct    = UCHAR_MAX+1;
    
/// Our sizes
  
 
//}





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

        typedef short Status;
        
        
        typedef struct 
        {
                State nextState;
                int workMask;
        }
        Edge;
        
        void process(SipMessage& msg,
                     char * buffer,
                     size_t length,
                     size_t start,
                     size_t& used,
                     size_t& discard,
                     Status& status);

        // msg -- A SipMessage for use with this data buffer.
        //        Will have HFV's installed by the pre-parser.
        //
        // buffer -- The character data to work on.  non-const since
        //           it can flatten CRLFs
        // 
        // length -- The number of chars in buffer.
        //
        // start -- begin the Preparse at this char offset.
        //          Used to restart on fragments.
        //
        // discard -- the preparse process has used this many
        //            chars and they are no longer required for
        //            subsequent preparse processing.
        // 
        // status -- returns a bitset. If the following bits are set,
        // the listed action / situation applies.
        //
        //   preparseError -- There was an error processing this chunk
        //   of data. You may discard up to 'discard' bytes of this
        //   packet and take appropriate action for the transport
        //   invovled (UDP - discard packet, TCP - close connection).
        //
        //   dataAssigned -- Pointers into this chunk were passed into
        //   the SipMessage (msg).  This data is now REQUIRED to be associated
        //   with th SipMessage msg.  Can be used (with caution) for avoiding
        //   needless copies while dealing with the preparser.
        //
        //   headersComplete -- the preparser has detected the end of the
        //   SipMessage headers for this message.  discard is set such that
        //   the body of the SipMessage starts after 'discard' characters.
        //  
        //   fragmented -- more data is needed; call process() again
        //   with the same buffer, concatenated with more data from
        //   the wire; set 'start' to the appropriate value for the
        //   new buffer.  IF dataAssigned is also set on return from
        //   ::process, you MUST assign this buffer to the SipMessage
        //   and the new buffer call for fragment processing has to include
        //   all the data from the current buffer.
        //


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
// --
const Preparse::Status stNone            = 0;
const Preparse::Status stHeadersComplete = (1 << 0);
const Preparse::Status stPreparseError   = (1 << 1);
const Preparse::Status stDataAssigned    = (1 << 2);
const Preparse::Status stFragmented      = (1 << 3);
// --

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
