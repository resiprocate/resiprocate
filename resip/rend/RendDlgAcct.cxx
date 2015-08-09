/**
**/

#include "RendDlgAcct.hxx"

const char*
RendDlgAcctKey::CatFmt(RendDlgCat cat)
{
   switch ( cat ) 
   {
   case REND_DlgCat_Null: return "N";
   case REND_DlgCat_Sub: return "S";
   case REND_DlgCat_Pub: return "P";
   case REND_DlgCat_Reg: return "R";
   }
   return "?";
}

std::ostream&
operator<<(std::ostream& ostrm, const RendDlgAcctKey& key)
{
   static const char *DlgCatNames[] = { "Nul", "Sub", "Pub", "Reg" };
   const char* catName = key.mCat >= 4 ? "Unk" : DlgCatNames[key.mCat];

   ostrm << "["<<catName <<",f="<<key.mFromIdx<<",t="<<key.mToIdx<<",r="<<key.mRepeatIdx <<"]";
   return ostrm;
}

RendCntMgr::RendCntMgr()
{
   reset();
}

#define CNTCHUNKSIZE (sizeof(unsigned)*REND_CntCode_MAX*REND_DlgCat_MAX)

void
RendCntMgr::reset() 
{
   resip_assert( CNTCHUNKSIZE* REND_CntPeriod_MAX == sizeof(mCnts) );
   memset( mCnts, 0, sizeof(mCnts));
}

void
RendCntMgr::endwave() 
{
   memcpy( mCnts[REND_CntPeriod_LastWave], mCnts[REND_CntPeriod_Current], CNTCHUNKSIZE);
   memset( mCnts[REND_CntPeriod_Current], 0, CNTCHUNKSIZE);
}

void
RendCntMgr::add(RendCntCode code, RendDlgCat cat, unsigned cnt) 
{
   (mCnts[REND_CntPeriod_Current][code][cat]) += cnt;
   (mCnts[REND_CntPeriod_Forever][code][cat]) += cnt;
}

unsigned
RendCntMgr::get(RendCntPeriod per, RendCntCode code, RendDlgCat cat)
{
   return mCnts[per][code][cat];
}

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
