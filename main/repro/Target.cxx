#include "repro/Target.hxx"

#include "resip/stack/Uri.hxx"
#include "resip/stack/NameAddr.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/Via.hxx"

namespace repro
{

Target::Target()
{
   mStatus=Pending;
}

Target::Target(const resip::Uri& uri)
{
   mNameAddr=resip::NameAddr(uri);
   mStatus=Pending;
}

Target::Target(const resip::NameAddr& target)
{
   mNameAddr=target;
   mStatus=Pending;
}

Target::Target(const repro::Target& target)
{
   mNameAddr=target.mNameAddr;
   mStatus=target.mStatus;
   mVia=target.mVia;
}



Target::~Target()
{
   
}


const resip::Data&
Target::tid() const
{
   return mVia.param(resip::p_branch).getTransactionId();
}


Target::Status&
Target::status()
{
   return mStatus;
}

const Target::Status&
Target::status() const
{
   return mStatus;
}


resip::Uri&
Target::uri()
{
   return mNameAddr.uri();
}

const resip::Uri&
Target::uri() const
{
   return mNameAddr.uri();
}


resip::Via&
Target::via()
{
   return mVia;
}

const resip::Via&
Target::via() const
{
   return mVia;
}


resip::NameAddr&
Target::nameAddr()
{
   return mNameAddr;
}

const resip::NameAddr&
Target::nameAddr() const
{
   return mNameAddr;
}

} // namespace repro
