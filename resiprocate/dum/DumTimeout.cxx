#include <cassert>
#include "DumTimeout.hxx"
#include "resiprocate/os/WinLeakCheck.hxx"

using namespace resip;

//const unsigned long DumTimeout::StaleCallTimeout = 3600*2; //2 hrs - now in profile

DumTimeout::DumTimeout(Type type, unsigned long duration, BaseUsageHandle targetBu, int seq, int altSeq)
    : mType(type),
      mDuration(duration),
      mUsageHandle(targetBu),
      mSeq(seq),
      mSecondarySeq(altSeq)
{
}

DumTimeout::DumTimeout(const DumTimeout& source)
    : mType(source.mType),
      mDuration(source.mDuration),
      mUsageHandle(source.mUsageHandle),
      mSeq(source.mSeq),
      mSecondarySeq(source.mSecondarySeq)
{
}

DumTimeout::~DumTimeout()
{
}

Message*
DumTimeout::clone() const
{
   return new DumTimeout(*this);
}

      
DumTimeout::Type 
DumTimeout::type() const
{
   return mType;
}

int 
DumTimeout::seq() const
{
   return mSeq;
}

int 
DumTimeout::secondarySeq() const
{
   return mSecondarySeq;
}
      
const Data& 
DumTimeout::getTransactionId() const
{
   assert(0);
   return Data::Empty;
}

bool 
DumTimeout::isClientTransaction() const
{
   assert(0);
   return false;
}
      
Data 
DumTimeout::brief() const
{
   Data data;
   DataStream strm(data);
   encode(strm);
   strm.flush();
   return data;
}

std::ostream& 
DumTimeout::encode(std::ostream& strm) const
{
   strm << "DumTimeout::";
   switch (mType)
   {
      case SessionExpiration:
         strm <<"SessionExpiration";
         break;
      case SessionRefresh:
         strm <<"SessionRefresh";
         break;
      case Registration:
         strm <<"Registration";
         break;
      case Provisional1:
         strm <<"Provisional1";
         break;
      case Provisional2:
         strm <<"Provisional2";
         break;
      case Publication:
         strm <<"Publication";
         break;
      case Retransmit200:
         strm <<"Retransmit200";
         break;
      case Retransmit1xx:
         strm <<"Retransmit1xx";
         break;
      case WaitForAck:
         strm <<"WaitForAck";
         break;
      case CanDiscardAck:
         strm <<"CanDiscardAck";
         break;
      case StaleCall:
         strm <<"StaleCall";
         break;
      case Subscription:
         strm <<"Subscription";
         break;
      case StaleReInvite:
         strm <<"StaleReInvite";
         break;
      case Glare:
         strm <<"Glare";
         break;
      case Cancelled:
         strm <<"Cancelled";
         break;
   }

   strm << ": duration=" << mDuration << " seq=" << mSeq;
   return strm;
}

BaseUsageHandle 
DumTimeout::getBaseUsage() const
{
   return mUsageHandle;
}

   

