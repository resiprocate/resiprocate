#if !defined(REND_DLGACCT_HXX)
#define REND_DLGACCT_HXX 1
/**
 * REND Dialog (Dlg) class
 *
 * Written by Kennard White (Logitech, Inc.), 2010..2011
**/

#include <map>

#include "rutil/SharedPtr.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TransactionUser.hxx"

#include "RendAcct.hxx"

/**
    This describes the category (or purpose) if dialog. Note that some
    of these are pseudo-dialogs.
**/
enum RendDlgCat 
{
   REND_DlgCat_Null,
   REND_DlgCat_Sub,
   REND_DlgCat_Pub,
   REND_DlgCat_Reg,
};
#define REND_DlgCat_MAX REND_DlgCat_Reg


/**
    This structure is used to "name" dialogs by account. It is used
    as index to map of dialogs.
**/
struct RendDlgAcctKey 
{
   RendDlgAcctKey() : mCat(REND_DlgCat_Null), mFromIdx(0), 
                      mToIdx(0), mRepeatIdx(0) { };
   RendDlgAcctKey(RendDlgCat cat, int fromIdx, int toIdx, int rep=0)
      : mCat(cat), mFromIdx(fromIdx), mToIdx(toIdx), mRepeatIdx(rep) {};

   RendDlgCat mCat;
   int mFromIdx;
   int mToIdx;
   int mRepeatIdx;

   bool operator==(const RendDlgAcctKey& b) const 
   {
      return mCat==b.mCat 
             && mFromIdx==b.mFromIdx 
             && mToIdx==b.mToIdx
             && mRepeatIdx==b.mRepeatIdx;
   }
   static const char* CatFmt(RendDlgCat cat);
};

std::ostream&
operator<<(std::ostream& ostrm, const RendDlgAcctKey& key);

#define REND_ITEM_CMP(a,b) if ((a) != (b)) { return (a) < (b); }

struct RendDlgAcctKeyCmp 
{
   bool operator() (const RendDlgAcctKey& a, const RendDlgAcctKey& b) 
   {
      REND_ITEM_CMP(int(a.mCat), int(b.mCat));
      REND_ITEM_CMP(a.mFromIdx,b.mFromIdx); 
      REND_ITEM_CMP(a.mToIdx,b.mToIdx); 
      REND_ITEM_CMP(a.mRepeatIdx,b.mRepeatIdx); 
      return false;	// must be same
   }
};


// index by DlgCat, 
// track time frames: forever, last cycle, now
// 

enum RendCntCode 
{
   REND_CntCode_Null,
   REND_CntCode_ReqGood,
   REND_CntCode_ReqFailRsp,	// got failure response
   REND_CntCode_ReqStaleRsp,	// late/missing rsp to req
   REND_CntCode_ReqStaleNot,	// late/missing notify(s) to req
   REND_CntCode_TupStaleNot,	// #missing tuples from notify(s)
   REND_CntCode_NotifyTerm,
   REND_CntCode_MAX
};

enum RendCntPeriod 
{
   REND_CntPeriod_Null,
   REND_CntPeriod_Current,
   REND_CntPeriod_LastWave,
   REND_CntPeriod_Forever,
   REND_CntPeriod_MAX
};


class RendCntMgr 
{
public:
   RendCntMgr();
   void reset();
   unsigned get(RendCntPeriod per, RendCntCode code, RendDlgCat cat);
   void add(RendCntCode code, RendDlgCat cat, unsigned cnt);

   void inc(RendCntCode code, RendDlgCat cat) 
   {
      add(code, cat, 1);
   }

   void endwave();

   unsigned mCnts[REND_CntPeriod_MAX][REND_CntCode_MAX][REND_DlgCat_MAX];
};

#endif	// end-of-header

/* ====================================================================

 Copyright (c) 2011, Logitech, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Logitech nor the names of its contributors 
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
