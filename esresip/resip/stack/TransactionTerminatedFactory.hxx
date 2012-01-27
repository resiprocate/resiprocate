/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef TRANSACTION_TERMINATED_FACTORY
#define TRANSACTION_TERMINATED_FACTORY

#include "resip/stack/TransactionTerminated.hxx"

namespace resip
{

/**
   @ingroup resip_config
   @brief Used to override the construction of TransactionTerminated messages 
      that are sent to the TransactionUser (specified on a per-transaction
      basis).

   For instance, it might be useful to include Dialog-related information in
   TransactionTerminated messages. TransactionTerminated doesn't have this
   ability, but an app could define a subclass 
   (say, TransactionTerminatedWithDialogInfo), and then create a 
   subclass of TransactionTerminatedFactory that would manufacture these.

   In order to use this class, you need to create an instance of the factory you
   wish to use, and pass it to SipMessage::setTransactionTerminatedFactory() in 
   the SipMessage that begins the transaction in question.

   @see SipMessage::setTransactionTerminatedFactory()
   @see TransactionTerminated
*/
class TransactionTerminatedFactory
{
   public:
      TransactionTerminatedFactory(){};
      virtual ~TransactionTerminatedFactory(){};
      virtual TransactionTerminated* make(const Data& tid, bool isClient, TransactionUser* tu)=0;
      virtual TransactionTerminatedFactory* clone() const=0;
};
}
#endif

/* Copyright 2007 Estacado Systems */
