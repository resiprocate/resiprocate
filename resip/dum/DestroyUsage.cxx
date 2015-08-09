#include "resip/dum/DestroyUsage.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/BaseUsage.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/Dialog.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/ResipAssert.h"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

DestroyUsage::DestroyUsage(BaseUsageHandle target)
   :mHandle(target),
    mDialogSet(0),
    mDialog(0)
{}

DestroyUsage::DestroyUsage(DialogSet* dialogSet)
   :mHandle(),
    mDialogSet(dialogSet),
    mDialog(0)
{}

DestroyUsage::DestroyUsage(Dialog* dialog)
   :mHandle(),
    mDialogSet(0),
    mDialog(dialog)
{}

DestroyUsage::DestroyUsage(const DestroyUsage& other) :
   mHandle(other.mHandle),
   mDialogSet(other.mDialogSet),
   mDialog(other.mDialog)
{}

DestroyUsage::~DestroyUsage()
{}

Message* 
DestroyUsage::clone() const
{
   return new DestroyUsage(*this);
}

EncodeStream& 
DestroyUsage::encodeBrief(EncodeStream& strm) const
{
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

   return strm;
}

EncodeStream& 
DestroyUsage::encode(EncodeStream& strm) const
{
   return strm << brief();
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
   else if (mHandle.isValid())
   {
      delete mHandle.get();
   }
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
