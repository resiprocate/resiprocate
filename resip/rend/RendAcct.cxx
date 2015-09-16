/**
**/

// #include <cstdio>

#include "popt.h"

#include "rutil/Logger.hxx"
// #include "resip/stack/NameAddr.hxx"

#include "RendAcct.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

resip::Data
RendAcctMgrSimple::Pattern::eval(int acctIdx) 
{
   char buf[100];
   buf[0] = 0;
   if ( mNumericLen > 0 ) 
   {
#ifdef WIN32
      _snprintf(buf, sizeof(buf), "%0*d", mNumericLen, acctIdx);
#else
      snprintf(buf, sizeof(buf), "%0*d", mNumericLen, acctIdx);
#endif
   }

   return mPrefix + buf + mSuffix;
}

void
RendAcctMgrSimple::getAuthFromPattern(int acctIdx, const resip::Data& realm,
                                      resip::Data& userVal, 
                                      resip::Data& passVal) 
{

   resip_assert(acctIdx>=0);
   userVal = mAuthUserPat.eval(acctIdx+mAcctBase);
   passVal = mAuthPassPat.eval(acctIdx+mAcctBase);
}

void
RendAcctMgrSimple::getAuth(int acctIdx, const resip::Data& realm,
                           resip::Data& userVal, 
                           resip::Data& passVal) 
{
   getAuthFromPattern(acctIdx, realm, userVal, passVal);
}

resip::NameAddr
RendAcctMgrSimple::getNameAddrAor(int acctIdx) 
{
   resip::Data user = mAorUserPat.eval(acctIdx+mAcctBase);
   resip::Data aor("sip:");
   aor += user;
   aor += "@";
   aor += mAorDomain;
   return resip::NameAddr(aor);
}

RendAcctOptsSimple TheRendAcctOptsSimple;
#define OPT TheRendAcctOptsSimple

static struct poptOption RendAcctOptsSimpleTbl[] = 
{
   { "acctnum", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mNumAccts, 0, "Number of sequential accounts", "NUM" },
   { "acctbase", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mAcctBase, 0, "Starting account index", "BASE" },
   { "acctuserpre", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mUserPrefix, 0, "Prefix for account user names", "PRE" },
   { "acctusersuf", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mUserSuffix, 0, "Suffix for account user names", "SUF" },
   { "acctpass", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mPassPrefix, 0, "Prefix for account passwords", "PASS" },
   { "acctsuflen", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mNumericLen, 0, "Length of numeric middle portion", "LEN" },
   { "acctdomain", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mDomain, 0, "Domain and realm for accounts", "DOM" },
   { NULL, 0, 0, NULL, 0 }
};

struct poptOption*
RendAcctOptsSimple::getPoptTbl() 
{
   return RendAcctOptsSimpleTbl;
}

RendAcctMgrSimple*
RendAcctOptsSimple::createAcctMgr() 
{
   RendAcctMgrSimple *acctMgr = new RendAcctMgrSimple;

   acctMgr->setAorPattern(resip::Data(mUserPrefix), 
                          mNumericLen, 
                          resip::Data(mUserSuffix),
                          resip::Data(mDomain));

   acctMgr->setAuthPattern(resip::Data(mUserPrefix), 
                           mNumericLen, 
                           resip::Data(mUserSuffix),
                           resip::Data(mPassPrefix), 
                           mNumericLen, 
                           resip::Data(mDomain));

   acctMgr->setAcctBase(mAcctBase);
   acctMgr->setNumAccts(mNumAccts);

   return acctMgr;
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
