#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#if !defined(RESIPROCATE_PP_DIAGNOSTIC)
#define RESIPROCATE_PP_DIAGNOSTIC

// #define PP_DEBUG

#if defined(PP_DEBUG)
/*
** This collection of uglyness is here to make the main Preparser Loop more
** readable  -- this is litterally JUST here for diagnotic output. If there are
** ANY PP related side effects in this code, it is a BUG.
*/

#define PP_DIAG_1                                                              \
     if (ppDebugFlags & ppDebug)                                               \
     {                                                                         \
        DebugLog(<<"[->] PP::process(...)");                                   \
        DebugLog(<<"ppDebugFlags: " << reinterpret_cast<void*>(ppDebugFlags)); \
        DebugLog(<<"mAnchorEndOff:"<<(void*)mAnchorEndOff);                    \
        DebugLog(<<"mAnchorBegOff:"<<(void*)mAnchorBegOff);                    \
        DebugLog(<<"mHeaderType:"<<(void*)mHeaderType);                        \
        DebugLog(<<"mHeaderOff:"<<(void*)mHeaderOff);                          \
        DebugLog(<<"mState:"<< ppStateName(mState));                           \
        DebugLog(<<"mStart: " << mStart);                                      \
        DebugLog(<<"buffer: " << (void*)buffer);                               \
	DebugLog(<<"length: " << length);                                      \
        DebugLog(<< Data(buffer, length));                                     \
     }

#define PP_DIAG_2                               \
        if (ppDebugFlags & ppDebug)             \
        {                                       \
          DebugLog(<<"PP Restarting");          \
        }
#define PP_DIAG_3                                                                                       \
         /* for suppressing the most common uninteresting messages                                      \
         ** unless EdgeVerbose is set                                                                   \
         */                                                                                             \
         bool ppDebugSupress = ((mState == edge.nextState) && (~(ppDebugFlags & ppDebugEdgeVerbose)));  \
                                                                                                        \
         if (!ppDebugSupress &&                                                                         \
             (ppDebugFlags & ppDebug) &&                                                                \
             (ppDebugFlags & ppDebugEdge))                                                              \
         {                                                                                              \
                                                                                                        \
               DebugLog( << "EDGE " << ppStateName(mState)                                              \
                         << " (" << (int) buffer[traversalOff] << ')'                                   \
                         << " -> " << ppStateName(edge.nextState)                                       \
                         << ::ppWorkString(edge.workMask) );                                            \
         }

#define PP_DIAG_4                                               \
             if ((ppDebugFlags & ppDebugActions) &&             \
                 (!ppDebugSupress))                             \
             {                                                  \
                DebugLog( << "+++Accumulated chars '"           \
                          << Data(                              \
                             &buffer[mAnchorBegOff],            \
                             mAnchorEndOff-mAnchorBegOff+1)     \
                          << '\'' );                            \
             }
#define PP_DIAG_5                                               \
             if (ppDebugFlags & ppDebugActions)                 \
             {                                                  \
                DebugLog(<<"Hdr \'"                             \
                         << Data(&buffer[mHeaderOff],           \
                                 mHeaderLength)                 \
                         << "\' Type: " << int(mHeaderType) );  \
             }
#define PP_DIAG_6                                                       \
             if (ppDebugFlags & ppDebugActions)                         \
             {                                                          \
                DebugLog(<<"DATA : \'"                                  \
                         << Data(                                       \
                            &buffer[mAnchorBegOff],                     \
                            mAnchorEndOff - mAnchorBegOff + 1)          \
                         << "\' (offset=" << mAnchorBegOff <<")");      \
             }

#define PP_DIAG_7                                                                               \
             if (ppDebugFlags & ppDebugActions)                                                 \
             {                                                                                  \
                DebugLog(<<"FLINE \'"                                                           \
                         << Data(&buffer[mAnchorBegOff], mAnchorEndOff - mAnchorBegOff + 1)     \
                         << "\'");                                                              \
             }

#define PP_DIAG_8                                               \
             if (ppDebugFlags & ppDebugActions)                 \
             {                                                  \
               DebugLog(<<"DISCARD: mDiscard=" << mDiscard      \
                        << " cur_state=" << ppStateName(mState) \
                        << ppWorkString(edge.workMask));        \
             }

#define PP_DIAG_9                                               \
     if (ppDebugFlags & ppDebugActions)                         \
     {                                                          \
        DebugLog(<<"[<-] PP::process(...)");                    \
        DebugLog(<<"mAnchorEndOff:"<<mAnchorEndOff);            \
        DebugLog(<<"mAnchorBegOff:"<<mAnchorBegOff);            \
        DebugLog(<<"mHeaderType:"<<mHeaderType);                \
        DebugLog(<<"mHeaderOff:"<<mHeaderOff);                  \
        DebugLog(<<"mState:"<<ppStateName(mState));             \
        DebugLog(<<"nBytes examined " << traversalOff  );       \
        DebugLog(<< "used:" << mUsed );                         \
        DebugLog(<< "disc:" << mDiscard);                       \
        DebugLog(<< "state: " << ppStateName(mState));          \
        DebugLog(<< "mStatus: " << ppStatusName(mStatus));      \
        DebugLog(<< "mStart: " << mStart);                      \
     }
#define PP_DIAG_ACTION(x) { DebugLog(<<ppWorkString(x)); }
#else
#define PP_DIAG_1 ;
#define PP_DIAG_2 ;
#define PP_DIAG_3 ;
#define PP_DIAG_4 ;
#define PP_DIAG_5 ;
#define PP_DIAG_6 ;
#define PP_DIAG_7 ;
#define PP_DIAG_8 ;
#define PP_DIAG_9 ;
#define PP_DIAG_ACTION(x) ;

#endif

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

#if defined(PP_DEBUG)
#include "resiprocate/test/TestSupport.hxx"

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
    DO_BIT(d,m,actFlat);
    DO_BIT(d,m,actEmptyHdr);
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
