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
   Data buffer;
   DataStream strm(buffer);
   
   if (mDialogSet)
   {
      static Data d("DestroyDialogSet");
      strm << d << " " << mDialogSet->getId();
   }
   else if (mDialog)
   {
      static Data d("DestroyDialog");
      strm << d << " " << mDialog->getId();
   }
   else
   {
      static Data d("DestroyUsage");
      strm << d << " " << *mHandle;
   }
   strm.flush();
   return buffer;
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
