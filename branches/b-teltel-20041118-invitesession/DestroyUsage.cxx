#include "resiprocate/dum/DestroyUsage.hxx"



void
DestroyUsage::destroy()
{
   InfoLog (<< "Destroying " << mHandle.get());
   delete mHandle.get();
}
