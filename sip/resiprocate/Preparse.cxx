#include <limits.h>
// limits for UCHAR_MAX

#include "resiprocate/sipstack/HeaderTypes.hxx"
#include "resiprocate/sipstack/Preparse.hxx"
#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/util/Data.hxx"
#include "resiprocate/util/Lock.hxx"
#include "resiprocate/util/Logger.hxx"
#include "resiprocate/util/Mutex.hxx"
#include "resiprocate/util/Socket.hxx"

#define VOCAL_SUBSYSTEM Subsystem::SIP

//#define PP_DEBUG

// Include required facilities for DEBUG only.

#if defined(PP_DEBUG)
#include <iostream> // debug only !ah!
const int ppDebug            = 0x01; // basic debug -- must be set
const int ppDebugEdge        = 0x02;
const int ppDebugEdgeVerbose = 0x04; // ALL edge taken
const int ppDebugActions     = 0x08; // Actions ( not Add )
const int ppDebugPrettyPrint = 0x10; // pp the buffers.

static int ppDebugFlags = 0xff;

#endif


using namespace std;
using namespace Vocal2;
using namespace PreparseConst;

// using namespace Vocal2::PreparseState;


Preparse::Edge *** Preparse::mTransitionTable = 0;
// Instance of the table (static member of Preparse class).



#if !defined(PP_DO_INLINES)
#include "resiprocate/sipstack/PreparseInlines.cxx"
#endif

#if defined(PP_DEBUG)
#include "resiprocate/sipstack/test/TestSupport.hxx"

// DO_BIT is a tasty macro to assemble a Data that
// represents the bitset (s) passed inside.
// it does this by destructively testing
// the mask.

#define DO_BIT(data,mask,bit)                   \
{                                               \
    if (mask & bit)                             \
    {                                           \
        data += #bit;           /* add name */  \
        mask &= ~bit;           /* clear bit */ \
        if (mask)               /* if more */   \
            data += ' ';        /* add space */ \
    }                                           \
}

Data ppStatusName(short s)
{
    
    Data d;
    d += '[';
    DO_BIT(d,s,stNone);
    DO_BIT(d,s,stFragmented);
    DO_BIT(d,s,stDataAssigned);
    DO_BIT(d,s,stPreparseError);
    DO_BIT(d,s,stHeadersComplete);
    d += ']';
    return d;
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

#define PP_STATE_NAME(s) case Preparse::s: return #s;

const char *  ppStateName(Preparse::State s)
{
    switch (s)
    {
        PP_STATE_NAME(NewMsg);
        PP_STATE_NAME(NewMsgCrLf);
        PP_STATE_NAME(StartLine);
        PP_STATE_NAME(StartLineCrLf);
        PP_STATE_NAME(BuildHdr);
        PP_STATE_NAME(EWSPostHdr);
        PP_STATE_NAME(EWSPostColon);
        PP_STATE_NAME(EmptyHdrCrLf);
        PP_STATE_NAME(EmptyHdrCont);
        PP_STATE_NAME(BuildData);
        PP_STATE_NAME(BuildDataCrLf);
        PP_STATE_NAME(CheckCont);
        PP_STATE_NAME(CheckEndHdr);
        PP_STATE_NAME(InQ);
        PP_STATE_NAME(InQEsc);
        PP_STATE_NAME(InAng);
        PP_STATE_NAME(InAngQ);
        PP_STATE_NAME(InAngQEsc);
        PP_STATE_NAME(EndMsg);
        default: assert(0);
    }
	assert(0);
	return 0;
}

#endif


const int X = -1;
const int nOct = UCHAR_MAX + 1;


// Table helpers
//  AE(int start, int disposition, const char *p, int next, int workMask);
//  AE(int start, int disposition, int ch, int next, int workMask);

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
   
    int charStart = (c==X) ? 0 : c;
    int charEnd = (c==X) ? nOct : (c+1);

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

    assert( Data(Symbols::CR).size() == 1);
    assert( Data(Symbols::LF).size() == 1);
    assert( Data(Symbols::LA_QUOTE).size() == 1);
    assert( Data(Symbols::RA_QUOTE).size() == 1);
    assert( Data(Symbols::DOUBLE_QUOTE).size() == 1);
    assert( Data(Symbols::COLON).size() == 1);
    assert( Data(Symbols::B_SLASH).size() == 1);
    assert( Data(Symbols::COMMA).size() == 1);
    assert( Data(Symbols::SPACE).size() == 1);
    assert( Data(Symbols::TAB).size() == 1);

    // Setup the table with useful transitions.
    // NOTE: This is done to (1) make them ints (2) make the automatic diagram
    // have reasonable symbol names.  DO NOT put Symbols::XX in the AE() calls.

    const int CR = (int)Symbols::CR[0];
    const int LF = (int)Symbols::LF[0];
    const int LAQUOT = (int)Symbols::LA_QUOTE[0];
    const int RAQUOT = (int)Symbols::RA_QUOTE[0];
    const int QUOT = (int)Symbols::DOUBLE_QUOTE[0];
   
    const int COLON = (int)Symbols::COLON[0];
    const int LSLASH = (int)Symbols::B_SLASH[0];
    const int COMMA = (int)Symbols::COMMA[0];

    // Remember... this works b/c the assertion above ensured it would.
    const char LWS[3]  =
        {
            Symbols::SPACE[0],
            Symbols::TAB[0],
            0
        }
    ;


    // AE -- add edge(s)
    // AE ( state, disp, char, newstate, work)

    // MUST be AE( format ) so fsm generation will work.
    AE( NewMsg,X,X,StartLine,actAdd); // all chars
    AE( NewMsg,X,CR,NewMsgCrLf,actNone); // eat CR

    AE( NewMsgCrLf,X,X,EndMsg,actBad|actDiscard );
    AE( NewMsgCrLf,X,LF,NewMsg,actDiscard); 

    AE( StartLine,X,X,StartLine,actAdd);
    AE( StartLine,X,CR,StartLineCrLf,actNone);

    AE( StartLineCrLf,X,X,EndMsg,actBad|actDiscard);
    AE( StartLineCrLf,X,LF,BuildHdr,actReset|actFline|actDiscard);

    AE( BuildHdr,X,X, BuildHdr,actAdd);
    AE( BuildHdr,X,LWS,EWSPostHdr,actNone);
    AE( BuildHdr,X,COLON,EWSPostColon,actHdr|actReset|actDiscardKnown);

    AE( EWSPostHdr,X,X,EndMsg,actBad|actDiscard);
    AE( EWSPostHdr,X,LWS,EWSPostHdr,actNone);
    AE( EWSPostHdr,X,COLON,EWSPostColon,actHdr|actReset|actDiscardKnown);

    AE( EWSPostColon,X,X,BuildData,actAdd);
    AE( EWSPostColon,X,LWS,EWSPostColon,actReset|actDiscardKnown);
    AE( EWSPostColon,X,CR,EmptyHdrCrLf,actReset|actDiscardKnown);

    AE( EmptyHdrCrLf,X,X,EndMsg,actBad|actDiscard);
    AE( EmptyHdrCrLf,X,LF,EmptyHdrCont,actReset|actDiscardKnown);
    
    AE( EmptyHdrCont,X,X,BuildHdr,actReset|actBack|actDiscard);
    AE( EmptyHdrCont,X,LWS,EWSPostColon,actReset|actDiscardKnown);
    
    // Add edges that will ''skip'' data building and
    // go straight to quoted states
    AE( EWSPostColon,dCommaSep,LAQUOT,InAng,actAdd );
    AE( EWSPostColon,dCommaSep,QUOT,InQ,actAdd );

    AE( BuildData,X,X,BuildData,actAdd);
    AE( BuildData,X,CR,BuildDataCrLf,actNone|actFlat);

    AE( BuildDataCrLf,X,X,EndMsg,actBad|actDiscard);
    AE( BuildDataCrLf,X,LF,CheckCont,actNone|actFlat);

    // (push 1st then b/u)
    AE( CheckCont,X,X, BuildHdr,actData|actReset|actBack|actDiscard);
    AE( CheckCont,X,LWS,BuildData,actAdd );
   
    // Check if double CRLF (end of hdrs)
    AE( CheckCont,X,CR,CheckEndHdr,actData|actReset|actDiscard);
    AE( CheckEndHdr,X,X,EndMsg,actBad|actDiscard);
    AE( CheckEndHdr,X,LF,EndMsg,actEndHdrs|actDiscard);

    // Disposition sensitive edges

    AE( BuildData,dCommaSep,LAQUOT,InAng,actAdd);

    // Angle Quotes
    AE(InAng,X,X,InAng,actAdd);
    AE(InAng,X,RAQUOT,BuildData,actAdd);
    AE(InAng,X,QUOT,InAngQ,actAdd);

    AE(InAngQ,X,X,InAngQ,actAdd);
    AE(InAngQ,X,QUOT,InAng,actAdd);
    AE(InAngQ,X,LSLASH,InAngQEsc,actAdd);

    AE(InAngQEsc,X,X,InAngQ,actAdd);

    // Bare Quotes
    AE(BuildData,dCommaSep,QUOT,InQ,actAdd);
    AE(InQ,X,X,InQ,actAdd);
    AE(InQ,X,QUOT,BuildData,actAdd);
    AE(InQ,X,LSLASH,InQEsc,actAdd);
    AE(InQEsc,X,X,InQ,actAdd);
   
    // add comma transition
    AE(BuildData,dCommaSep,COMMA,EWSPostColon,actData|actReset|actDiscardKnown);
    initialised = true;
#if defined(PP_DEBUG)
    char *p = getenv("VOCAL_WHIZ_PP_DEBUG_MASK");
    if (p)
    {
       ppDebugFlags = strtol(p,0,0);
    }
#endif
}


Preparse::Preparse():
    mDisposition(dContinuous),
    mState(NewMsg),
    mHeaderOff(0),
    mHeaderLength(0),
    mHeaderType(Headers::UNKNOWN),
    mAnchorBegOff(0),
    mAnchorEndOff(0),
    mStatus(0),
    mStart(0),
    mDiscard(0),
    mUsed(UINT_MAX)
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
    mStart = 0;
    mDiscard = 0;
    mStatus = 0;
    mUsed = UINT_MAX;
}


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
// 
// Alan Hawrylyshen
// Jasomi Networks Inc.

// State info needed across calls:
//  * offset where machine has processed to.
//  * offset where data might be needed to be assigned. (anchors)
//  (by convention, the buffer[0] character IS the start anchor
//   position at the start of the call... the process call will
//   ensure that it reports this position as the discard location
//   if continuity is required accross calls to process).
//

int
Preparse::process(SipMessage& msg,
                  char * buffer,
                  size_t length)
{
     mStatus = stNone;
     
     size_t traversalOff = mStart;
     
     // set internal status variables to (in)sane values
     mUsed = UINT_MAX;
     mDiscard = 0;
     
     // Invariants -- failure is error in caller.
     assert(traversalOff < length);
     assert(buffer);

#if defined(PP_DEBUG)
     if (ppDebugFlags & ppDebug)
     {
        DebugLog(<<"[->] PP::process(...)");
        // !ah! Log all the anchors etc here.
        DebugLog(<<"ppDebugFlags: " << reinterpret_cast<void*>(ppDebugFlags));
        

        DebugLog(<<"mAnchorEndOff:"<<(void*)mAnchorEndOff);
        DebugLog(<<"mAnchorBegOff:"<<(void*)mAnchorBegOff);
        DebugLog(<<"mHeaderType:"<<(void*)mHeaderType);
        DebugLog(<<"mHeaderOff:"<<(void*)mHeaderOff);

        DebugLog(<<"mState:"<< ppStateName(mState));
        DebugLog(<<"mStart: " << mStart);
        DebugLog(<<"buffer: " << (void*)buffer);
	DebugLog(<<"length: " << length);
        DebugLog(<< Data(buffer, length));
     }
#endif

     if (mState == EndMsg) // restart machine if required.
     {
#if defined(PP_DEBUG)
        if (ppDebugFlags & ppDebug)
        {
          DebugLog(<<"PP Restarting");
        }
#endif
         ResetMachine();
     }
     
     while ( traversalOff < length && mState != EndMsg )
     {
         Edge& edge(mTransitionTable[mState][mDisposition][buffer[traversalOff]]);

#if defined(PP_DEBUG)
         // for suppressing the most common uninteresting messages
         // unless EdgeVerbose is set
         bool ppDebugSupress = ((mState == edge.nextState) && (~(ppDebugFlags & ppDebugEdgeVerbose)));
         
         if (!ppDebugSupress &&
             (ppDebugFlags & ppDebug) &&
             (ppDebugFlags & ppDebugEdge))
         {

               DebugLog( << "EDGE " << ppStateName(mState)
                         << " (" << (int) buffer[traversalOff] << ')'
                         << " -> " << ppStateName(edge.nextState)
                         << ::ppWorkString(edge.workMask) );
         }
#endif      
      
         if (edge.workMask & actAdd)
         {
             mAnchorEndOff = traversalOff;

#if defined(PP_DEBUG)
             if ((ppDebugFlags & ppDebugActions) && 
                 (!ppDebugSupress))
             {
                DebugLog( << "+++Accumulated chars '"
                          << Data( 
                             &buffer[mAnchorBegOff],
                             mAnchorEndOff-mAnchorBegOff+1)
                          << '\'' );
             }
             
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
             if (ppDebugFlags & ppDebugActions)
             {
                DebugLog(<<"Hdr \'"
                         << Data(&buffer[mHeaderOff],
                                 mHeaderLength)
                         << "\' Type: " << int(mHeaderType) );
             }
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

             mStatus = mStatus | stDataAssigned;
             
#if defined(PP_DEBUG)
             if (ppDebugFlags & ppDebugActions)
             {
                DebugLog(<<"DATA : \'"
                         << Data(
                            &buffer[mAnchorBegOff],
                            mAnchorEndOff - mAnchorBegOff + 1)
                         << "\' (offset=" << mAnchorBegOff <<")");
             }
#endif
         }

         if (edge.workMask & actFline) // first line complete.
         {
             msg.setStartLine(&buffer[mAnchorBegOff], mAnchorEndOff - mAnchorBegOff + 1);
             mStatus |= stDataAssigned;
#if defined(PP_DEBUG)
             if (ppDebugFlags & ppDebugActions)
             {
                DebugLog(<<"FLINE \'"
                         << Data(&buffer[mAnchorBegOff], mAnchorEndOff - mAnchorBegOff + 1)
                         << "\'");
             }
#endif

         }

	 if (edge.workMask & actFlat)
	 {
#if defined(PP_DEBUG)
            if (ppDebugFlags & ppDebugActions)
            {
	      DebugLog(<<"FLATTEN");
            }
#endif
	      buffer[traversalOff] = ' ';
	 }

         if (edge.workMask & actBack)
         {
#if defined (PP_DEBUG)             
            if (ppDebugFlags & ppDebugActions)
            {
              DebugLog(<<"BACK");
            }
#endif
             traversalOff--;
         }

         if (edge.workMask & actBad)
         {
#if defined (PP_DEBUG)             
            if (ppDebugFlags & ppDebugActions)
            {
               DebugLog(<<"BAD");
            }
#endif
             mStatus |= stPreparseError;

             // Note. All edges that have actBad are also transitioning to
             // EndMsg .. therefore, the FSM will stop processing...
             // (That's why there is no explicit exit from the while() construct
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
             mDiscard = traversalOff;
             // clear frag state here. No longer fragmented.
             mStatus &= ~ stFragmented;
#if defined (PP_DEBUG)
             if (ppDebugFlags & ppDebugActions)
             {
               DebugLog(<<"DISCARD: mDiscard=" << mDiscard << " cur_state=" << ppStateName(mState) << ppWorkString(edge.workMask));
             }
#endif            
         }
         else
         {
             mStatus = mStatus | stFragmented;
         }
         

         mState = edge.nextState;
         // Advance state from machine edge.

         if (edge.workMask & actEndHdrs)
         {
#if defined(PP_DEBUG)
            if (ppDebugFlags && ppDebugActions)
            {
              DebugLog(<<"END_HDR");
            }
#endif
             mStatus |= stHeadersComplete;
         }
      
         if (edge.workMask & actReset) // Leave this AFTER the tp++ (above)
         {
             mAnchorBegOff = mAnchorEndOff = traversalOff;
#if defined(PP_DEBUG)
            if (ppDebugFlags && ppDebugActions)
            {
               DebugLog(<<"RESET");
            }
#endif
         }     
     }

	 // Compute Discard -- always the same, unless fragmented, then
     // discard is zero.

     // update all members for next (stateful) call
     mAnchorEndOff -= mDiscard;
     mAnchorBegOff -= mDiscard;
     mHeaderOff -= mDiscard;
     mUsed = traversalOff;

     // -- This is where we start next time.
     mStart = traversalOff;
     mStart -= mDiscard;

#if defined(PP_DEBUG)
     if (ppDebugFlags && ppDebugActions)
     {
        DebugLog(<<"[<-] PP::process(...)");
        // !ah! Log all the anchors etc here.
        DebugLog(<<"mAnchorEndOff:"<<mAnchorEndOff);
        DebugLog(<<"mAnchorBegOff:"<<mAnchorBegOff);
        DebugLog(<<"mHeaderType:"<<mHeaderType);
        DebugLog(<<"mHeaderOff:"<<mHeaderOff);
        DebugLog(<<"mState:"<<ppStateName(mState));
        DebugLog(<<"nBytes examined " << traversalOff  );
        DebugLog(<< "used:" << mUsed );
        DebugLog(<< "disc:" << mDiscard);
        DebugLog(<< "state: " << ppStateName(mState));
        DebugLog(<< "mStatus: " << ppStatusName(mStatus));
        DebugLog(<< "mStart: " << mStart);
     }
#endif
     return mStatus & stPreparseError;
}


void
Preparse::reset()
{
  ResetMachine();
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
