#if !defined(REND_ACCT_HXX)
#define REND_ACCT_HXX
/**
    Account management for REND. Responsible for keeping track
    of AOR and authentication info.
**/

#include "resip/stack/NameAddr.hxx"

#include "RendMisc.hxx"

/**
    All accounts are given a serial index. This is just an int, but
    we sometimes use this typedef for code clarity
**/
typedef int RendAcctIdx;

class RendAcctMgrIf 
{
public:
   RendAcctMgrIf() {};
   virtual ~RendAcctMgrIf() {};

   virtual resip::NameAddr getNameAddrAor(int acctIdx) = 0;

   virtual resip::Uri getUriAor(int acctIdx) 
   { 
      return getNameAddrAor(acctIdx).uri();
   }

   virtual resip::Data getAorNoPort(int acctIdx) 
   { 
      return getNameAddrAor(acctIdx).uri().getAorNoPort();
   }

   /* Get the number (maximum) accounts. This is used to "spread"
    * computed indexs for local transport selection.
    */
   virtual int getNumAccts() const = 0;

   /* Get the account base, e.g, the first account index. This is
    * used to partition accounts so that different accounts 
    * use different account ranges
    */
   virtual int getAcctBase() const = 0;

   /*
    * Get authorization credentials for particular account
    */
   virtual void getAuth(int acctIdx, const resip::Data& realm, 
                        resip::Data& userVal, resip::Data& passVal) = 0;
   virtual resip::Data getDefaultRealm() = 0;
};

class RendAcctMgrSimple : public RendAcctMgrIf 
{
public:
   struct Pattern 
   {
      Pattern() : mNumericLen(0) {};
      resip::Data mPrefix;
      resip::Data mSuffix;
      int mNumericLen;

      resip::Data eval(int acctIdx);
   };
   RendAcctMgrSimple() {};

   virtual resip::NameAddr getNameAddrAor(int acctIdx);
   virtual void getAuth(int acctIdx, const resip::Data& realm,
                        resip::Data& userVal, resip::Data& passVal);
   virtual int getNumAccts() const { return mNumAccts; };
   virtual int getAcctBase() const { return mAcctBase; };
   virtual resip::Data getDefaultRealm() { return mAuthRealm; }

   /*
   * Note this isn't virtual: not all account managers will allow
   * the number of accounts to be set; e.g., if loaded from file,
   * then number will be determined by file, not arbitrarily.
   */
   void setNumAccts(int numAccts) { mNumAccts = numAccts; }

   void  setAorPattern(const resip::Data& userPrefix, 
                       int userNumericLen,
                       const resip::Data& userSuffix, 
                       const resip::Data& domain) 
   {
      mAorUserPat.mPrefix = userPrefix;
      mAorUserPat.mSuffix = userSuffix;
      mAorUserPat.mNumericLen = userNumericLen;
      mAorDomain = domain;
   }

   void setAuthPattern(const resip::Data& userPrefix, 
                       int userNumericLen,
                       const resip::Data& userSuffix, 
                       const resip::Data& passPrefix, 
                       int passNumericLen,
                       const resip::Data& realm) 
   {
      mAuthUserPat.mPrefix = userPrefix;
      mAuthUserPat.mSuffix = userSuffix;
      mAuthUserPat.mNumericLen = userNumericLen;
      mAuthPassPat.mPrefix = passPrefix;
      mAuthPassPat.mNumericLen = passNumericLen;
      mAuthRealm = realm;
   }

   void setAcctBase(int acctBase) 
   {
      mAcctBase = acctBase;
   }

   void getAuthFromPattern(int acctIdx, const resip::Data& realm,
                           resip::Data& userVal, resip::Data& passVal);

protected:
   int mNumAccts;

   Pattern mAorUserPat;
   resip::Data mAorDomain;

   Pattern mAuthUserPat;
   Pattern mAuthPassPat;
   resip::Data mAuthRealm;

   int mAcctBase;	/* shared aor&user&pass */
};

class RendAcctOptsSimple : public RendOptsBase 
{
public:
   RendAcctOptsSimple()
      : mNumAccts(10), mUserPrefix(0), mUserSuffix(0), 
        mPassPrefix(0), mDomain(0), mNumericLen(6), mAcctBase(0) {}

   int mNumAccts;
   const char* mUserPrefix;
   const char* mUserSuffix;
   const char* mPassPrefix;
   const char* mDomain;
   int mNumericLen;
   int mAcctBase;

   virtual struct poptOption* getPoptTbl();
   virtual const char* getPoptDesc() { return "Account options"; }

   RendAcctMgrSimple* createAcctMgr();
};

extern RendAcctOptsSimple TheRendAcctOptsSimple;
#endif

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
