#include "resiprocate/dum/DestroyUsage.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/DialogSet.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include <cassert>

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

DestroyUsage::DestroyUsage(BaseUsageHandle target)
   :mHandle(target),
    mDialogSet(0),
    mDialog(0)
{
}

DestroyUsage::DestroyUsage(DialogSet* dialogSet)
   :mHandle(),
    mDialogSet(dialogSet),
    mDialog(0)
{
}

DestroyUsage::DestroyUsage(Dialog* dialog)
   :mHandle(),
    mDialogSet(0),
    mDialog(dialog)
{
}

DestroyUsage::DestroyUsage(const DestroyUsage& other) :
   mHandle(other.mHandle),
   mDialogSet(other.mDialogSet),
   mDialog(other.mDialog)
{
}

DestroyUsage::~DestroyUsage()
{  
}


Message* 
DestroyUsage::clone() const
{
   return new DestroyUsage(*this);
}

Data
DestroyUsage::brief() const
{
   if (mDialogSet)
   {
      static Data d("DestroyDialogSet");
      return d;
   }
   else if (mDialog)
   {
      static Data d("DestroyDialog");
      return d;
   }
   else
   {
      static Data d("DestroyUsage");
      return d;
   }
}

std::ostream& 
DestroyUsage::encode(std::ostream& strm) const
{
   strm << this->brief();
   return strm;
}



void
DestroyUsage::destroy()
{
   if (mDialogSet)
   {
      delete mDialogSet;
   }
   else if (mDialog)
   {
      delete mDialog;
   }
   else
   {
      delete mHandle.get();
   }
}
