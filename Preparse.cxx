#include "sip2/sipstack/HeaderTypes.hxx"
#include "sip2/sipstack/Preparse.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Data.hxx"
#include "sip2/util/Lock.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/util/Mutex.hxx"
#include "sip2/util/Socket.hxx"


// #define PP_DEBUG
// #define PP_SUPER_DEBUG


// Set dependancies in debug settings
#if defined(PP_SUPER_DEBUG) && ! defined(PP_DEBUG)
#define PP_DEBUG
#endif

#if !defined(NDEBUG) && defined(PP_DEBUG) && !defined(DEBUG_HELPERS)
#define PP_DEBUG_HELPERS
#endif

// Include required facilities for DEBUG only.

#if defined(PP_DEBUG) || defined(PP_SUPER_DEBUG) || defined(PP_DEBUG_HELPERS)
#include <iostream> // debug only !ah!
#endif

#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace std;
using namespace Vocal2;
using namespace PreparseConst;

// Table helpers
//  AE(int start, int disposition, const char *p, int next, int workMask);
//  AE(int start, int disposition, int ch, int next, int workMask);

// using namespace Vocal2::PreparseState;

Preparse::Edge *** Preparse::mTransitionTable = 0;
// Instance of the table (static member of Preparse class).


#if defined(PP_DEBUG_HELPERS)

// These are routines (for debug only) that output data from
// the preparse buffers. -- they are ugly, but they will not
// be in a stable, client deployed, stack.

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

#if defined (PP_DEBUG_HELPERS)

    // DO_BIT is a tasty macro to assemble a Data that
    // represents the bitset (s) passed inside.
    // it does this by destructively testing
    // the mask.

#define DO_BIT(data,mask,bit)                   \
{                                               \
    if (mask & bit)                             \
    {                                           \
        data += #bit;           /* add name */  \
        mask &= ~bit; /* clear bit */           \
        if (mask)               /* if more */   \
            data += ' ';        /* add space */ \
    }                                           \
}

Data ppStatusName(Preparse::Action s)
{
    unsigned int st = s;
    Data d;
    d += '[';
    DO_BIT(d,st,stNone);
    DO_BIT(d,st,stFragmented);
    DO_BIT(d,st,stDataAssigned);
    DO_BIT(d,st,stPreparseError);
    DO_BIT(d,st,stHeadersComplete);
    d += ']';
    return d;
}

const char *  ppStateName(Preparse::State s)
{
    switch (s)
    {
        case Preparse::NewMsg: return "NewMsg";
        case Preparse::NewMsgCrLf: return "NewMsgCrLf";
        case Preparse::StartLine: return "StartLine";
        case Preparse::StartLineCrLf: return "StartLineCrLf";
        case Preparse::BuildHdr: return "BuildHdr";
        case Preparse::EWSPostHdr: return "EWSPostHdr";
        case Preparse::EWSPostColon: return "EWSPostColon";
        case Preparse::EmptyHdrCrLf: return "EmptyHdrCrLf";
        case Preparse::EmptyHdrCont: return "EmptyHdrCont";
        case Preparse::BuildData: return "BuildData";
        case Preparse::BuildDataCrLf: return "BuildDataCrLf";
        case Preparse::CheckCont: return "CheckCont";
        case Preparse::CheckEndHdr: return "CheckEndHdr";
        case Preparse::InQ: return "InQ";
        case Preparse::InQEsc: return "InQEsc";
        case Preparse::InAng: return "InAng";
        case Preparse::InAngQ: return "InAngQ";
        case Preparse::InAngQEsc: return "InAngQEsc";
        case Preparse::EndMsg: return "EndMsg";
        default: assert(0);
    }
}


// return a descriptive data for the edge work
// coded in 'm'.
Data
ppWorkString(int m)
{
    using namespace PreparseConst;
    
    Data d("[");
    DO_BIT(d,m,actNone);
    DO_BIT(d,m,actAdd);
    DO_BIT(d,m,actBack);
    DO_BIT(d,m,actFline);
    DO_BIT(d,m,actReset);
    DO_BIT(d,m,actHdr);
    DO_BIT(d,m,actData);
    DO_BIT(d,m,actBad);
    DO_BIT(d,m,actEndHdrs);
    DO_BIT(d,m,actDiscard);
    DO_BIT(d,m,actDiscardKnown);
    d += ']';
    return d;
}
#endif

const int X = -1;
const char XC = -1;



void
Preparse::AE ( State start,
     int disposition,
     int c,
     State next,
     int workMask)
{
   // for loops below and this code implements the
   // X -- don't care notation for the AE(...) calls.
   

    int stateStart = (start==X) ? 0 : start;
    int stateEnd = (start==X) ? nPreparseStates : start+1;

    int dispStart = (disposition==X) ? 0 : disposition;
    int dispEnd = (disposition==X) ? nDisp : disposition+1;
   
    int charStart = (c==X) ? 0 : (int)c;
    int charEnd = (c==X) ? nOct : (int)(c+1);

    for(int st = stateStart ; st < stateEnd ; st++)
        for(int di = dispStart ; di < dispEnd ; di++)
            for(int ch = charStart ; ch < charEnd ; ch++)
            {
                Edge& edge( mTransitionTable[st][di][ch] );
                edge.nextState = next;
                edge.workMask = workMask;
            }
}

void
Preparse::AE (State start,
    int disposition,
    const char * p,
    State next,
    int workMask)
{
    const char *q = p;
    while(*q)
        AE (start, disposition, (int)*q++, next, workMask);
}

void
Preparse::InitStatePreparseStateTable()
{
    static Mutex m;
    
    Lock l(m); // lock this function against re-entrancy
    
    static bool initialised = false;
   
    // !ah! This needs to be mutexd

    if (initialised) return;

    mTransitionTable = new Edge**[nPreparseStates];
    for(int i = 0 ; i < nPreparseStates ; i++)
    {
        mTransitionTable[i] = new Edge*[nDisp];
        for(int j = 0 ; j < nDisp ; j++)
        {
            mTransitionTable[i][j] = new Edge[nOct];
            for(int k = 0 ; k < nOct ; k ++)
            {
                Edge& edge(mTransitionTable[i][j][k]);
                edge.nextState = NewMsg;
                edge.workMask = 0;
            }
        }
    }

    // Assert that the Symbols package is as we expect.
    // Better than redefining symbols here.
    // This is only done once at initialisation.
    // This will catch any variance that we don't expect.
    // -- It's a good thing to crash here. Figure out 
    //    what changed in Symbols:: and update as required.
    //    (Analyse the impact on the Preparse FSM).
    //
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
    AE( NewMsg,X,XC,StartLine,actAdd); // all chars
    AE( NewMsg,X,CR,NewMsgCrLf,actNone); // eat CR

    AE( NewMsgCrLf,X,XC,EndMsg,actBad|actDiscard );
    AE( NewMsgCrLf,X,LF,NewMsg,actDiscard); 

    AE( StartLine,X,XC,StartLine,actAdd);
    AE( StartLine,X,CR,StartLineCrLf,actNone);

    AE( StartLineCrLf,X,XC,EndMsg,actBad|actDiscard);
    AE( StartLineCrLf,X,LF,BuildHdr,actReset|actFline|actDiscard);

    AE( BuildHdr,X,XC, BuildHdr,actAdd);
    AE( BuildHdr,X,LWS,EWSPostHdr,actNone);
    AE( BuildHdr,X,COLON,EWSPostColon,actHdr|actReset|actDiscardKnown);

    AE( EWSPostHdr,X,XC,EndMsg,actBad|actDiscard);
    AE( EWSPostHdr,X,LWS,EWSPostHdr,actNone);
    AE( EWSPostHdr,X,COLON,EWSPostColon,actHdr|actReset|actDiscardKnown);

    AE( EWSPostColon,X,XC,BuildData,actAdd);
    AE( EWSPostColon,X,LWS,EWSPostColon,actReset|actDiscardKnown);
    AE( EWSPostColon,X,CR,EmptyHdrCrLf,actReset|actDiscardKnown);

    AE( EmptyHdrCrLf,X,XC,EndMsg,actBad|actDiscard);
    AE( EmptyHdrCrLf,X,LF,EmptyHdrCont,actReset|actDiscardKnown);
    
    AE( EmptyHdrCont,X,XC,BuildHdr,actReset|actBack|actDiscard);
    AE( EmptyHdrCont,X,LWS,EWSPostColon,actReset|actDiscardKnown);
    
    // Add edges that will ''skip'' data building and
    // go straight to quoted states
    AE( EWSPostColon,dCommaSep,LAQUOT,InAng,actAdd );
    AE( EWSPostColon,dCommaSep,QUOT,InQ,actAdd );

    AE( BuildData,X,XC,BuildData,actAdd);
    AE( BuildData,X,CR,BuildDataCrLf,actNone|actFlat);

    AE( BuildDataCrLf,X,XC,EndMsg,actBad|actDiscard);
    AE( BuildDataCrLf,X,LF,CheckCont,actNone|actFlat);

    // (push 1st then b/u)
    AE( CheckCont,X,XC, BuildHdr,actData|actReset|actBack|actDiscard);
    AE( CheckCont,X,LWS,BuildData,actAdd );
   
    // Check if double CRLF (end of hdrs)
    AE( CheckCont,X,CR,CheckEndHdr,actData|actReset|actDiscard);
    AE( CheckEndHdr,X,XC,EndMsg,actBad|actDiscard);
    AE( CheckEndHdr,X,LF,EndMsg,actEndHdrs|actDiscard);

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
    AE(BuildData,dCommaSep,COMMA,EWSPostColon, actData|actReset|actDiscardKnown);
   
    initialised = true;
  
}


Preparse::Preparse():
    mDisposition(dContinuous),
    mState(NewMsg),
    mHeaderOff(0),
    mHeaderLength(0),
    mHeaderType(Headers::UNKNOWN),
    mAnchorBegOff(0),
    mAnchorEndOff(0)
{
    InitStatePreparseStateTable();
    ResetMachine();
}

void
Preparse::ResetMachine()
{
    mDisposition = dContinuous;
    mState = NewMsg;
    mHeaderOff = 0;
    mHeaderLength = 0;
    mHeaderType = Headers::UNKNOWN;
    mAnchorBegOff = 0;
    mAnchorEndOff = 0;
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
// We can assume that the PP machine restarts at the discard
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
                  char * buffer,
                  size_t length,
                  size_t start,
                  size_t& used,
                  size_t& discard,
                  Status& status)
{
     status = stNone;
     
     size_t traversalOff = start;
     
     // set return values so they are sensible
     used = UINT_MAX;
     discard = 0;
     

     // Invariants -- failure is error in caller.
     assert(traversalOff < length);
     assert(buffer);
     assert(start < length);

#    if defined(PP_DEBUG)
     DebugLog(<<"[->] PP::process(...)");
     // !ah! Log all the anchors etc here.
#    if !defined(DEBUG_HELPERS)
#     define ppStateName(x) x
#    endif
     DebugLog(<<"mAnchorEndOff:"<<mAnchorEndOff);
     DebugLog(<<"mAnchorBegOff:"<<mAnchorBegOff);
     DebugLog(<<"mHeaderType:"<<mHeaderType);
     DebugLog(<<"mHeaderOff:"<<mHeaderOff);

     DebugLog(<<"mState:"<< ppStateName(mState));
     DebugLog(<<"buffer: 0x"<<hex<<(unsigned long)buffer<<dec);
#    endif

     if (mState == EndMsg) // restart machine if required.
     {
#        if defined(PP_DEBUG)
          DebugLog(<<"PP Restarting");
#        endif
         ResetMachine();
     }
     
     while ( traversalOff < length && mState != EndMsg )
     {
         Edge& edge(mTransitionTable[mState][mDisposition][buffer[traversalOff]]);

#if defined(PP_DEBUG) && defined(PP_SUPER_DEBUG)
         DebugLog( << "EDGE " << ppStateName(mState)
                   << " (" << (int) buffer[traversalOff] << ')'
                   << " -> " << ppStateName(edge.nextState)
                   << ::ppWorkString(edge.workMask) );
#endif      
      
         if (edge.workMask & actAdd)
         {
             mAnchorEndOff = traversalOff;

#if defined(PP_DEBUG) && defined(PP_SUPER_DEBUG)
             DebugLog( << "+++Adding char '"
                       << showN( &buffer[mAnchorBegOff], mAnchorEndOff-mAnchorBegOff+1)
                       << '\'' );
#endif

         }

         if (edge.workMask & actHdr) // this edge indicates a header was seen
         {
             mHeaderOff = mAnchorBegOff;
             mHeaderLength = mAnchorEndOff-mAnchorBegOff+1;

             mHeaderType = Headers::getType(&buffer[mHeaderOff], mHeaderLength);
            

             // ask header class if this is a header that needs to be split on commas?
             if ( Headers::isCommaTokenizing( mHeaderType ))
             {
                 mDisposition = dCommaSep;
             }
             else
             {
                 mDisposition = dContinuous;
             }

#if defined(PP_DEBUG)
             DebugLog(<<"Hdr \'"
                      << showN(&buffer[mHeaderOff], mHeaderLength)
                      << "\' Type: " << int(mHeaderType) );
#endif
         }

         if (edge.workMask & actData) // there is some header data to pass up
         {
             msg.addHeader(mHeaderType,
                           &buffer[mHeaderOff],
                           mHeaderLength,
                           &buffer[mAnchorBegOff],
                           mAnchorEndOff - mAnchorBegOff + 1
                 );

             status = status | stDataAssigned;
             
#if defined(PP_DEBUG)
             DebugLog(<<"DATA : \'"
                      << showN(&buffer[mAnchorBegOff], mAnchorEndOff - mAnchorBegOff + 1)
                      << "\' (offset=" << mAnchorBegOff <<")");
#endif
         }

         if (edge.workMask & actFline) // first line complete.
         {
             msg.setStartLine(&buffer[mAnchorBegOff], mAnchorEndOff - mAnchorBegOff + 1);
             status |= stDataAssigned;
#if defined(PP_DEBUG)
             DebugLog(<<"FLINE \'"
                      << showN(&buffer[mAnchorBegOff], mAnchorEndOff - mAnchorBegOff + 1)
                      << "\'");
#endif

         }

	 if (edge.workMask & actFlat)
	 {
#	    if defined(PP_DEBUG)
	      DebugLog(<<"FLATTEN");
#           endif
	      buffer[traversalOff] = ' ';
	 }

         if (edge.workMask & actBack)
         {
#            if defined (PP_DEBUG)             
              DebugLog(<<"BACK");
#            endif
             traversalOff--;
         }


         if (edge.workMask & actBad)
         {
#            if defined (PP_DEBUG)             
              DebugLog(<<"BAD");
#            endif
             status |= stPreparseError;
         }
         else
         {
             traversalOff++;
         }

         if ((edge.workMask & actDiscard) ||
             ((edge.workMask & actDiscardKnown) && 
              (mHeaderType != Headers::UNKNOWN)))
         {
             // move discard along to our current offset
             discard = traversalOff;
             // clear frag state here. No longer fragmented.
             status &= ~ stFragmented;
#            if defined (PP_DEBUG)
               DebugLog(<<"DISCARD: discard=" << discard << " cur_state=" << ppStateName(mState) << ppWorkString(edge.workMask));
#            endif            
         }
         else
         {
             status = status | stFragmented;
         }
         

         mState = edge.nextState;
         // Advance state from machine edge.

         if (edge.workMask & actEndHdrs)
         {
#            if defined(PP_DEBUG)
              DebugLog(<<"END_HDR");
#            endif
             status |= stHeadersComplete;
         }
      
         if (edge.workMask & actReset) // Leave this AFTER the tp++ (above)
         {
             mAnchorBegOff = mAnchorEndOff = traversalOff;
#if defined(PP_DEBUG)
             DebugLog(<<"RESET");
#endif
         }
      
     }


     
     // Compute Discard -- always the same, unless fragmented, then
     // discard is zero.


     // update all members for next (stateful) call
     mAnchorEndOff -= discard;
     mAnchorBegOff -= discard;
     mHeaderOff -= discard;
     used = traversalOff;

#if defined(PP_DEBUG)
     DebugLog(<<"[<-] PP::process(...)");
     // !ah! Log all the anchors etc here.
     DebugLog(<<"mAnchorEndOff:"<<mAnchorEndOff);
     DebugLog(<<"mAnchorBegOff:"<<mAnchorBegOff);
     DebugLog(<<"mHeaderType:"<<mHeaderType);
     DebugLog(<<"mHeaderOff:"<<mHeaderOff);
     DebugLog(<<"mState:"<<ppStateName(mState));
     DebugLog(<<"nBytes examined " << traversalOff  );
     DebugLog(<< "used:" << used );
     DebugLog(<< "disc:" << discard);
     DebugLog(<< "state: " << ppStateName(mState));
     DebugLog(<< "status: " << ppStatusName(status));
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
