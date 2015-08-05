/**
  * REND Dialog and Transaction User classes:
  * 	RendDlg
  * 	RendTu
  *
**/

#ifndef WIN32
#include <unistd.h>
#endif

#include <stdarg.h>

#include <iostream>
#include <map>
#include <string>
#include <sstream>

#include "rutil/Logger.hxx"
#include "rutil/SharedPtr.hxx"
#include "rutil/DnsUtil.hxx"

#include "resip/stack/Message.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/DeprecatedDialog.hxx"
#include "resip/stack/StackThread.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/ShutdownMessage.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/KeepAliveMessage.hxx"
#include "resip/stack/ConnectionTerminated.hxx"

#include "RendAcct.hxx"
#include "RendKa.hxx"
#include "RendDlg.hxx"

#include "popt.h"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::APP


#define REND_DlgTmr_KeepAlive	1
#define REND_DlgTmr_Renew	2
#define REND_DlgTmr_TuStats	3

// #define REND_UseForceTransport

/*************************************************************************
 *
 * RendDlgTimer,RendTimerQueue
 *
 *************************************************************************/

struct RendDlgTimer 
{
   RendDlgTimer(RendLocalKey key, int which, UInt64 when)
      : mLocalKey(key), mWhich(which), mWhen(when) { }
   RendLocalKey mLocalKey;
   int mWhich;
   UInt64 mWhen;
};

bool operator<(const RendDlgTimer& a, const RendDlgTimer& b)
{
    return a.mWhen < b.mWhen;
}

typedef std::multiset<RendDlgTimer> RendDlgTimerQueue;

/**
    Trick is that we never modify or delete timers. Our only purpose
    is to "wakeup" the dialog. It is up to the dialog to "know" if the
    timer is still valid or not. If dialog goes away, the LocalKey
    is invalidated within its lookup map.
**/
class RendTimerQueue 
{
public:
   // addTmr(RendDlgTimer& tmr);
   void addAbsUs(RendLocalKey key, int which, RendTimeUs when);
   void addRelUs(RendLocalKey key, int which, RendTimeUs ofs);
   void addRelSecs(RendLocalKey key, int which, unsigned ofs);

   bool getNext(RendTimeUs now, RendLocalKey &key, int &which);

   RendDlgTimerQueue	mQueue;
};

void
RendTimerQueue::addAbsUs(RendLocalKey key, int which, RendTimeUs when)
{
   RendDlgTimer tmr(key, which, when);
   mQueue.insert(tmr);
}

void
RendTimerQueue::addRelUs(RendLocalKey key, int which, RendTimeUs ofs) 
{
   if ( ofs <= 0 )
      return;
   RendTimeUs when = ofs + RendGetTimeUsRel();
   addAbsUs(key, which, when);
}


void
RendTimerQueue::addRelSecs(RendLocalKey key, int which, unsigned ofs)
{
   if ( ofs <= 0 )
      return;
   RendTimeUs when = ofs*1000*1000 + RendGetTimeUsRel();
   addAbsUs(key, which, when);
}

bool
RendTimerQueue::getNext(RendTimeUs now, RendLocalKey &key, int &which)
{
   if ( mQueue.empty() )
      return false;

   RendDlgTimerQueue::iterator it = mQueue.begin();
   RendDlgTimer tmr = *it;
   if ( tmr.mWhen > now )
      return false;
   key = tmr.mLocalKey;
   which = tmr.mWhich;
   mQueue.erase(it);
   //std::cout << "got timer which="<<which<<" when="<<tmr.mWhen
   //  <<" now="<<now<<std::endl;
   return true;
}

/*************************************************************************
 *
 * RendDlg
 *
 *************************************************************************/


RendDlg::RendDlg(RendTu& tu, int fromAcctIdx, int toAcctIdx) :
  mTu(tu),
  mFromAcctIdx(fromAcctIdx),
  mToAcctIdx(toAcctIdx),
  mLocalCSeqReq(0),mLocalCSeqRsp(0),
  mRemoteCSeq(0) 
{
   mFlags = 0;
   mTransport = NULL;
   mState = REND_DS_None;
   mCntAuthFail = 0;
   mRemoteCSeqFull = false;
   mExpireSecs = -1;
   mKeepAliveKey = 0;
   mLocalKeyIdx = 0;
}

RendDlg::~RendDlg() 
{
}

RendAcctMgrIf&
RendDlg::getAcctMgr() 
{ 
   return mTu.getAcctMgr();
}

void
RendDlg::buildAndRegLocalTag(RendDlgSharedPtr& dlgPtr) 
{
   // verify idle dialog
   resip_assert( mState == REND_DS_None );

   if ( dlgPtr.get() != this ) 
   {
      dlgPtr.reset(this);
   }
   resip_assert( mLocalKeyIdx==0 );	// verify previously released
   // below updates mLocalTag
   mLocalKeyIdx = mTu.buildLocalKey( mLocalTag);
   // below, Tu makes copy of shared pointer (to get shared count)
   mTu.registerDlg(mLocalKeyIdx, dlgPtr);
   mState = REND_DS_Bound;
}

void
RendDlg::relDlgState()
{
   if ( mKeepAliveKey ) 
   {
      mTu.mKeepAliveMgr->delConn(mKeepAliveKey);
      mKeepAliveKey = 0;
   }
   if ( mLocalKeyIdx ) 
   {
      mTu.unregisterDlg(mLocalKeyIdx);
      mLocalKeyIdx = 0;
   }

   /* transport state */
   mTransport = NULL;
   /* Tu-state */
   mCallId.clear();
   mLocalTag.clear();
   mRemoteTag.clear();
   mRouteSet.clear();
   mRemoteETag.clear();
   mRemoteCSeqFull = false;
   mLocalCSeqReq = mLocalCSeqRsp = 0;
   mRemoteTarget = resip::Tuple();

   mExpireSecs = -1;

   mCmdRetryCnt = 0;	/* is this still used? */
   mLastRspCode = 0;
   mLastRspReason.clear();
   mLastRspWarnings.clear();

   mCntAuthFail = 0;
   // leave AuthState.mNonce alone

   mRemoteContact.clear();	/* maybe leave alone? */
   mState = REND_DS_None;
}

/**
    Add an auth line to the request based upon rspCode&inAuth.
    Returns 1 if able to add auth.
**/
int
RendDlg::addAuthCore(resip::SipMessage *req, int rspCode, const resip::Auth& inAuth)
{
   // Someday, could add cnonce and nonceCount to AuthState
   // to support qop=auth
   // For now, just leave empty
   resip::Data cnonce, nonceCountStr, authQop;

   // Someday extract realm from challenge and keep as state
   // to support multiple proxies.
   // For now, just leave empty
   resip::Data realm;

   resip::Data userVal, passVal;
   getAcctMgr().getAuth( mFromAcctIdx, inAuth.param(resip::p_realm), userVal, passVal);

   resip::Auth outAuth;
   resip::Helper::makeChallengeResponseAuth(*req, userVal, passVal,
                                            inAuth, cnonce, authQop, 
                                            nonceCountStr, outAuth);
   if ( rspCode==401 ) 
   {
      req->header(resip::h_Authorizations).push_back(outAuth);
   } 
   else 
   {
      req->header(resip::h_ProxyAuthorizations).push_back(outAuth);
   }
   return 1;

   // InfoLog(<< "Adding auth: user=" << *userVal << " pass="<<*passVal);
   // resip::Helper::addAuthorization(*nextReq, *rsp, userVal, passVal,
   //  cnonce, nonceCount);
}

/**
    Called when sending a "new" request and we want to try and
    anticipate the challenge using cached state.
    Returns:
      1 if sucessfully added authorization
      0 didn't add auth b/c no cached state
**/
int
RendDlg::addAuthFromCache(resip::SipMessage *req) 
{
   if ( mAuthState.mNonce.empty() )
      return 0;
   resip::Auth inChal;
   inChal.param(resip::p_realm) = getAcctMgr().getDefaultRealm();
   inChal.param(resip::p_nonce) = mAuthState.mNonce;
   return addAuthCore(req, mAuthState.mRspCode, inChal);
}

/**
    Called when re-sending an "old" request in response to a 401/407
    response.
    XXX: this only handles a single challenge, and only the www(401) type
**/
int
RendDlg::addAuthFromRsp(resip::SipMessage *req, const resip::SipMessage& rsp) 
{
   resip_assert(rsp.isResponse());
   int rspCode = rsp.header(resip::h_StatusLine).responseCode();
   resip_assert (rspCode==/*www*/401 || rspCode==/*proxy*/407);
   if(rsp.exists(resip::h_WWWAuthenticates))
   {
      const resip::ParserContainer<resip::Auth>& auths = rsp.header(resip::h_WWWAuthenticates);
      resip_assert( auths.size()== 1);
      const resip::Auth &au = auths.front();
      //const resip::Data& realm = au.param(resip::p_realm); // use/check this?
      const resip::Data& nonce = au.param(resip::p_nonce);
      // XXX: make sure qop is missing, etc.
      mAuthState.update(rspCode, nonce);
      return addAuthCore(req, rspCode, au);
   }
   else if(rsp.exists(resip::h_ProxyAuthenticates))
   {
      const resip::ParserContainer<resip::Auth>& auths = rsp.header(resip::h_ProxyAuthenticates);
      resip_assert( auths.size()== 1);
      const resip::Auth &au = auths.front();
      //const resip::Data& realm = au.param(resip::p_realm); // use/check this?
      const resip::Data& nonce = au.param(resip::p_nonce);
      // XXX: make sure qop is missing, etc.
      mAuthState.update(rspCode, nonce);
      return addAuthCore(req, rspCode, au);
   }
   return 0;
}

resip::SipMessage*
RendDlg::makeRequest(resip::MethodTypes method)
{
   resip::SipMessage *req = new resip::SipMessage;

   resip_assert( mState != REND_DS_None );

   if ( mState == REND_DS_Bound ) 
   {
      mState = REND_DS_Opening;
      mTransport = mTu.getNextTransport();
      mCallId = resip::Helper::computeCallId();
   }

   resip_assert( mState==REND_DS_Opening || mState==REND_DS_Established || mState==REND_DS_Closing);

   resip_assert( mLocalCSeqReq == mLocalCSeqRsp ); // last response hasn't finished yet
   RendAcctMgrIf& acctMgr = getAcctMgr();
   resip::RequestLine rLine(method);
   const resip::NameAddr& toNA = acctMgr.getNameAddrAor(mToAcctIdx);
   if ( method==resip::REGISTER ) 
   {
      // not sure if better to copy and clear, or just copy
      // what we want
      rLine.uri() = toNA.uri();
      rLine.uri().user().clear();
   }
   else if ( mState==REND_DS_Opening || mRemoteContact.empty() ) 
   {
      rLine.uri() = toNA.uri();
   } 
   else 
   {
      // XXX: how is memory managed below?
      resip::Uri ruri(mRemoteContact);
      rLine.uri() = ruri;
   }
   req->header(resip::h_RequestLine) = rLine;
   req->header(resip::h_To) = toNA;
   if ( (mFlags&REND_DlgF_StdDlg)!=0 && !mRemoteTag.empty() )
   {
      req->header(resip::h_To).param(resip::p_tag) = mRemoteTag;
   }
   const resip::NameAddr& fromNA = acctMgr.getNameAddrAor(mFromAcctIdx);
   req->header(resip::h_From) = fromNA;
   req->header(resip::h_From).param(resip::p_tag) = mLocalTag;
   req->header(resip::h_CallId).value() = mCallId;
   req->header(resip::h_MaxForwards).value() = 70;
   req->header(resip::h_CSeq).method() = method;
   mLastReqMethod = method;
   req->header(resip::h_CSeq).sequence() = ++mLocalCSeqReq;
   if ( !mRouteSet.empty() ) 
   {
      RendDataList::iterator it;
      for (it=mRouteSet.begin(); it!=mRouteSet.end(); it++) 
      {
         resip::Uri rsu(*it);
         resip::NameAddr rsna(rsu);
         req->header(resip::h_Routes).push_back( rsna);
      }
   }
   if ( (mFlags&REND_DlgF_ETagDlg)!=0 && !mRemoteETag.empty() )
   {
      req->header(resip::h_SIPIfMatch).value() = mRemoteETag;
   }

   resip::NameAddr localContact;
   localContact.uri().host() = mTransport->interfaceName();
   localContact.uri().port() = mTransport->port();
   localContact.uri().user() = fromNA.uri().user();
   if ( mTu.mOutboundMode && 
       (method==resip::SUBSCRIBE || method==resip::REGISTER) ) 
   {
      (void) localContact.uri().param(resip::p_ob);
   }
   req->header(resip::h_Contacts).push_front( localContact );

   if ( mExpireSecs >= 0 ) 
   {
      /* could make this also conditional on flag */
      req->header(resip::h_Expires).value() = mExpireSecs;

      /* real expire is set upon response, this is just upper bound */
      // KILL mExpireAbsTime = time(NULL) + mExpireSecs;
      if ( mExpireSecs == 0 && mState == REND_DS_Established ) 
      {
         mState = REND_DS_Closing;
      }
   }

   resip::Via via;
   req->header(resip::h_Vias).push_front(via);

   /* TBD: Add user-agent */

   return req;
}

int
RendDlg::sendNextCmd(int expireSecs, int maxRetryReqCnt) 
{
   resip_assert( mState != REND_DS_None );
   if ( mLocalCSeqReq!=mLocalCSeqRsp )
   {
      return REND_Sts_Busy;
   }

   setExpireSecs(expireSecs);
   std::auto_ptr<resip::SipMessage> msg(makeNextReq());
   // XXX: do something with maxRetryReqCnt
   mCmdRetryCnt = 0;
   addAuthFromCache(msg.get());
   mReqSendTime = RendGetTimeUsRel();
   getTu().sendMsg(msg, this);
   return 1;
}

int
RendDlg::handleRequest(RendReqCxt& cxt) 
{
   getTu().badRequest( cxt.mMsg, 500, "Stray or unimplemented", "unhandled request for this dialog type");
   return REND_PRA_RspSent;
}

int
RendDlg::badResponse( const resip::SipMessage *rsp, const char *why) 
{
   // XXX: add info about our dialog
   WarningLog(<<"Bad response: "<<why);
   return -1;
}

/**
    Called upon getting 2xx response while opening new dialog.
    We "latch" certain state into the dialog.
**/
int
RendDlg::latchEstablish(RendTimeUs now, const resip::SipMessage *rsp, 
                        const resip::Data& remoteTag)
{
   if ( rsp->exists(resip::h_Contacts)
      && !rsp->header(resip::h_Contacts).empty() ) 
   {
      const resip::NameAddr& cna 
         = rsp->header(resip::h_Contacts).front();
      resip::Data cda = cna.uri().toString();
      mRemoteContact = cda;
   }
   if ( (mFlags&REND_DlgF_StdDlg)!=0 )
   {
      mRemoteTag = remoteTag;
      if ( rsp->exists(resip::h_RecordRoutes) ) 
      {
         resip::NameAddrs rs = rsp->header(resip::h_RecordRoutes);
         resip::ParserContainer<resip::NameAddr>::iterator it;
         for (it=rs.begin(); it!=rs.end(); it++) 
         {
            resip::NameAddr rsna = *it;
            // note list is reversed by "push_front"
            mRouteSet.push_front( rsna.uri().toString() );
         }
      }
   }
   // Save this away, primarily for the flow key. Note that this is used
   // for all types of dialogs (not just StdDlg). It especially happens
   // for REGISTER in outbound mode.
   mRemoteTarget = rsp->getSource();
   // check incoming Transport against outgoing. If different,
   // then something went very wrong.
   resip_assert( mRemoteTarget.mTransportKey != 0 );
   if ( mRemoteTarget.mTransportKey != mTransport->getKey() ) 
   {
      CritLog(<<"Response from different transport than request:"
         << " req-tp=[" << mTransport->getKey() << "]"
         << " rsp-tp=["<< mRemoteTarget.mTransportKey << "]");
      resip_assert(0);
   }
   if ( mTu.mOutboundMode ) 
   {
      mRemoteTarget.onlyUseExistingConnection = true;
   }
   if ( (mFlags&REND_DlgF_KeepAlive)!=0 ) 
   {
      /* XXX: don't do this if expires==0 */
      /* XXX: but if expires==0, then shouldn't be latching! */
      resip_assert( mKeepAliveKey == 0 );
      mKeepAliveKey = mTu.mKeepAliveMgr->addConn(now, mRemoteTarget);
   }
   return 0;
}

int
RendDlg::processResponse( RendTimeUs now, const resip::SipMessage *rsp)
{
   int sts;

   /* Caller has already verified that To,From,CallId,CSeq exist */
   if ( mState == REND_DS_None )
   {
      return badResponse(rsp, "dialog not started yet");
   }
   if ( mState == REND_DS_Closed )
   {
      return badResponse(rsp, "dialog already terminated");
   }

   resip::Data cid = rsp->header(resip::h_CallId).value();
   if ( cid != mCallId ) 
   {
      return badResponse(rsp, "callid mismatch");
   }

   unsigned long rspCSeq = rsp->header(resip::h_CSeq).sequence();
   if ( rspCSeq != mLocalCSeqReq )
   {
      return badResponse(rsp, "cseq number mismatch");
   }
   resip::MethodTypes rspMeth = rsp->header(resip::h_CSeq).method();
   if ( rspMeth != mLastReqMethod )
   {
      return badResponse(rsp, "cseq method mismatch");
   }

   int rspCode = rsp->header(resip::h_StatusLine).responseCode();
   mLastRspCode = rspCode;
   mLastRspReason = rsp->header(resip::h_StatusLine).reason();
   mLastRspWarnings.clear();
   if ( rsp->exists(resip::h_Warnings) ) 
   {
      // for now, just get the first one, and only the text
      const resip::WarningCategory& warn = rsp->header(resip::h_Warnings).front();
      mLastRspWarnings = warn.hostname();
      mLastRspWarnings += ':';
      mLastRspWarnings += warn.text();
   }
   mLocalCSeqRsp = rspCSeq;
   if ( rspCode==401 || rspCode==407 ) 
   {
      // InfoLog(<< "Processing auth challenge: " << rspMeth << rspCSeq);
      if ( ++(mCntAuthFail) >= 2 ) 
      {
         WarningLog(<< "Auth failed (too many retries):" 
            << getMethodName(rspMeth) <<" "<< rspCSeq);
         handleResponse(now, rsp, "auth failed");
         return -2;
      }

      std::auto_ptr<resip::SipMessage> nextReq(makeNextReq());
      if ( addAuthFromRsp(nextReq.get(), *rsp) < 0 ) 
      {
         WarningLog(<< "Auth failed (unable to respond):" 
            << getMethodName(rspMeth) <<" "<< rspCSeq);
         handleResponse(now, rsp, "auth failed");
         return -2;
      }
      mReqSendTime = RendGetTimeUsRel();	// not sure about this
      mTu.sendMsg(nextReq, this);
   } 
   else if(rspCode>=100 && rspCode < 199) 
   {
      ; // could mark as early or such, ...
   } 
   else if ( rspCode>=200 && rspCode < 299 ) 
   {
      mCntAuthFail = 0;
      resip::Data remoteTag = rsp->header(resip::h_To).param(resip::p_tag);
      if ( remoteTag.empty() )
      {
         return badResponse(rsp, "Empty to-tag");
      }
      mRspRecvTime = now;
      if ( mState==REND_DS_Opening ) 
      {
         if ( (sts=latchEstablish( now, rsp, remoteTag)) < 0 )
         {
            return sts;
         }
         mState = REND_DS_Established;
      } 
      else if ( mState==REND_DS_Established) 
      {
         if ((mFlags&REND_DlgF_StdDlg)!=0 && remoteTag != mRemoteTag)
         {
            return badResponse(rsp, "Changed to-tag");
         }
      } 
      else if ( mState==REND_DS_Closing ) 
      {
         /* XXX: could check Expires to confirm 0, but why? */
         mState = REND_DS_Closed;
      }
      if ( (mState==REND_DS_Opening || mState==REND_DS_Established)
         && (mFlags&REND_DlgF_ETagDlg)!=0 ) 
      {
         resip::Data etag;
         if ( rsp->exists(resip::h_SIPETag) )
         {
            etag = rsp->header(resip::h_SIPETag).value();
         }
         if ( etag.empty() )
         {
            return badResponse(rsp, "Missing ETag");
         }
         mRemoteETag = etag;
      }
      if ( (mState==REND_DS_Opening || mState==REND_DS_Established)
         && (mFlags&REND_DlgF_StdDlg)!=0 ) 
      {
         if ( rsp->exists(resip::h_Expires) ) 
         {
            int newExp = rsp->header(resip::h_Expires).value();
            if ( newExp <= 0 ) 
            {
               return badResponse(rsp, "Unexpected zero expires");
            }
         }
      }
      handleResponse(now, rsp);
      // end of 2xx processing
   } 
   else 
   {
      handleResponse(now, rsp, "non-2xx");
   }
   return 0;
}

void
RendDlg::handleConnTerm(RendTimeUs now) 
{
}

int
RendDlg::getRspState(int *rttMs) 
{
   if (mLocalCSeqReq!=mLocalCSeqRsp)
   {
      return 0;
   }
   
   if (mLastRspCode/100 == 2 && rttMs) 
   {
      *rttMs = ((int)(mRspRecvTime - mReqSendTime))/1000;
      resip_assert( *rttMs >= 0 );  // can be zero on fast machines
   }
   resip_assert( mLastRspCode > 0 );
   return mLastRspCode;
}

/**********************************************************************
 *
 * RendTu
 *
 **********************************************************************/

RendTu::RendTu(resip::SipStack& stack, RendAcctMgrIf& acctMgr, int keepAliveIvalSecs) :
   resip::TransactionUser(DoNotRegisterForTransactionTermination,
   RegisterForConnectionTermination),
   mName("RendTu"),
   mStack(stack),
   mAcctMgr(acctMgr)
{
   mDlgTagSeq = REND_LocalKey_MinVal;
   mCurTransportIdx = 0;
   mOutboundMode = false;

   mKeepAliveMgr = RendKaMgrIf::createMgr(stack, keepAliveIvalSecs);

   RendTimeUs now = RendGetTimeUsRel();
   mTimers = new RendTimerQueue();
   processTuStatsTimeout(now);
}

RendTu::~RendTu() 
{ 
   int numDialogs = (int)mDialogMap.size();
   if ( numDialogs > 0 ) 
   {
      CritLog(<<"TransactionUser destruction while dialogs still alive: cnt="
         <<numDialogs);
   }
   resip_assert( numDialogs == 0);

   if ( mKeepAliveMgr ) 
   {
      // NOTE: All keep-alive connections must be deleted prior
      // to deleting the KeepAliveMgr. In practice, this means
      // all dialogs w/keep-lives need to be shutdown
      delete mKeepAliveMgr; mKeepAliveMgr = NULL;
   }
   if ( mTimers ) 
   {
      delete mTimers; mTimers = NULL;
   }
}

void
RendTu::setProxy(const resip::Uri &uri) 
{
   mOutboundMode = false;
   mOutboundProxy = uri;
   if ( mOutboundProxy.exists(resip::p_ob) ) 
   {
      mOutboundProxy.remove(resip::p_ob);
      mOutboundMode = true;
   }
}

void
RendTu::addTransportVec( resip::TransportType proto, resip::IpVersion vers,
  const resip::Data& ipInterface, resip::StunSetting stun, int transportFlags,
  int portBase, int numPorts) 
{
   InfoLog(<< "Adding transport proto="<<resip::getTransportNameFromType(proto)
      <<" if="<<ipInterface << ":"<<portBase<<"x"<<numPorts);

   int idx;
   for (idx=0; idx < numPorts; idx++) 
   {
      resip::Transport* tran = mStack.addTransport( proto, portBase+idx, 
         vers, stun, ipInterface, 
         /*tlsDomain*/resip::Data::Empty, /*tlsKey*/resip::Data::Empty,
         /*tlsType*/resip::SecurityTypes::TLSv1,
         transportFlags);
      mTransports.push_back(tran);
   }
}

resip::Transport*
RendTu::getNextTransport() 
{
   unsigned sz;
   if ((sz=(unsigned)mTransports.size()) == 0)
   {
      return NULL;
   }
   return mTransports.at( mCurTransportIdx++ % sz);
}

/**
    Build a local tag.
    The tag consists of an increment index number (the local key)
    and a suffix that is unique to this instance of app. (So that
    we can easily identify messages that below to "earlier" instances
    of us.)

    Note that the local key is used to route a particular message
    to its dialog. We later check the CallId and totag, but only
    the local key is used to route the message up.
    
    According to RFC 3261 Sect19.3, the tag should be
    (a) globally unique, and (b) be cryptographically random with
    at least 32 bits of randomness. Both of these seems entirely
    redundant with the callid.
**/
RendLocalKey
RendTu::buildLocalKey( resip::Data& localTag) 
{
   if ( mLocalTagSuffix.empty() ) 
   {
      RendTimeUs curTime = RendGetTimeUsRel();
      mLocalTagSuffix = resip::Data((char*)&curTime, sizeof(curTime)).hex();
   }
   RendLocalKey dlgTag = ++mDlgTagSeq;
   unsigned sufsz = (unsigned)mLocalTagSuffix.size();
   char *buf = localTag.getBuf(8+sufsz);
   resip::Helper::integer2hex(buf, dlgTag);
   memcpy(buf+8, mLocalTagSuffix.data(), sufsz);
   return dlgTag;
}

void
RendTu::registerDlg(RendLocalKey key, RendDlgSharedPtr& dlg) 
{
   mDialogMap[key] = dlg;
}

void
RendTu::unregisterDlg(RendLocalKey key)
{
   RendDialogMap::iterator it;
   if ( (it = mDialogMap.find(key)) != mDialogMap.end() )
      mDialogMap.erase(it);
}

void
RendTu::processConnTerm(RendTimeUs now, const resip::Tuple& flow)
{
   int cnt = 0;
   RendDialogMap::iterator it = mDialogMap.begin();
   for ( ; it != mDialogMap.end(); ++it) {
      RendDlgSharedPtr dlgptr = it->second;
      RendDlg *dlg = dlgptr.get();
      resip_assert( dlg );
      if ( dlg->mRemoteTarget.onlyUseExistingConnection
         && dlg->mRemoteTarget.mFlowKey==flow.mFlowKey
         && dlg->mRemoteTarget==flow ) {
            // WATCHOUT above: comparing flow key alone isn't good enough
            // (1) transport might be different, (2) flow key (fd) might
            // might be re-used
            ++cnt;
            dlg->handleConnTerm(now);
      }
   }
   // XXX: do something with count
}

RendDlgSharedPtr
RendTu::lookupDlg(RendLocalKey localKey)
{
   RendDialogMap::iterator j = mDialogMap.find(localKey);
   if ( j==mDialogMap.end() )
      return RendDlgSharedPtr();
   return j->second;
}

RendDlgSharedPtr
RendTu::lookupDlg(const resip::Data& localTag, const char **badDetail)
{
   *badDetail = NULL;
   unsigned sufsz = (unsigned)mLocalTagSuffix.size();
   if ( localTag.size() != 8+sufsz ) 
   {
      *badDetail = localTag.size()==0 ? "empty" : "size";
      return RendDlgSharedPtr();
   }
   if ( memcmp( localTag.data()+8, mLocalTagSuffix.data(), sufsz)!=0 ) 
   {
      *badDetail = "suffix";
      return RendDlgSharedPtr();
   }
   RendLocalKey tagInt = resip::Helper::hex2integer(localTag.data());
   if ( tagInt < REND_LocalKey_MinVal ) 
   {
      *badDetail = "keymin";
      return RendDlgSharedPtr();
   }

   *badDetail = "keyval";
   return lookupDlg(tagInt);
}

// extern resip::Transport *RendTransports[];

void
RendTu::sendMsg(std::auto_ptr<resip::SipMessage> msg, RendDlg *dlg)
{
   if ( msg->isRequest() ) 
   {
      if ( dlg && dlg->mRemoteTarget.mTransportKey != 0 ) 
      {
         // this handles outbound flow routing
         msg->setDestination(dlg->mRemoteTarget);
      } 
      else if ( dlg && !dlg->mRemoteTag.empty() ) 
      {
         // stdDlg and in-dialog: use record-route/RURI
         ; // this is default behavior
      } 
      else if ( !mOutboundProxy.host().empty() ) 
      {
         msg->setForceTarget(mOutboundProxy);
      } 
      else 
      {
         ; // standard routing: use record-route/RURI
      }

      if ( dlg!=NULL && dlg->mTransport ) 
      {
         if ( msg->getDestination().mTransportKey != 0 ) 
         {
            resip_assert( msg->getDestination().mTransportKey == dlg->mTransport->getKey() );
         } 
         else 
         {
            // below make use of "mistake" in SipMessage that
            // getDestination returns non-const reference so
            // we can use to tweak the private member mDestination
            msg->getDestination().mTransportKey = dlg->mTransport->getKey();
         }
         // Strictly, this isn't always require (if we have
         // set a flow above)
         // makeRequest creates empty Via, could do this work there
         // const resip::Tuple& tt = dlg->mTransport->
         // std::cout << "Sending from "<<dlg->mTransport->interfaceName()<<std::endl;
         msg->header(resip::h_Vias).front().sentHost() = dlg->mTransport->interfaceName();
         msg->header(resip::h_Vias).front().sentPort() = dlg->mTransport->port();
      }
      ++(mStats.mTransTxReqCnt);
   } 
   else 
   {
      ++(mStats.mTransTxRspCnt);
   }

#if 0
   if ( dlg!=NULL && mTransports.size() > 0 ) 
   {
      // int tOfs = dlg->mFromAcctIdx + 
      int tIdx = dlg->mFromAcctIdx % mTransports.size();
      resip::Transport* trans = mTransports.at(tIdx);
      std::cout << "Using transport "<<tIdx<<" of "<<mTransports.size()<<
         ": if="<<trans->interfaceName()<<":"<<trans->port()<<"\n";
      msg->setForceTransport(trans);
   }
#endif

   mStack.send(msg, this);
}

int /*REND_PRA_... */
RendTu::badRequest( const resip::SipMessage *req, int rspCode, 
  const char *reason, const char *detailWhy, const char *why2) 
{
   WarningLog(<<"Processed bad request: "
      <<" m="<<getMethodName(req->header(resip::h_RequestLine).getMethod())
      <<" code="<<rspCode
      <<" reason="<<reason
      <<", why="<<detailWhy
      <<", why2="<<(why2?why2:"na"));

   std::auto_ptr<resip::SipMessage> rsp(new resip::SipMessage);
   resip::Helper::makeResponse(*rsp, *req, rspCode, resip::Data(reason));
   sendMsg(rsp);
   return REND_PRA_RspSent;
}

/**
    Caller must free the request
**/
int
RendTu::processRequest( RendTimeUs now, const resip::SipMessage *req) 
{
   const resip::RequestLine& rLine = req->header(resip::h_RequestLine);
   resip::MethodTypes method = rLine.getMethod();
   resip::MethodTypes reqMeth = req->header(resip::h_CSeq).method();
   unsigned long reqCSeq = req->header(resip::h_CSeq).sequence();

   ++mStats.mTransRxReqCnt;
   if (method != reqMeth)
   {
      return badRequest(req, 400, "method mismatch", "rline-vs-cseq");
   }

   if (!req->header(resip::h_To).exists(resip::p_tag))
   {
      return badRequest(req, 481, "Missing totag for dialog", "no totag");
   }

   const resip::Data& localTag = req->header(resip::h_To).param(resip::p_tag);
   const char *badDetail = NULL;
   RendDlgSharedPtr dlg = lookupDlg(localTag, &badDetail);

   if ( dlg.get() == NULL ) 
   {
      // This only really applies to NOTIFY
      return badRequest(req, 481, "Dialog does not exist", "unmatched localTag(to-tag)", badDetail);
   }

   resip::Data cid = req->header(resip::h_CallId).value();
   if (cid != dlg->mCallId)
   {
      // Only applies to existing dialogs
      return badRequest(req, 481, "Wrong CallId/localTag", "callid doesnt match localTag");
   }

   /* XXX: update dialog state for NOTIFY incase it arrives before SUB reply */

   bool outOfOrder = dlg->mRemoteCSeqFull && reqCSeq <= dlg->mRemoteCSeq;
   if (!outOfOrder) 
   {
      dlg->mRemoteCSeq = reqCSeq;
   }

   dlg->mRemoteCSeqFull = true;
   RendReqCxt cxt(req, method);
   cxt.mOutOfOrder = outOfOrder;
   cxt.mRxTimeUs = now;
   int act = dlg->handleRequest(cxt);
   if (act!=REND_PRA_RspSent)
   {
      resip_assert ( act >= 100 && act <= 999 );
      std::auto_ptr<resip::SipMessage> rsp(new resip::SipMessage);
      resip::Helper::makeResponse(*rsp, *req, act);
      sendMsg(rsp);
      return REND_PRA_RspSent;
   }
   return act;
}

int
RendTu::badResponse( const resip::SipMessage *rsp, const char *why, const char *why2)
{
   WarningLog(<<"Bad response: "<< why << ", why2=" << (why2?why2:"na"));
   return -1;
}

int
RendTu::processResponse( RendTimeUs now, const resip::SipMessage *rsp)
{
   ++mStats.mTransRxRspCnt;
   if ( !rsp->exists(resip::h_From) 
      || !rsp->exists(resip::h_To)
      || !rsp->exists(resip::h_CallId)
      || !rsp->exists(resip::h_CSeq)
      ) 
   {
      // I suspect transaction layer has already checked this ... 
      return badResponse(rsp, "missing required header");
   }

   // XXX: check that to-tag and from-tag exist?
   const char *badDetail = NULL;
   const resip::Data& localTag = rsp->header(resip::h_From).param(resip::p_tag);
   RendDlgSharedPtr dlg = lookupDlg(localTag, &badDetail);
   if (dlg.get() == NULL)
   {
      return badResponse(rsp, "unmatched localTag(from-tag)", badDetail);
   }
   return dlg->processResponse(now, rsp);
}

/**
    Process one message, waiting at most {ms} for it to be ready.
    Return true if message processed, false if timed out.
**/
void
RendTu::processMsg(RendTimeUs now, resip::Message *msg)
{
   resip::SipMessage *sipmsg = dynamic_cast<resip::SipMessage*>(msg);
   if ( sipmsg ) 
   {
      if (sipmsg->isRequest()) 
      {
         processRequest(now, sipmsg);
      } 
      else if (sipmsg->isResponse()) 
      {
         processResponse(now, sipmsg);
      }
      delete sipmsg;
      return;
   }

   resip::ConnectionTerminated *termmsg = dynamic_cast<resip::ConnectionTerminated*>(msg);
   if ( termmsg )
   {
      processConnTerm(now, termmsg->getFlow());
      return;
   }

   resip::ShutdownMessage *shutmsg = dynamic_cast<resip::ShutdownMessage*>(msg);
   if (shutmsg)
   {
      WarningLog(<< "Got shutdown app message.");
      return;
   }

   WarningLog(<< "Unhandled message from stack: " << *msg);
   delete msg;
}

/**************************************************************************
 *
 *	RendTu: Transport & KeepAlive processing
 *
 **************************************************************************/

void
RendTu::processTuStatsTimeout(RendTimeUs now) 
{
   RendKaStats kaStats;
   mKeepAliveMgr->getStats(kaStats);
#if 0
   std::cout << "Stats:"
      <<" kaSentMsgCnt="<<kaStats.mSentMsgCnt
      <<" kaNumCurConn="<<kaStats.mNumCurConn
      <<" kaNumCurSession="<<kaStats.mNumCurSession
      <<std::endl;
#endif
   mTimers->addAbsUs( 0, REND_DlgTmr_TuStats, 
      now+60*1000000);
}


/**************************************************************************
 *
 * RendTu: Common-stuff
 *
 **************************************************************************/

int
RendTu::processTimers(RendTimeUs tmrNow) 
{
   RendLocalKey tmrKey;
   int tmrWhich;
   int cnt = 0;

   mKeepAliveMgr->processTimers(tmrNow);
   while ( mTimers->getNext( tmrNow, tmrKey, tmrWhich) )
   {
      ++cnt;
      if (tmrWhich == REND_DlgTmr_KeepAlive) 
      {
         resip_assert(0);
         // processKeepAliveTimeout(tmrNow, tmrKey);
         continue;
      }
      if ( tmrWhich == REND_DlgTmr_TuStats )
      {
         processTuStatsTimeout(tmrNow);
         continue;
      }
      RendDlgSharedPtr dlg = lookupDlg(tmrKey);
      if (dlg.get() == 0)
      {
         continue; // normal: dialog went away
      }
#if 0
      if ( tmrWhich==REND_DlgTmr_Renew ) 
      {
         dlg->processRenew(tmrNow);
         continue;
      }
#endif
      resip_assert( false );
   }
   return cnt;
}

/**
    Return true if process state changed (e.g., we did something)
    Return false if timeout reached
**/
bool
RendTu::processAll(int waitMS) 
{
   RendTimeUs startTimeMs = RendGetTimeUsRel()/1000;
   RendTimeUs endTimeMs = startTimeMs + waitMS;
   int cnt;
   bool didsomething = false;

   // first read off all pending messages, not worrying about wait times

   /* XXX: really need sub-sec resolution timers so that
   * the keep-alives spread out */
   for ( cnt=0; ; cnt++) 
   {
      RendTimeUs tmrNow;
      tmrNow = RendGetTimeUsRel();
      while (mFifo.messageAvailable()) 
      {
         // message is available, so waitMS doesn't matter
         resip::Message* msg = mFifo.getNext();
         resip_assert( msg );
         processMsg(tmrNow, msg);
         didsomething = true;
      }

      tmrNow = RendGetTimeUsRel();
      if (processTimers(tmrNow) > 0)
         continue;

      int remainMS = (int)endTimeMs - (long)(tmrNow/1000);
      if (remainMS <= 0)
      {
         break;
      }
      if (remainMS > 1000)
      {
         remainMS = 1000;
      }
      resip::Message* msg = mFifo.getNext(remainMS);
      if (msg) 
      {
         tmrNow = RendGetTimeUsRel();
         processMsg(tmrNow, msg);
         didsomething = true;
      }
      if (cnt>100*1000)
      {
         ErrLog(<<"Apparent infinite protocol loop. Breaking.");
         abort();
         break;
      }
   }
   return didsomething;
}


/**************************************************************************
 *
 *	RendTu Factory
 *
 **************************************************************************/

class RendTuOpts : public RendOptsBase
{
public:
   RendTuOpts() 
   {
      mProxy = NULL;
      mLocalTransportUri = NULL;
      mLocalNumPorts = 1;
      mLocalBind = 1;
      mKeepAliveIvalSecs = 20;
   }

   char* mProxy;
   char* mLocalTransportUri;
   int mLocalNumPorts;
   int mLocalBind;	// 1 to bind
   int mKeepAliveIvalSecs;

   virtual struct poptOption* getPoptTbl();
   virtual const char* getPoptDesc() { return "Transaction user options"; }

   void configureTu(RendTu *tu);
};

static RendTuOpts TheRendTuOpts;
RendOptsBase* TheRendTuOptsPtr = &TheRendTuOpts;

#define OPT TheRendTuOpts
static struct poptOption RendTuOptsTbl[] = 
{
   /* these should be Tu params */
   { "proxy", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mProxy, 0, "Outbound proxy URI" },
   { "localuri", 0, POPT_ARG_STRING|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mLocalTransportUri, 0, "Local URI (transports)" },
   { "localports", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mLocalNumPorts, 0, "Number of local ports (transports)" },
   { "localbind", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mLocalBind, 0, "Bind local ports" },
   { "kasecs", 0, POPT_ARG_INT|POPT_ARGFLAG_SHOW_DEFAULT, &OPT.mKeepAliveIvalSecs, 0, "Keep alive interval (secs)" },
   { NULL, 0, 0, NULL, 0 }
};

struct poptOption*
RendTuOpts::getPoptTbl() 
{ 
   return RendTuOptsTbl;
}

void
RendTuOpts::configureTu(RendTu *tu) 
{
   if ( mProxy && mProxy[0]!='\0' ) 
   {
      tu->setProxy(resip::Uri(mProxy));
   }

   {
      std::list<std::pair<resip::Data,resip::Data> > ifList = resip::DnsUtil::getInterfaces();
      std::list<std::pair<resip::Data,resip::Data> >::iterator it;
#if 0
      for (it = ifList.begin(); it !=ifList.end(); it++) 
      {
         std::cout << "INTERFACE: " << it->first << "," << it->second << "\n";
      }
#endif

      //resip::Data eth0Addr = resip::DnsUtil::getLocalIpAddress(resip::Data("eth0"));
      //std::cout << "ETH0Addr: " << eth0Addr << "\n";
   }

   if ( mLocalTransportUri && mLocalTransportUri[0]!='\0' ) 
   {
      const char *uriStr = mLocalTransportUri;
      resip::Uri localUri = resip::Uri(uriStr);
      resip::Data ipHost = localUri.host();
      resip::TransportType proto = resip::UDP;
      resip::IpVersion ipVers = resip::V4;
      resip::StunSetting stunMode = resip::StunDisabled;
      int portBase = localUri.port();
      int numPorts = mLocalNumPorts;
      int transFlags = 0;

      if(mLocalBind == 0)
      {
         transFlags = RESIP_TRANSPORT_FLAG_NOBIND;
      }

      if (localUri.exists(resip::p_transport))
      {
         resip::Data tpstr = localUri.param(resip::p_transport).uppercase();
         proto = resip::toTransportType(tpstr);
         if (proto == 0)
         {
            std::cerr << "Bad transport type \""<<tpstr<<"\" in localuri: \"" << uriStr << "\"\n";
            exit(1);
         }
      }
      if (ipHost.empty()) 
      {
         ; // this is ok, means bind INADDRANY (I think)
      } 
      else if ( !resip::DnsUtil::isIpAddress(ipHost) ) 
      {
         try 
         {
            // this converts using interface name e.g., eth1
            resip::Data ipByIf = resip::DnsUtil::getLocalIpAddress(ipHost);
            ipHost = ipByIf;
         } 
         catch (resip::DnsUtil::Exception&) 
         {
            ;
         }
         // XXX: add gethostbyname lookup
         if ( !resip::DnsUtil::isIpAddress(ipHost) ) 
         {
            std::cerr << "Cannot convert local hostname into IP address: " << ipHost << " from uri: " << uriStr << "\n";
            exit(1);
         }
      }
      if ( resip::DnsUtil::isIpV6Address(ipHost) ) 
      {
         ipVers = resip::V6;
      }
#if 0
      if ( localUri.exists(resip::p_size) ) 
      {
         resip::Data sz = localUri.param(resip::p_size);
         numPorts = sz.convertInt();
         if ( numPorts <= 0 ) {
            std::cerr << "Bad port-length (size) parameter: " << uriStr << "\n";
            exit(1);
         }
      }
#endif
      tu->addTransportVec( proto, ipVers, ipHost, stunMode, 
         transFlags, portBase, numPorts);
   }
   // GONE: tu->mKeepAliveIvalSecs = mKeepAliveIvalSecs;
   //
}

RendTu* RendTuCreate(resip::SipStack& stack, RendAcctMgrIf& acctMgr) 
{
   RendTu *myTu = new RendTu(stack, acctMgr, /*kaSecs*/20);
   stack.registerTransactionUser(*myTu);
   TheRendTuOpts.configureTu(myTu);
   return myTu;
}

void RendTuDestroy(RendTu *tu) 
{
   if ( tu ) 
   {
      delete tu;
   }
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
