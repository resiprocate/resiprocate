/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef DNS_RESULT_MESSAGE_HXX
#define DNS_RESULT_MESSAGE_HXX

#include "resip/stack/TransactionMessage.hxx"

namespace resip
{

/**
   @internal
*/
class DnsResultMessage : public TransactionMessage
{
   public:
      DnsResultMessage(const resip::Data& tid, bool isClient)
         : mTid(tid),
         mIsClientTransaction(isClient)
      {}
      
      virtual ~DnsResultMessage(){};
      
      virtual const Data& getTransactionId() const {return mTid;} 

      // indicates this message is associated with a Client Transaction for the
      // purpose of determining which TransactionMap to use
      virtual bool isClientTransaction() const {return mIsClientTransaction;}

      virtual Message* clone() const {return new DnsResultMessage(*this);}
      /// output the entire message to stream
      virtual std::ostream& encode(std::ostream& strm) const
      {
         strm << (mIsClientTransaction ? Data("Client ") : Data("Server ") )
               << Data("DnsResultMessage: tid=") << mTid;
         return strm;
      }
      /// output a brief description to stream
      virtual std::ostream& encodeBrief(std::ostream& strm) const
      {
         strm << (mIsClientTransaction ? Data("Client ") : Data("Server ") )
               << Data("DnsResultMessage: tid=") << mTid;
         return strm;
      }

   private:
      DnsResultMessage();
      resip::Data mTid;
      bool mIsClientTransaction;
};
}

#endif

/* Copyright 2007 Estacado Systems */
