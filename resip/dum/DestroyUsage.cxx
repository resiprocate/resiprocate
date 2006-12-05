#include "precompile.h"
#include "resip/dum/DestroyUsage.hxx"
#include "rutil/Logger.hxx"
#include "resip/dum/BaseUsage.hxx"
#include "resip/dum/DialogSet.hxx"
#include "resip/dum/Dialog.hxx"
#include "rutil/WinLeakCheck.hxx"
#include <cassert>

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
/* ivr mod !Entire File!*/
DestroyUsage::DestroyUsage(const BaseUsageHandle &target)
   :mHandle(target)
{}

DestroyUsage::DestroyUsage(const DialogSetHandle& dialogSet)
   :mDialogSetHandle(dialogSet)
{}

DestroyUsage::DestroyUsage(const DialogHandle& dialog)
   :   mDialogHandle(dialog)
{}

DestroyUsage::DestroyUsage(const DestroyUsage& other) :
   mHandle(other.mHandle),
   mDialogSetHandle(other.mDialogSetHandle),
   mDialogHandle(other.mDialogHandle)
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
	if (mDialogSetHandle.isValid())
   {
      static Data d("DestroyDialogSet");
	  strm << d << " " << std::hex << mDialogSetHandle.get();
   }
	else if (mDialogHandle.isValid())
   {
      static Data d("DestroyDialog");
	  strm << d << " " << std::hex << mDialogHandle.get();
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
	if (mDialogSetHandle.isValid())
   {
	   InfoLog(<< "DestroyUsage::destroy(), Deleting mDialogSetHandle: " << std::hex << mDialogSetHandle.get());
      delete mDialogSetHandle.get();
   }
	else if (mDialogHandle.isValid())
   {
	   InfoLog(<< "DestroyUsage::destroy(), Deleting mDialogHandle: " << std::hex << mDialogHandle.get());
      delete mDialogHandle.get();
   }
   else if (mHandle.isValid())
   {
	   InfoLog(<< "DestroyUsage::destroy(), Deleting mhandle: " << std::hex << mHandle.get());
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
