#include "sip2/sipstack/HeaderTypes.hxx"
#include "sip2/sipstack/Preparse.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Data.hxx"
#include "sip2/util/Lock.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/util/Mutex.hxx"
#include "sip2/util/Socket.hxx"

// #define PP_DEBUG
// #define DEBUG_HELPERS

#if defined(PP_DEBUG) || defined(SUPER_DEBUG) || defined(DEBUG_HELPERS)
#include <iostream> // debug only !ah!
#endif

#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;
using namespace std;

// Table helpers
//  AE(int start, int disposition, const char *p, int next, int workMask);
//  AE(int start, int disposition, int ch, int next, int workMask);

using namespace Vocal2::PreparseState;

Edge *** mTransitionTable = 0;


#if !defined(NDEBUG) && defined(PP_DEBUG)
#if !defined(DEBUG_HELPERS)
#define DEBUG_HELPERS
#endif


Data
showN(const char * p, size_t l)
{
    Data s;
    for(unsigned int i = 0 ; i < l ; i++)
    {
        s += p[i];
    }
    return s;
}
#endif

#if defined (DEBUG_HELPERS)

const char * statusName(PreparseState::TransportAction s)
{
    switch (s)
    {
        case headersComplete: return "headersComplete"; break;
        case preparseError: return "preparseError"; break;
        case fragment: return "fragment"; break;
        case NONE: return "NONE"; break;
    }
    assert(0);
}

const char *  stateName(PreparseState::State s)
{
    switch (s)
    {
        case NewMsg: return "NewMsg";
        case NewMsgCrLf: return "NewMsgCrLf";
        case StartLine: return "StartLine";
        case StartLineCrLf: return "StartLineCrLf";
        case BuildHdr: return "BuildHdr";
        case EWSPostHdr: return "EWSPostHdr";
        case EWSPostColon: return "EWSPostColon";
        case BuildData: return "BuildData";
        case BuildDataCrLf: return "BuildDataCrLf";
        case CheckCont: return "CheckCont";
        case CheckEndHdr: return "CheckEndHdr";
        case InQ: return "InQ";
        case InQEsc: return "InQEsc";
        case InAng: return "InAng";
        case InAngQ: return "InAngQ";
        case InAngQEsc: return "InAngQEsc";
        case EndMsg: return "EndMsg";
        default: assert(0);
            
    }
}


Data
workString(int m)
{
    Data s("[");

    if ( m &  actNil) s += " Nil ";
    if ( m &  actAdd) s += " Add ";
    if ( m &  actBack) s += " Back ";
    if ( m &  actFline) s += " Fline ";
    if ( m &  actReset) s += " Reset ";
    if ( m &  actHdr) s += " Hdr ";
    if ( m &  actData) s += " Data ";
    if ( m &  actBad) s += " Bad ";
    if ( m & actEndHdrs) s += " EndHdrs ";
    if ( m & actFrag ) s += " Frag ";
    s += ']';
    return s;
}
#endif

const int X = -1;
const char XC = -1;

void
AE ( PreparseState::State start,
     int disposition,
     int c,
     PreparseState::State next,
     int workMask)
{
   // for loops below and this code implements the
   // X -- don't care notation for the AE(...) calls.

    int stateStart = (start==X) ? 0 : start;
    int stateEnd = (start==X) ? nStates : start+1;

    int dispStart = (disposition==X) ? 0 : disposition;
    int dispEnd = (disposition==X) ? nDisp : disposition+1;
   
    int charStart = (c==X) ? 0 : (int)c;
    int charEnd = (c==X) ? nOct : (int)(c+1);

    for(int st = stateStart ; st < stateEnd ; st++)
        for(int di = dispStart ; di < dispEnd ; di++)
            for(int ch = charStart ; ch < charEnd ; ch++)
            {
                Edge& e( mTransitionTable[st][di][ch] );
                e.nextState = next;
                e.workMask = workMask;
            }
}

static void
AE (PreparseState::State start,
    int disposition,
    const char * p,
    PreparseState::State next,
    int workMask)
{
    const char *q = p;
    while(*q)
        AE (start, disposition, (int)*q++, next, workMask);
}

void
PreparseState::InitStatePreparseStateTable()
{
    static Mutex m;
    
    Lock l(m); // lock this function against re-entrancy
    
    static bool initialised = false;
   
    // !ah! This needs to be mutexd

    if (initialised) return;

    mTransitionTable = new Edge**[nStates];
    for(int i = 0 ; i < nStates ; i++)
    {
        mTransitionTable[i] = new Edge*[nDisp];
        for(int j = 0 ; j < nDisp ; j++)
        {
            mTransitionTable[i][j] = new Edge[nOct];
            for(int k = 0 ; k < nOct ; k ++)
            {
                Edge& e(mTransitionTable[i][j][k]);
                e.nextState = NewMsg;
                e.workMask = 0;
            }
        }
    }

    // Assert that the Symbols package is as we expect.
    // Better than redefining symbols here.
    // This is only done once at initialisation.

    assert(Symbols::CR && strlen(Symbols::CR) == 1);
    assert(Symbols::LF && strlen(Symbols::LF) == 1);
    assert(Symbols::LA_QUOTE && strlen(Symbols::LA_QUOTE) == 1);
    assert(Symbols::RA_QUOTE && strlen(Symbols::RA_QUOTE) == 1);
    assert(Symbols::DOUBLE_QUOTE && strlen(Symbols::DOUBLE_QUOTE) == 1);
    assert(Symbols::COLON && strlen(Symbols::COLON) == 1);
    assert(Symbols::B_SLASH && strlen(Symbols::B_SLASH) == 1);
    assert(Symbols::COMMA && strlen(Symbols::COMMA) == 1);
    assert(Symbols::SPACE && strlen(Symbols::SPACE) == 1);
    assert(Symbols::TAB && strlen(Symbols::TAB) == 1);

    // Setup the table with useful transitions.
    // NOTE: This is done to (1) make them ints (2) make the automatic diagram
    // have reasonable symbol names.  DO NOT put Symbols::XX in the AE() calls.

    const int CR = (int)(*Symbols::CR);
    const int LF = (int)(*Symbols::LF);
    const int LAQUOT = (int)(*Symbols::LA_QUOTE);
    const int RAQUOT = (int)(*Symbols::RA_QUOTE);
    const int QUOT = (int)(*Symbols::DOUBLE_QUOTE);
   
    const int COLON = (int)(*Symbols::COLON);
    const int LSLASH = (int)(*Symbols::B_SLASH);
    const int COMMA = (int)(*Symbols::COMMA);
    const char LWS[3]  =
        {
            (*Symbols::SPACE),
            (*Symbols::TAB),
            0
        }
    ;


    // AE -- add edge(s)
    // AE ( state, disp, char, newstate, work)

    // MUST be AE( format ) so fsm generation will work.
    AE( NewMsg,X,XC,StartLine,actAdd | actFrag); // all chars
    AE( NewMsg,X,CR,NewMsgCrLf,actNil); // eat CR

    AE( NewMsgCrLf,X,XC,EndMsg,actBad|actFrag);
    AE( NewMsgCrLf,X,LF,NewMsg,actNil); 

    AE( StartLine,X,XC,StartLine,actAdd);
    AE( StartLine,X,CR,StartLineCrLf,actNil);

    AE( StartLineCrLf,X,XC,EndMsg,actBad|actFrag);
    AE( StartLineCrLf,X,LF,BuildHdr,actReset | actFline | actFrag);

    AE( BuildHdr,X,XC, BuildHdr,actAdd);
    AE( BuildHdr,X,LWS,EWSPostHdr,actNil);
    AE( BuildHdr,X,COLON,EWSPostColon,actHdr | actReset | actFrag);
  
    AE( EWSPostHdr,X,XC,EndMsg,actBad|actFrag);
    AE( EWSPostHdr,X,LWS,EWSPostHdr,actNil);
    AE( EWSPostHdr,X,COLON,EWSPostColon,actHdr | actReset | actFrag);

    AE( EWSPostColon,X,XC,BuildData,actAdd);
    AE( EWSPostColon,X,LWS,EWSPostColon,actReset | actFrag);

    // Add edges that will ''skip'' data building and
    // go straight to quoted states
    AE( EWSPostColon,dCommaSep,LAQUOT,InAng,actAdd );
    AE( EWSPostColon,dCommaSep,QUOT,InQ,actAdd );

    AE( BuildData,X,XC,BuildData,actAdd);
    AE( BuildData,X,CR,BuildDataCrLf,actNil);

    AE( BuildDataCrLf,X,XC,EndMsg,actBad|actFrag);
    AE( BuildDataCrLf,X,LF,CheckCont,actNil);

    // (push 1st then b/u)
    AE( CheckCont,X,XC, BuildHdr,actData|actReset|actBack|actFrag);
    AE( CheckCont,X,LWS,BuildData,actAdd );
   
    // Check if double CRLF (end of hdrs)
    AE( CheckCont,X,CR,CheckEndHdr,actData|actReset|actFrag);
    AE( CheckEndHdr,X,XC,EndMsg,actBad|actFrag);
    AE( CheckEndHdr,X,LF,EndMsg,actEndHdrs|actFrag);

    // Disposition sensitive edges

    AE( BuildData,dCommaSep,LAQUOT,InAng,actAdd);

    // Angle Quotes
    AE(InAng,X,XC,InAng,actAdd);
    AE(InAng,X,RAQUOT,BuildData,actAdd);
    AE(InAng,X,QUOT,InAngQ,actAdd);

    AE(InAngQ,X,XC,InAngQ,actAdd);
    AE(InAngQ,X,QUOT,InAng,actAdd);
    AE(InAngQ,X,LSLASH,InAngQEsc,actAdd);

    AE(InAngQEsc,X,XC,InAngQ,actAdd);

    // Bare Quotes
    AE(BuildData,dCommaSep,QUOT,InQ,actAdd);
    AE(InQ,X,XC,InQ,actAdd);
    AE(InQ,X,QUOT,BuildData,actAdd);
    AE(InQ,X,LSLASH,InQEsc,actAdd);
    AE(InQEsc,X,XC,InQ,actAdd);
   
    // add comma transition
    AE(BuildData,dCommaSep,COMMA,EWSPostColon, actData|actReset|actFrag);
   
    initialised = true;
  
}


Preparse::Preparse():
    mDisposition(dContinuous),
    mState(NewMsg)
{
    InitStatePreparseStateTable();
}


#if defined(PP_DEBUG)
#include <ctype.h>

ostream& showchar(ostream& os, char c)
{
    if (isprint(c))
        os << c;
    else
        os << (int)c;

    return os;
   
}
#endif


// Some state needs to be saved between calls to process.
// This state is used for fragmented buffers and so that
// the machine knows where it can start processing the
// next buffer. A pointer into the middle of the buffer
// to the new data is required.  (And the PP will assume
// that it is generating a data ''back'' into the buffer OR,
// We can assume that the PP machine restarts at the frag
// boundary.
//
// Option 1:
//   PROS:
//    o Fewer rescans of the data.
//   CONS:
//    o Harder to implement. Pointers are invalidated when process()
//      returns. Need to cache them as offsets.
//
// Option 2:
//   PROS:
//    o Easier to implement.
//   CONS:
//    o Buffer gets rescaned in part (the fragmented portion of a header).
//
// 
// THE CODE BELOW IMPLEMENTS Option 2.  UNTIL PROVEN INEFFICIENT, IT WILL
// REMAIN THIS WAY.
//
// Alan Hawrylyshen
// Jasomi Networks Inc.

// State info needed across calls:  fragState, fragOffset.

void
Preparse::process(SipMessage& msg,
                  const char * buffer,
                  size_t length,
                  int& used,
                  PreparseState::TransportAction& status)
 {
     Headers::Type headerType = Headers::NONE;
     int headerLength = 0 ; // size of the header string (To,From, etc)

     State fragState = NewMsg;
     const char * fragPtr = buffer;
     
     bool done = false;
     
     const char * traversalPtr = 0;
     const char * headerPtr = 0;
     const char * anchorBeg = 0;
     const char * anchorEnd = 0;

     // initialise the pointers
     traversalPtr = anchorEnd = anchorBeg = buffer;
     headerPtr = 0;
     
     // set return values so they are sensible
     used = -1;
     status = fragmented;
    
     while ( (static_cast<size_t>(traversalPtr - buffer)) < length 
             && mState != EndMsg )
     {
         //using namespace PreparseStateTable;
         Edge& e(mTransitionTable[mState][mDisposition][*traversalPtr]);

#if defined(PP_DEBUG) && defined(SUPER_DEBUG)
         DebugLog( << "EDGE " << ::stateName(mState)
                   << " (" << (int) *traversalPtr << ')'
                   << " -> " << ::stateName(e.nextState)
                   << ::workString(e.workMask) );
#endif      
      
         if (e.workMask & actAdd)
         {
             anchorEnd = traversalPtr;

#if defined(PP_DEBUG) && defined(SUPER_DEBUG)

             DebugLog( << "+++Adding char '"
                       << showN( anchorBeg, anchorEnd-anchorBeg+1)
                       << '\'' );
#endif

         }

         if (e.workMask & actHdr) // this edge indicates a header was seen
         {
             headerPtr = anchorBeg;
             headerLength = anchorEnd-anchorBeg+1;

             headerType = Headers::getType(headerPtr, headerLength);
            

             // ask header class if this is a header that needs to be split on commas?
             if ( Headers::isCommaTokenizing( headerType ))
             {
                 mDisposition = dCommaSep;
             }
             else
             {
                 mDisposition = dContinuous;
             }
#if defined(PP_DEBUG)
             DebugLog(<<"Hdr \'"
                      << showN(headerPtr, headerLength)
                      << "\' Type: " << int(headerType) );
#endif
         }

         if (e.workMask & actData) // there is some header data to pass up
         {
             msg.addHeader(headerType,
                           headerPtr,
                           headerLength,
                           anchorBeg,
                           anchorEnd - anchorBeg + 1
                 );
#if defined(PP_DEBUG)
             DebugLog(<<"DATA : \'"
                      << showN(anchorBeg, anchorEnd - anchorBeg + 1)
                      << "\'");
#endif
         }

         if (e.workMask & actFline) // first line complete.
         {
#if defined(PP_DEBUG)
             DebugLog(<<"FLINE \'"
                      << showN(anchorBeg, anchorEnd - anchorBeg + 1)
                      << "\'");
#endif
             msg.setStartLine(anchorBeg, anchorEnd - anchorBeg + 1);
         }

         if (e.workMask & actBack)
         {
             traversalPtr--;
         }

         if (e.workMask & actFrag)
         {
             fragPtr = traversalPtr;
             fragState = mState;
#if defined (PP_DEBUG)
             DebugLog(<<"FRAG " << fragPtr - buffer << " " << stateName(fragState));
            
#endif            
         }
        
         if (e.workMask & actBad)
         {
             DebugLog(<<"BAD");
             status = PreparseState::preparseError;
         }
         else
         {
             traversalPtr++;
         }
         mState = e.nextState;
        

         if (e.workMask & actEndHdrs)
         {
#if defined(PP_DEBUG)
             DebugLog(<<"END_HDR");
#endif
             status = headersComplete;
         }
      
         if (e.workMask & actReset) // Leave this AFTER the tp++ (above)
         {
             anchorBeg = anchorEnd = traversalPtr;
#if defined(PP_DEBUG)
             DebugLog(<<"RESET");
#endif
         }
      
     }


     // Check our status here ... 

     if ( mState != EndHdrs )
         used = traversalPtr - buffer; // !ah! temporarily disable FRAG support

#if defined(PP_DEBUG)
     DebugLog(<<"nBytes examined " << traversalPtr - buffer  );
     DebugLog(<< "used:" << used );
     DebugLog(<< "state: " << stateName(mState));
     DebugLog(<< "status: " << statusName(status));
#endif
    
     return ;

 }


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
