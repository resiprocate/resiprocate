#include "MsgHandler.hxx"

#include "Transaction.h"

// KitchenSink - weed this out.
#include "esstring.h"
#include "Header.h"
#include "Transaction.h"
#include "TransactionManager.h"
#include "TimeValue.h"
#include "INETAddr.h"
#include "CallId.h"
#include "SMP.h"
#include "Via.h"
#include "To.h"
#include "From.h"
#include "Contact.h"
#include "Expires.h"
#include "Route.h"
#include "CSeq.h"
#include "Logger.h"
#include "TextPlainBody.h"
#include "MessageCpimBody.h"
#include "SIPURL.h"
#include "ConnectionManager.h"
#include "MessageStatusReportBody.h"
#include "OtherClientTransaction.h"
#include "ApplicationManager.h"
#include "EventHandler.h"
//#include "InputStream.h"
//#include "OutputStream.h"
#include "SIPMessage.h"


MsgHandler::MsgHandler() : magicAvalue(0)
{}

MsgHandler::~MsgHandler()
{}
    
basis::Observer::status_t
MsgHandler::transactionCreated(basis::Transaction* t)
{
  assert(t);
  {
    t->registerObserver(this);
  }
  return basis::Observer::HANDLED;
}

void
MsgHandler::transactionTerminated(basis::Transaction* t)
{
}


basis::Observer::status_t
MsgHandler::onRequest(basis::SIPRequest* request,
                      basis::Transaction* transaction) 
{
  // We only seem to have IgnoreCase? Methods are CS.
  escs::String method = request->getMethod();
  //if (ParserUtils::compareToIgnoreCase(method, "MESSAGE")!=0)
  if ( method != "MESSAGE" )
  {
    return basis::Observer::UNSUPPORTED_METHOD;
  }

  basis::SIPResponse *response=0;
  response = createMessageResponse(request,200,"OK");  
  transaction->sendResponse(response);
  // adam tells me that the stack owns (or should own) response now.
  // similarly, somebody that's not me owns request, so I'm not deleting
  // either of them.
  return basis::Observer::HANDLED;
}

void 
MsgHandler::onResponse(basis::SIPResponse* response,
                       basis::Transaction* transaction) 
{
}

void 
MsgHandler::onTimeout(basis::SIPRequest* request,
                      basis::Transaction* transaction) 
{
}

void 
MsgHandler::onTransmitError(basis::SIPRequest* request, 
                            int code, escs::String text) 
{
}


// This is swiped in whole-cloth from MsgClient
basis::SIPResponse* 
MsgHandler::createMessageResponse(basis::SIPRequest* request,
                                 int statusCode,
                                 escs::String reasonPhrase)
{
  Header *h;

  // Who deletes these?
  basis::StatusLine *sl = new basis::StatusLine(statusCode, reasonPhrase);
  basis::SIPResponse* response = new basis::SIPResponse(sl);

  h = request->getHeader(basis::FROM_HEADER_CONSTANT());
  if (h != 0)
  {
    response->addHeader(h->clone());
  }

  h = request->getHeader(basis::TO_HEADER_CONSTANT());
  if (h != 0)
  {
    response->addHeader(h->clone());
  }

  CallId *cid =
      dynamic_cast<CallId *>(request->getHeader(basis::CALL_ID_HEADER_CONSTANT()));

  if (cid != 0)
  {
    response->addHeader(basis::CALL_ID_HEADER_CONSTANT(),cid->clone());
  }

  h = request->getHeader(basis::CSEQ_HEADER_CONSTANT());

  if (h != 0)
  {
    response->addHeader(h->clone());
  }

  // Need to copy all the Via headers, not the topmost Via

  std::list<Header *> *hl =
      request->getHeaderList(basis::VIA_HEADER_CONSTANT());
  if (hl != 0)
  {
    for (std::list<Header *>::iterator j = hl->begin();
          j != hl->end(); j++)
    {
      response->addHeader(basis::VIA_HEADER_CONSTANT(),
                          dynamic_cast<Via *>(*j)->clone());
    }
  }

  h = request->getHeader(basis::CONTACT_HEADER_CONSTANT());
  if (h != 0)
  {
    response->addHeader(h->clone());
  }

  h = request->getHeader(basis::EXPIRES_HEADER_CONSTANT());
  if (h != 0)
  {
    response->addHeader(h->clone());
  }

  response->setAddress(request->getAddress());

  {
   char aValStr[40];
   snprintf(aValStr,40,"%d",magicAvalue++);
   response->addHeader(new UnknownHeader("X-magicAvalue",aValStr));
  }

  return response;

}
/* Copyright 2007 Estacado Systems */
