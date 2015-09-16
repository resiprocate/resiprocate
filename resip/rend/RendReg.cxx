/**
    REND REGISTER sketches

    Written by Kennard White (Logitech, Inc.), 2010..2011
**/


#include "rutil/Logger.hxx"
// #include "rutil/Random.hxx"

// #include "RendAcct.hxx"
// #include "RendDlg.hxx"
// #include "RendDlgAcct.hxx"
#include "RendTroop.hxx"
#include "RendSketch.hxx"

#include "popt.h"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP

/**
What is real difference between TuEngine and Wave?

Reg thing:

issue new REGs at rate cps. if current num dlgs > target dlgs, kill some.
rotate thru accounts?

convert cps into starts/tick. every tick period, start that many somethings.

when is a something a new user vs renewal of prior? do renewals count
in the cps? when just kill user and move on to next one?
actions are:
    start: REG a currently unstarted user
    renew: reREG a currently running user
    stop: unREG a current running user

could have unREG%, and each tick (on avg) that % of the actions will be unREG.
the process renews with remaining cps, then process starts with any left
over cps. starts and stops are limited by the target session value

need list of started, list of stopped. Also those in transition.
Also need list of those needing renewal.

the repeat index can serve as multiple regs for same user

can pick next user to do stuff to either linearly or based upon random number

in wave mode, everyone in troop does same thing (with some rate limiting),
and waiting until complete.everyone stays in sync because we wait.

in non-wave mode, we keep issuings at fixed rate, and given dialogs either
keep up or not. Thus the key difference is in the waiting.
**/


/********************************************************************
 *
 * REGISTER-specific classes
 *
 ********************************************************************/

/**
    It is somewhat unclear which AOR of the REGISTER request
    is "the" AOR to register: R-URI, To, or From.

    Somewhere in RFC 3261 (cf needed) it indicates the R-URI is used.
    Opensips (see modules/registrar/save.c) uses the To header.
    The "To" header probably makes most sense: it allows for
    3rd party registrations, where the 3rd party is identified 
    by the From header, and auth is against the From header.
**/


class RendDlgReg : public RendTroopDlg
{
public:
   RendDlgReg(RendTroopBase& troop, RendTu& tu, RendAcctIdx acctIdx, int regId) 
      : RendTroopDlg(tu, acctIdx, acctIdx), mTroop(troop), mRegId(regId) 
   { 
      mFlags |= REND_DlgF_KeepAlive;
      resip_assert(acctIdx>=0); 
   }
   virtual resip::SipMessage*	makeNextReq();

   RendTroopBase& mTroop;
   int mRegId;

   virtual RendTroopBase& getTroop() { return mTroop; }
   virtual int getRspExpire(const resip::SipMessage& rsp);

protected:
   const resip::NameAddr* getMyContact(const resip::SipMessage& msg);
   const resip::NameAddr* getContactByInstance(const resip::SipMessage& msg, 
                                               const resip::Data& inst, int regId);
   const resip::NameAddr* getContactByTransport(const resip::SipMessage& msg, 
                                                const resip::Transport& tp);
};

class RendTroopReg : public RendTroopBase
{
public:
   RendTroopReg(RendTimeUs now, RendTu& tu, RendCntMgr& cntMgr) 
      : RendTroopBase(now, tu, REND_DlgCat_Reg, cntMgr) 
   {
   }
   virtual RendTroopDlg* createNewDlg(const RendDlgAcctKey& key);
};

RendTroopDlg*
RendTroopReg::createNewDlg(const RendDlgAcctKey& key) 
{
    return new RendDlgReg(*this, mTu, key.mToIdx, key.mRepeatIdx);
}

#define REND_REG_SIPINSTANCE "<rend>"

resip::SipMessage*
RendDlgReg::makeNextReq()
{
   resip::SipMessage *req = makeRequest(resip::REGISTER);
   // +sip.instance SHOULD be a urn:uuid, but skip that for now
   // we want to collapse all registration accross all instances
   // of this program. If want multi regs for AOR, then use the repeatIdx
   if ( mRegId >= 0 ) 
   {
      // The angle brackets are specified in the RFC 5626 grammar
      req->header(resip::h_Contacts).front().param(resip::p_Instance) = REND_REG_SIPINSTANCE;
      req->header(resip::h_Contacts).front().param(resip::p_regid) = mRegId;
   }
   return req;
}

const resip::NameAddr*
RendDlgReg::getContactByInstance(const resip::SipMessage& msg, const resip::Data& inst, int regId)
{
   const resip::NameAddrs& contacts = msg.header(resip::h_Contacts);
   resip::NameAddrs::const_iterator it = contacts.begin();
   for ( ; it != contacts.end(); it++) 
   {
      if ( ! it->exists(resip::p_Instance) 
         || it->param(resip::p_Instance)!=inst )
      {
         continue;
      }
      if ( regId>0 
         && (!it->exists(resip::p_regid) 
         || it->param(resip::p_regid) != (UInt32)regId) )
      {
         continue;
      }
      return &*it;
   }
   return NULL;
}

const resip::NameAddr*
RendDlgReg::getContactByTransport(const resip::SipMessage& msg, const resip::Transport& tp) 
{
   const resip::NameAddrs& contacts = msg.header(resip::h_Contacts);
   resip::NameAddrs::const_iterator it = contacts.begin();
   for ( ; it != contacts.end(); it++) 
   {
#if 1
      InfoLog(<<"contacturi="<<(it->uri()) <<" tpif="<<tp.interfaceName() <<" tpport="<<tp.port());
#endif
      if ( it->uri().host() == tp.interfaceName() && it->uri().port() == tp.port() ) 
      {
            return &*it;
      }
   }
   return NULL;
}

const resip::NameAddr*
RendDlgReg::getMyContact(const resip::SipMessage& msg)
{
   const resip::NameAddr* mycontact;
   resip::Data sipinst(REND_REG_SIPINSTANCE);
   if ( (mycontact=getContactByInstance(msg, sipinst, mRegId)) )
   {
      return mycontact;
   }
   if ( mTransport && (mycontact=getContactByTransport(msg, *mTransport)) )
   {
      return mycontact;
   }
   // due to NAT munging of the addr, the above might fail
   const resip::NameAddrs& contacts = msg.header(resip::h_Contacts);
   if ( ! contacts.empty() )
   {
      return &contacts.front();
   }
   return NULL;
}

int
RendDlgReg::getRspExpire(const resip::SipMessage& msg)
{
   const resip::NameAddr* mycontact = getMyContact(msg);
   if ( mycontact==NULL )
      return -1;
   if ( ! mycontact->exists(resip::p_expires) )
      return -2;
   return mycontact->param(resip::p_expires);
}


/********************************************************************
 *
 * REGISTER sketch specific classes
 *
 ********************************************************************/

class RendReg1Sketch : public RendSketchBase 
{
public:
   RendReg1Sketch(RendTu& tu, RendCntMgr& cntMgr, RendTroopReg*tr) 
      : RendSketchBase(tu, cntMgr) 
   {
      mTroopList.push_back(tr);
   }
   // virtual const char*	getName() const { return "Reg1"; }

   // virtual void	cleanup();
};



extern struct poptOption TheReg1OptTbl[];
// above, not really extern but otherwise compiler wants to know size

class RendReg1Options : public RendOptsBase 
{
public:
   RendReg1Options() : mWorkVol(1, 0.0, 1.0, 5) 
   {
      mAcctBase = 0;
      mAcctLen = 0; // default to all available in AcctMgr
      mAcctStride = 1;
      mRegRepeatBase = 15;
      mRegRepeatLen = 1;
      mRegExpireSecs = 10*60;
   }
   virtual struct poptOption* getPoptTbl() { return TheReg1OptTbl; }
   virtual const char*		getPoptDesc() { return "Reg1Options"; };

   int mAcctBase;
   int mAcctLen;
   int mAcctStride;
   int mRegRepeatBase;
   int mRegRepeatLen;

   int mRegExpireSecs;

   /*
   * Below here can be changed dynamically
   **/
   RendWorkVolume mWorkVol;
};


class RendReg1SketchFactory : public RendSketchFactory 
{
public:
   RendReg1SketchFactory() { };
   virtual const char* getName() const { return "Reg1"; }
   virtual RendOptsBase* getOpts() { return &mOpts; }
   virtual RendSketchBase* makeSketch(RendTu& tu, RendCntMgr& cntMgr);

   // virtual void	setWorkVolume(RendWorkVolume& vol);

   RendReg1Options	mOpts;
};

static RendReg1SketchFactory TheReg1SketchFactory;
RendSketchFactory* TheRendReg1SketchFacPtr = &TheReg1SketchFactory;

RendSketchBase* 
RendReg1SketchFactory::makeSketch(RendTu& tu, RendCntMgr &cntMgr) 
{ 
   RendTimeUs now = RendGetTimeUsRel();
   RendTroopReg* tr = new RendTroopReg(now, tu, cntMgr);
   int numDlgs = mOpts.mAcctLen>0 ? mOpts.mAcctLen : tu.getAcctMgr().getNumAccts();

   tr->setVectorDlgs(numDlgs, mOpts.mAcctBase, numDlgs, mOpts.mAcctStride, mOpts.mRegRepeatBase, mOpts.mRegRepeatLen);
   // tr->setTgtOpenDlgs(mOpts.mWorkLevel);	// see volume set below
   tr->setExpires(mOpts.mRegExpireSecs);

   RendReg1Sketch* sketch = new RendReg1Sketch(tu, cntMgr, tr);
   sketch->setWorkVolume(mOpts.mWorkVol);
   return sketch;
}


#define OPTOBJ (TheReg1SketchFactory.mOpts)
struct poptOption TheReg1OptTbl[] = 
{
   { "regacctbase", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mAcctBase, 0, "First account index" },
   { "regacctlen", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mAcctLen, 0, "Number of accounts" },
   { "regrepeatbase", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mRegRepeatLen, 0, "Repeat base"},
   { "regrepeatlen", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mRegRepeatLen, 0, "Repeat factor"},
   { "regexpire", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mRegExpireSecs, 0, "Expires"},
   { "level", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkLevel, 0, "Target number of open REGISTER dialogs"},
   { "minrate", 'r', POPT_ARG_FLOAT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkRateMin, 0, "Target minimum change REGISTER rate"},
   { "maxrate", 'R', POPT_ARG_FLOAT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkRateMax, 0, "Target maximum change REGISTER rate"},
   { "maxpend", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPTOBJ.mWorkVol.mWorkPendCntMax, 0, "Maximum count of pending work"},
   { NULL, 0, 0, NULL, 0 }
};

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
