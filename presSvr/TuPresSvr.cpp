#include "resiprocate/Helper.hxx"

#include "ResourceMgr.h"
#include "SubDialog.h"
#include "TuPresSvr.h"

using namespace resip;

bool
TuPresSvr::process()
{

    bool done = 0;
    FdSet fdset;
    mStack->buildFdSet(fdset);
//    int err = fdset.selectMilliSeconds(0);
    int err = fdset.selectMilliSeconds(100);
    assert( err != -1 );

    mStack->process(fdset);

    SipMessage* msg = (mStack->receive());
    if (msg)
    {
      if (msg->isRequest())
      {
	if (msg->header(h_RequestLine).getMethod() == SUBSCRIBE )
	{
          processSubscribe(msg);
	}
	else if (msg->header(h_RequestLine).getMethod() == REGISTER )
	{
	  processPublish(msg);
	}
	else if (msg->header(h_RequestLine).getMethod() == OPTIONS )
	{
          auto_ptr<SipMessage> resp(
               Helper::makeResponse(*msg,500,"You Shot Me!")); 
          mStack->send(*resp);
	  done = 1;
	}
	else if (   (msg->header(h_RequestLine).getMethod() == UNKNOWN) 
		 && (msg->header(h_RequestLine).unknownMethodName()=="PUBLISH")
		)
	{
           processPublish(msg);
	}
	else
	{
          auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,501,"")); 
          mStack->send(*resp);
	}
      }
      else
      {
	/*
	 Nope - dialog key is currently overscoped to requests - bad.
	assert(msg->isResponse());
	if (msg->header(h_CSeq).method()==NOTIFY)
	  mDialogMgr.dispatchNotifyResponse(msg);
         */
      }
      delete msg;
    } else {
      mDialogMgr.processExpirations();
    }
    return done;
}

void
TuPresSvr::processSubscribe(SipMessage* msg)
{
  // See if this subscribe matches a dialog we have in progress
  if (mDialogMgr.dialogExists(msg))
       { mDialogMgr.dispatchSubscribe(msg); }
  else { processNewSubscribe(msg); }
}

void TuPresSvr::processNewSubscribe(SipMessage* msg)
{
  if (   !msg->exists(h_Event)
       || msg->header(h_Event).value()!=Data("presence")
     )
  {
    auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,489,"")); 
    Token presToken;
    presToken.value()=Data("presence");
    resp->header(h_AllowEvents).push_back(presToken); 
    mStack->send(*resp);
    return;
  }

  if (ResourceMgr::instance()
		  .exists(msg->header(h_RequestLine).uri().getAorNoPort()))
  {
    mDialogMgr.dispatchNewSubscribe(msg);
  }
  else
  {
    auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,404,"")); 
    mStack->send(*resp);
  }

}

void TuPresSvr::processPublish(SipMessage* msg)
{
  //ignore any PUBLISH related headers and any contacts
  //provided in a REGISTER
  //This is a rather vile hack for SIMPLEt 1
  Data aor;
  if ( msg->header(h_RequestLine).getMethod() == REGISTER )
  {
    aor = msg->header(h_To).uri().getAorNoPort();
  }
  else
  {
    aor = msg->header(h_RequestLine).uri().getAorNoPort();
  }
  if (ResourceMgr::instance().exists(aor))
  {
    Contents * contents = msg->getContents();
    if (contents)
    {
      int retcode = (ResourceMgr::instance().setPresenceDocument(aor,contents)
	             ?200:403);
      auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,retcode,"")); 
      mStack->send(*resp);
    }
    else
    {
      auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,400,"This hacked-up service requires a body")); 
      mStack->send(*resp);
    }
  }
  else
  {
    auto_ptr<SipMessage> resp(Helper::makeResponse(*msg,404,"")); 
    mStack->send(*resp);
  }
}
