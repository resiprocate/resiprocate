#include <cassert>
#include "DumTimer.hxx"

using namespace resip;

DumTimer::DumTimer(Type type, unsigned long duration, int cseq, int rseq) : 
   mType(type),
   mDuration(duration),
   mCseq(cseq),
   mRseq(rseq)
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
DumTimer::cseq() const
{
   return mCseq;
}

int 
DumTimer::rseq() const
{
   return mRseq;
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
   strm << "DumTimer: " << mType << " : " << mDuration << "," << mCseq << "," << mRseq;
   return data;
}

std::ostream& 
DumTimer::encode(std::ostream& strm) const
{
   strm << brief();
   return strm;
}

