#include "SubDialog.h"
#include "ResourceMgr.h"
#include "resiprocate/os/MD5Stream.hxx"

SubDialog::SubDialog(Data key, SipStack* stack, DialogState *dlgState)
 : mKey(key)
{
  assert (dlgState);
  mStack = stack;
  mSubState = NULL;
  mDlgState = dlgState;
}

SubDialog::~SubDialog()
{
  ResourceMgr::instance().detachFromPresenceDoc(mSubState->resource(),this);
  if (mSubState) delete mSubState;
}

SubDialogCondition
SubDialog::processSubscribe(SipMessage* msg)
{

  if (!mSubState)
  {
    mSubState = new SubscriptionState;

    mSubState->resource() = msg->header(h_RequestLine).uri().getAorNoPort();
    if (!msg->exists(h_Expires))
    {
      mSubState->expires() = time(NULL)+3600;
    }
    else
    {
      mSubState->expires() = (time(NULL)+msg->header(h_Expires).value());
    }


    mSubState->event() = msg->header(h_Event).value();
    mSubState->eventId() = msg->header(h_Event).param(p_id);
    //  need  subState.watcher
    //        subState.Accepts

    // check authorization
    //   reject if no
    // setup observation of resource 
    ResourceMgr::instance().attachToPresenceDoc(mSubState->resource(),this);
  } else {
    assert(mSubState);
    assert(mDlgState);
    // verify remote CSeq is sane
    mSubState->expires() = (time(NULL)+msg->header(h_Expires).value());

  }

  auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,202,""));
  resp->header(h_Expires).value() = mSubState->expires()-time(NULL);
  mStack->send(*resp);

  sendNotify(ResourceMgr::instance().presenceDocument(mSubState->resource())); 
  return ACTIVE;

}

void
SubDialog::sendNotify()
{
  sendNotify(ResourceMgr::instance().presenceDocument(mSubState->resource())); 
}

void
SubDialog::sendNotify(const Contents* document)
{

  assert (document);

  bool expired = (mSubState->expires()<=time(NULL));


  SipMessage *notify = new SipMessage;;
  {
    RequestLine rLine(NOTIFY);
    rLine.uri() = mDlgState->remoteTarget().uri();
    notify->header(h_RequestLine) = rLine; 
    notify->header(h_To) = mDlgState->remoteTFHeader();
    notify->header(h_From) = mDlgState->localTFHeader();
    notify->header(h_CallId).value() = mDlgState->callId();
    notify->header(h_CSeq).method() = NOTIFY;
    notify->header(h_CSeq).sequence() = (++(mDlgState->localCSeq()));
    notify->header(h_Event).value() = mSubState->event();
    if (mSubState->eventId()!=Data("")) 
    {
      notify->header(h_Event).param(p_id) = mSubState->eventId();
    }
    notify->header(h_Contacts).push_front(mDlgState->me());
    notify->header(h_MaxForwards).value() = 70;
    {
      Token substateHFV;
      if (expired)
      {
	substateHFV.value() = Data("terminated");
	substateHFV.param(p_reason) = Data("timeout");
      }
      else
      {
        substateHFV.value() = Data("active");// pending?
        substateHFV.param(p_expires) = mSubState->expires()-time(NULL);
      }
      notify->header(h_SubscriptionStates).push_front(substateHFV);
    }
    
    /*hackage*/
//    Token req;
//    req.value() = Data("YouDontSupportThis");
//    notify->header(h_Requires).push_front(req);
    /*end hackage*/
    notify->setContents(document);

    // Now build a via with the right dialog identifier in the client
    // field of the branch parameter
    { 
      Via firstVia;
      firstVia.param(p_branch).clientData()= key();
      notify->header(h_Vias).push_front(firstVia);
    }

  }
  //send message
  mStack->send(*notify);
  delete notify;
  //add transaction key to dlgState->pendingNotifies (may be waste)
}

SubDialogCondition
SubDialog::processNotifyResponse(SipMessage *msg)
{
  //verify transactionkey (may be waste)
  // if 1xx, ignore
  // if 2xx return current condition
  // if 481 just return condition TERMINATED
  //
  // think about other non2xx's - for now 
  //   become TERMINATED, send a new NOTIFY and
  //   return condition TERMINATED
  //
  //   Major hackage for simplet 1
  if (msg->header(h_StatusLine).responseCode() == 481)
  {
    return TERMINATED;
  }
  return ACTIVE;
}

time_t
SubDialog::expires() {
  assert(mSubState);
  return mSubState->expires();
}

void
SubDialog::presenceDocumentChanged(const Contents* document)
{
  sendNotify(document);
}
