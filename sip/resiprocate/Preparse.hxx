#if !defined(PREPARSE_HXX)
#define PREPARSE_HXX

#include <sys/types.h>
#include <limits.h>

#include "sip2/sipstack/HeaderTypes.hxx"

namespace Vocal2
{

class Transport;

class SipMessage;            // fwd decl

namespace PreparseState
{

typedef enum 
{
    headersComplete=0,
    preparseError,
    fragment,
    NONE
} TransportAction;
 

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
    EndMsg,
    lastStateMarker
} State;

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
const int actFrag = (1<<8);

  
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
        Preparse();
        
        void process(SipMessage& msg,
                     const char * buffer,
                     size_t length,
                     int& used,
                     PreparseState::TransportAction& status);

        // used returns:
        //  -1 -- needs more data (ran off end)
        //        call setBuffer() before calling process again
        //   n -- Number of bytes used from the current buffer.
        //        
        // messageComplete is true if we processed the end of the mesage.
        // housekeep the buffer overlap and recall:
        // until -1 OR status == headersComplete and used == length.
        // 
        // err -- set to true if there is an error in this buffer, discard.
        // !ah! do we want to chew data until CRLFCRLF??


    private:

        PreparseState::Disposition mDisposition;
        // the disposition of this machine, a function
        // of the mHeader enum
      
        PreparseState::State mState;
        
        // State of machine.  This is manipulated
        // oddly for fragmentation; see the code.
        
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
