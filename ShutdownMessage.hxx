#if !defined(RESIP_SHUTDOWNMESSAGE_HXX)
#define RESIP_SHUTDOWNMESSAGE_HXX 

// Copyright 2002 Cathay Networks, Inc. 

#include "Message.hxx"

namespace resip
{

class ShutdownMessage : public Message
{
   public:
      ShutdownMessage() {};
      virtual const Data& getTransactionId() const { return Data::Empty; }
      virtual bool isClientTransaction() const { return false; }
      virtual Data brief() const { return ("Shutdown"); }
      virtual std::ostream& encode(std::ostream& strm) const { return strm << brief(); }
};
 
}

#endif
