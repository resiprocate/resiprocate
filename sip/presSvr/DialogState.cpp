#include "DialogState.h"

DialogState::DialogState(Data key,SipMessage* msg)
 :mKey(key)
{
  //Assume for now that msg is a request we recieved
  //Remember that this is a bad assumption and more
  //work needs to happen here
  
  //invariants
  mLocalTFHeader = msg->header(h_To);
  mRemoteTFHeader = msg->header(h_From);
  mCallId = msg->header(h_CallId).value();
  if (msg->exists(h_RecordRoutes))
  {
    mRouteSet = msg->header(h_RecordRoutes);
  }
  mMe = NameAddr(msg->header(h_RequestLine).uri());

  //variants
  mLocalCSeq = 0;
  mRemoteCSeq = msg->header(h_CSeq).sequence();
  mRemoteTarget = msg->header(h_Contacts).front();

}
