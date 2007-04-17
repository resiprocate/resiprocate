#ifndef DIALOGSTATE
#define DIALOGSTATE


#include "rutil/Data.hxx"
#include "resip/stack/SipMessage.hxx"

using namespace resip;

class DialogState {
  public:
    DialogState(Data key,SipMessage* msg);
    const Data& key() { return mKey; }
    const H_To::Type& localTFHeader() {return mLocalTFHeader;}
    const H_To::Type& remoteTFHeader() {return mRemoteTFHeader;}
    unsigned long & localCSeq() {return mLocalCSeq;}
    unsigned long & remoteCSeq() {return mRemoteCSeq;}
    const Data & callId() {return mCallId;}
    NameAddr & remoteTarget() {return mRemoteTarget;}
    const NameAddrs & routeSet() {return mRouteSet;}
    const NameAddr & me() {return mMe;}
  private:

    //invariants
    Data mKey;
    H_To::Type mLocalTFHeader;
    H_To::Type mRemoteTFHeader;
    Data mCallId;
    NameAddrs mRouteSet;
    NameAddr mMe;

    //variants
    unsigned long mLocalCSeq;
    unsigned long mRemoteCSeq;
    NameAddr mRemoteTarget;
};
#endif
