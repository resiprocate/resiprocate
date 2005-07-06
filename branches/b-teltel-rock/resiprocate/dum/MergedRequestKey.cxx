#include "resiprocate/dum/MergedRequestKey.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/CSeqCategory.hxx"

using namespace resip;

const MergedRequestKey MergedRequestKey::Empty;

MergedRequestKey::MergedRequestKey()
{
}

MergedRequestKey::MergedRequestKey(const SipMessage& req) : 
   mRequestUri(Data::from(req.header(h_RequestLine).uri())),
   mCseq(Data::from(req.header(h_CSeq))),
   mTag(req.header(h_From).exists(p_tag) ? req.header(h_From).param(p_tag) : Data::Empty),
   mCallId(req.header(h_CallID).value())
{
}

bool
MergedRequestKey::operator==(const MergedRequestKey& other) const
{
   return (mCallId == other.mCallId  &&
           mTag == other.mTag &&
           mCseq == other.mCseq &&
           mRequestUri == other.mRequestUri);
}

bool
MergedRequestKey::operator!=(const MergedRequestKey& other) const
{
   return !(*this == other);
}

bool
MergedRequestKey::operator<(const MergedRequestKey& other) const
{
   if ( mCallId < other.mCallId)
   {
      return true;
   }
   else if (mCallId > other.mCallId)
   {
      return false;
   }
   
   if (mTag < other.mTag)
   {
      return true;
   }
   else if (mTag > other.mTag)
   {
      return false;
   }
   
   if (mCseq < other.mCseq)
   {
      return true;
   }
   else if (mCseq > other.mCseq)
   {
      return false;
   }
   
   return (mRequestUri < other.mRequestUri);
}

