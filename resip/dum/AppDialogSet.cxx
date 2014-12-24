#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;

AppDialogSet::AppDialogSet(DialogUsageManager& dum) : 
   Handled(dum),
   mDum(dum),
   mDialogSet(0),
   mIsReUsed(false)
{}

AppDialogSet::~AppDialogSet()
{}

AppDialogSetHandle 
AppDialogSet::getHandle()
{
   return AppDialogSetHandle(mHam, mId);
}

void
AppDialogSet::destroy()
{
   delete this;
}

void 
AppDialogSet::end()
{
   if (mDialogSet)
   {
      mDialogSet->end();
   }
}

void
AppDialogSet::endCommand()
{
   AppDialogSetHandle handle = getHandle();
   mDum.post(new AppDialogSetEndCommand(handle));
}

SharedPtr<UserProfile>  
AppDialogSet::selectUASUserProfile(const SipMessage&)
{
   // Default is Master Profile - override this method to select a different userProfile for UAS DialogSets
   return mDum.getMasterUserProfile();
}

SharedPtr<UserProfile> 
AppDialogSet::getUserProfile()
{
   if(mDialogSet)
   {
      return mDialogSet->getUserProfile();
   }
   else
   {
      return SharedPtr<UserProfile>();
   }
}

AppDialog* 
AppDialogSet::createAppDialog(const SipMessage&)
{
   return new AppDialog(mDum);
}

DialogSetId 
AppDialogSet::getDialogSetId()
{
   if (mDialogSet)
   {
       return mDialogSet->getId();
   }
   else
   {
       return DialogSetId(Data::Empty, Data::Empty);
   }
}

AppDialogSet*
AppDialogSet::reuse()
{
   resip_assert(mDialogSet);
   mDialogSet->appDissociate();
   mDialogSet = 0;
   
   mIsReUsed = true;
   return this;
}

bool
AppDialogSet::isReUsed() const
{
   return mIsReUsed;
}

const Data 
AppDialogSet::getClassName()
{
   return "AppDialogSet";
}

EncodeStream& 
AppDialogSet::dump(EncodeStream& strm) const
{
   strm << "AppDialogSet " << mId;
   return strm;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
