#include "resiprocate/dum/DestroyUsage.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include <cassert>

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM


resip::DestroyUsage::DestroyUsage(BaseUsageHandle target)
   :mHandle(target)
{
}

resip::DestroyUsage::~DestroyUsage()
{  
}


resip::Message* 
resip::DestroyUsage::clone() const
{
   assert(0);
   return new DestroyUsage(mHandle);
}

resip::Data
resip::DestroyUsage::brief() const
{
   return Data("DestroyUsage");
}

std::ostream& 
resip::DestroyUsage::encode(std::ostream& strm) const
{
   strm << this->brief();
   return strm;
}



void
resip::DestroyUsage::destroy()
{
   InfoLog (<< "Destroying " << mHandle.get());
   delete mHandle.get();
}
