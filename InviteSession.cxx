#include "InviteSession.hxx"

InviteSession::InviteSession() : 
   mLocalSdp(0),
   mRemoteSdp(0),
   mMyNextOffer(0),
   mPendingReceivedOffer(0)
{
}

const SdpContents* 
InviteSession::getLocalSdp()
{
   return mLocalSdp;
}


const SdpContents* 
InviteSession::getRemoteSdp()
{
   return mRemoteSdp;
}
