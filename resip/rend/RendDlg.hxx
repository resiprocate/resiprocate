#if !defined(REND_DLG_HXX)
#define REND_DLG_HXX 1
/**
 * REND Dialog (Dlg) class
 *
 * Written by Kennard White (Logitech, Inc.)
**/

#include <map>

#include "rutil/SharedPtr.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TransactionUser.hxx"

#include "RendAcct.hxx"

struct RendReqCxt 
{
   RendReqCxt(const resip::SipMessage* msg, resip::MethodTypes meth)
      : mMsg(msg), mMethod(meth), mOutOfOrder(false), mRxTimeUs(0) {};

   // Caller always frees. Callee must make copy if needed
   // consider convert to sharedptr?. 
   const resip::SipMessage* mMsg;
   resip::MethodTypes mMethod;
   bool mOutOfOrder;
   RendTimeUs mRxTimeUs;
};

typedef int RendDlgFlags;
#define REND_DlgF_MustCreate  (1<<0)
#define REND_DlgF_MustExist   (1<<1)
// #define REND_DlgF_GotPostSubNotify   (1<<2)
#define REND_DlgF_StdDlg      (1<<3)  // totag-based standard dialog
#define REND_DlgF_ETagDlg     (1<<4)  // Etag-based PUB dialog
#define REND_DlgF_Renew       (1<<5)
#define REND_DlgF_KeepAlive   (1<<6)  // For incoming

#define REND_Sts_Busy         -10

// comments from UAC-perspective.
enum RendDlgState 
{
   REND_DS_None,
   REND_DS_Bound,       /* dialog bound into Tu */
   REND_DS_Opening,     /* dialog-forming request sent */
   REND_DS_Established, /* dialog-forming response received */
   REND_DS_Closing,     /* dialog-ending request sent */
   REND_DS_Closed,      /* dialog-ending reply received */
   REND_DS_MAX
};

typedef std::list<resip::Data> RendDataList;

/* process request action: return code
 * that allows callee to tell caller what to do
 */
typedef int RendRequestAction;
#define REND_PRA_RspSent 1  // caller already sent a response
#define REND_PRA_SendOk  200

class RendTu;

/**
    Session authenticate state between client&server (challenge-response).
    We are focused on performance test application, thus not trying to
    cover all the cases. For now, we only deal with no qop case, and
    only a single realm.
**/
class RendAuthState 
{
public:
   RendAuthState() : mRspCode(0) {}

   void update(int rspCode, const resip::Data &nonce) 
   {
      mRspCode = rspCode;
      mNonce = nonce;
   }
   int mRspCode;  // www-401 or proxy-407
   resip::Data mNonce;  // from UAS 
};


/**
    Notes:
    *)  Storing the Tu here is little bit redundant (the leaf classes
        have pointer to engine which has pointer to Tu), but the 4 extra
        bytes greatly simplifiers class structure.
**/


class RendDlg;
typedef resip::SharedPtr<RendDlg> RendDlgSharedPtr;

class RendDlg 
{
public:
   RendDlg(RendTu& tu,
      int fromAcctIdx, int toAcctIdx = -1);

   virtual ~RendDlg();

   RendTu& getTu() { return mTu; }
   RendAcctMgrIf& getAcctMgr();

   virtual resip::SipMessage* makeRequest(resip::MethodTypes method);
   virtual resip::SipMessage* makeNextReq() = 0;
   virtual int  handleRequest(RendReqCxt& cxt);
   virtual void handleResponse(RendTimeUs now, 
                               const resip::SipMessage *msg, 
                               const char *failWhy=NULL) = 0;
   virtual void handleConnTerm(RendTimeUs now);

   int badResponse( const resip::SipMessage *rsp, const char *why);
   int processResponse(RendTimeUs, const resip::SipMessage *msg);
   // void processRenew(RendTimeUs now);
   int  sendNextCmd(int expireSecs, int maxRetryReqCnt);
   void relDlgState();
   int latchEstablish(RendTimeUs now, const resip::SipMessage *rsp, const resip::Data& remoteTag);

   /**
   Construct a new unique localtag and register with Tu.
   Dialog must be in DS_None, and will transistion to DS_Bound.
   **/
   void buildAndRegLocalTag(RendDlgSharedPtr& dlgPtr);

   /**
   Returns status of last locally generated requested and its
   response. Returns 0 if no response yet, otherwise the latest
   response code. {*rttMs} is filled in with the round-trip-time
   in msec.
   **/
   int getRspState(int *rttMs);

   /**
   * Returns state of dialog.
   **/
   RendDlgState getDlgState() const { return mState; };

   /*
   * Below here stays the same for life of object
   */
   RendTu& mTu;
   int mFromAcctIdx;
   int mToAcctIdx;

   /*
   * Below here stays same during dialog, but can change
   * when dialog is recycled.
   */
   resip::Transport* mTransport;
   resip::Tuple mRemoteTarget; // of first hop
   RendLocalKey mLocalKeyIdx;
   // resip::NameAddr mLocalContact;
   resip::Data mCallId;
   resip::Data mLocalTag;
   RendLocalKey mKeepAliveKey;
   resip::Data mRemoteETag; // for PUBLISH
   /* Below here fixed for dialog, but changes when recycled,
   * and is only used by StdDlgs (SUBSCRIBE&INVITE)
   */
   resip::Data mRemoteTag;
   RendDataList mRouteSet;

   /* 
   * below here may change thru life of dialog
   */
   // RemoteContact is used as R-URI when in-dialog. It must be
   // convertable to Uri type. (We use Data type here because Uri is HUGE)
   RendDlgFlags mFlags;
   RendDlgState mState;
   resip::Data mRemoteContact;
   unsigned long mLocalCSeqReq;
   unsigned long mLocalCSeqRsp;
   unsigned long mRemoteCSeq;
   bool mRemoteCSeqFull;
   int mCntAuthFail;
   int mLastRspCode; // to last local request
   resip::Data mLastRspReason; // to last local request
   resip::Data mLastRspWarnings; // to last local request
   resip::MethodTypes mLastReqMethod; // of last local request
   RendTimeUs mReqSendTime; // for mLocalCSeqReq
   RendTimeUs mRspRecvTime; // for matching 2xx response

   int mCmdRetryCnt;

   /*
   * Not all SIP methods use expires, but enough do to put
   * this in base class. It is significant for opening and closing dialogs.
   * Expire tracks the UAS's view, while Renew tracks our UAC view.
   */
   void setExpireSecs(int expireSecs) 
   {
      mExpireSecs = expireSecs; 
   };
   int mExpireSecs;

   /*
   * Authentication
   */
   int addAuthCore(resip::SipMessage *req, int rspCode, const resip::Auth& inAuth);
   int addAuthFromCache(resip::SipMessage *req);
   int addAuthFromRsp(resip::SipMessage *req, const resip::SipMessage& rsp);
   // for now, we only track one session/proxy
   RendAuthState mAuthState;
};


typedef resip::SharedPtr<RendDlg> RendDlgSharedPtr;
typedef std::map<RendLocalKey, RendDlgSharedPtr> RendDialogMap;
typedef std::deque<resip::Transport*> RendTransportList;

struct RendTuStats 
{
   RendTuStats() : mKaMsgCnt(0), mTransTxReqCnt(0), mTransTxRspCnt(0),
                   mTransRxReqCnt(0), mTransRxRspCnt(0) { };
   int mKaMsgCnt;
   int mTransTxReqCnt;	// we are UAC
   int mTransTxRspCnt;	// we are UAS
   int mTransRxReqCnt;	// we are UAS
   int mTransRxRspCnt;	// we are UAC
};
class RendTimerQueue;
class RendKaMgrIf;

class RendTu : public resip::TransactionUser 
{
public:
   RendTu(resip::SipStack& stack, RendAcctMgrIf& acctMgr, int keepAliveIvalSecs);
   virtual ~RendTu();
   virtual const resip::Data& name() const { return mName; };

   //  takes ownership of the message (deletes when done)
   void processMsg(RendTimeUs now, resip::Message *msg);
   bool processAll(int ms);
   int processTimers(RendTimeUs now);

   RendAcctMgrIf& getAcctMgr() { return mAcctMgr; };

   RendLocalKey buildLocalKey( resip::Data& localTag);
   void registerDlg(RendLocalKey localKey, RendDlgSharedPtr& dlg);
   void unregisterDlg(RendLocalKey localKey);
   RendDlgSharedPtr lookupDlg(RendLocalKey localKey);
   RendDlgSharedPtr lookupDlg(const resip::Data& localTag, const char **badDetail);

   void sendMsg(std::auto_ptr<resip::SipMessage> msg, RendDlg *dlg = NULL);

   int processRequest(RendTimeUs now, const resip::SipMessage *rsp);
   int processResponse(RendTimeUs now, const resip::SipMessage *rsp);
   void processConnTerm(RendTimeUs now, const resip::Tuple& flow);

   int badRequest(const resip::SipMessage *req, int rspCode, const char *reason, 
                  const char *whyDetail, const char *why2=NULL);
   int badResponse( const resip::SipMessage *rsp, const char *why, const char *why2=NULL);

   void addTransportVec(resip::TransportType proto, resip::IpVersion vers,
                        const resip::Data& ipInterface, 
                        resip::StunSetting stun, int transportFlags,
                        int portBase, int numPorts);
   resip::Transport* getNextTransport();

   void setProxy(const resip::Uri &uri);

   resip::Data mName;
   resip::SipStack& mStack;
   RendAcctMgrIf& mAcctMgr;

   RendLocalKey mDlgTagSeq;
   resip::Data mLocalTagSuffix;

   RendDialogMap mDialogMap;
   RendTimerQueue* mTimers;

   resip::Uri mOutboundProxy;
   bool mOutboundMode;
   RendTransportList mTransports;
   int mCurTransportIdx;

   RendKaMgrIf* mKeepAliveMgr;

   void processTuStatsTimeout(RendTimeUs now);
   RendTuStats mStats;
};

extern RendOptsBase* TheRendTuOptsPtr;

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
