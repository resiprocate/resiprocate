#ifndef DIALOGSTATE
#define DIALOGSTATE


#include "resiprocate/os/Data.hxx"
#include "resiprocate/SipMessage.hxx"

using namespace resip;

class DialogState {
  public:
    DialogState(Data key,SipMessage* msg);
    const Data& key() { return mKey; }
    const To_Header::Type& localTFHeader() {return mLocalTFHeader;}
    const To_Header::Type& remoteTFHeader() {return mRemoteTFHeader;}
    unsigned long & localCSeq() {return mLocalCSeq;}
    unsigned long & remoteCSeq() {return mRemoteCSeq;}
    const Data & callId() {return mCallId;}
    NameAddr & remoteTarget() {return mRemoteTarget;}
    const NameAddrs & routeSet() {return mRouteSet;}
    const NameAddr & me() {return mMe;}
  private:

    //invariants
    Data mKey;
    To_Header::Type mLocalTFHeader;
    To_Header::Type mRemoteTFHeader;
    Data mCallId;
    NameAddrs mRouteSet;
    NameAddr mMe;

    //variants
    unsigned long mLocalCSeq;
    unsigned long mRemoteCSeq;
    NameAddr mRemoteTarget;
};
#endif
