#include <cassert>
#include "DumTimer.hxx"

using namespace resip;


DumTimer::DumTimer(Type type, unsigned long duration, BaseUsage::Handle & targetBu, int seq, int altSeq)
    : mType(type),
      mDuration(duration),
      mUsageHandle(targetBu),
      mSeq(seq),
      mSecondarySeq(altSeq)
 {
}

DumTimer::~DumTimer()
{
}
      
DumTimer::Type 
DumTimer::type() const
{
   return mType;
}

int 
DumTimer::seq() const
{
   return mSeq;
}

int 
DumTimer::secondarySeq() const
{
   return mSecondarySeq;
}
      
const Data& 
DumTimer::getTransactionId() const
{
   assert(0);
   return Data::Empty;
}

bool 
DumTimer::isClientTransaction() const
{
   assert(0);
   return false;
}
      
Data 
DumTimer::brief() const
{
   Data data;
   DataStream strm(data);
   strm << "DumTimer: " << mType << " : " << mDuration << "," << mSeq << "," << mSecondarySeq ;
   return data;
}

std::ostream& 
DumTimer::encode(std::ostream& strm) const
{
   strm << brief();
   return strm;
}

