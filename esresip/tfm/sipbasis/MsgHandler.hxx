#ifndef _MSGHANDLER_HXX
#define _MSGHANDLER_HXX

#include "cria.h"
#include "Transaction.h" // This doesn't work without including cria.h...


namespace basis {
  class SIPRequest;
  class SIPResponse;
};

class MsgHandler: public basis::TransactionLifecycleObserver, basis::TransactionObserver
{
  public:
    MsgHandler();
    virtual ~MsgHandler();
    
    // TransactionLifecycleObserver

    virtual basis::Observer::status_t transactionCreated(basis::Transaction* t);
    virtual void transactionTerminated(basis::Transaction* t);

    // TransactionObserver

    virtual basis::Observer::status_t onRequest(basis::SIPRequest* request,
                                                basis::Transaction* transaction);
    virtual void onResponse(basis::SIPResponse* response,
                            basis::Transaction* transaction);
    virtual void onTimeout(basis::SIPRequest* request,
                           basis::Transaction* transaction);
    virtual void onTransmitError(basis::SIPRequest* request, 
                                 int code, escs::String text);
  private:
    basis::SIPResponse* createMessageResponse(basis::SIPRequest* request,
                                              int code, escs::String reason);
    int magicAvalue;

};

#endif
/* Copyright 2007 Estacado Systems */
