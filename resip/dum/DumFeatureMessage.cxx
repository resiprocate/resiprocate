#include "rutil/ResipAssert.h"
#include "DumFeatureMessage.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "resip/dum/BaseUsage.hxx"

using namespace resip;

DumFeatureMessage::DumFeatureMessage(const Data& tid)
   : mTransactionId(tid)
{}

DumFeatureMessage::DumFeatureMessage(const DumFeatureMessage& source)
   : mTransactionId(source.mTransactionId)
{}

DumFeatureMessage::~DumFeatureMessage()
{}

Message*
DumFeatureMessage::clone() const
{
   return new DumFeatureMessage(*this);
}
            
EncodeStream&
DumFeatureMessage::encodeBrief(EncodeStream& strm) const
{
   return encode(strm);
}

EncodeStream& 
DumFeatureMessage::encode(EncodeStream& strm) const
{
   strm << "DumFeatureMessage::" << mTransactionId;
   return strm;
}
