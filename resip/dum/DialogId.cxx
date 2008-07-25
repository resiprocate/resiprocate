#include "DialogId.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

DialogId::DialogId(const SipMessage& msg) : 
   mDialogSetId(msg),
   mRemoteTag(Data::Empty)
{
   //find remote tag, which may not exist
   if (msg.isExternal())
   {
      if(msg.isResponse())
      {        
         if (msg.header(h_To).exists(p_tag))
         {
            mRemoteTag = msg.header(h_To).param(p_tag);
         }
      }
      else
      {
         if (msg.header(h_From).exists(p_tag))
         {
            mRemoteTag = msg.header(h_From).param(p_tag);
         }
      }
   }
   else
   {
      if(msg.isRequest())
      {
         //?dcm? -- is this just for 2543?  At this point, we will have to have
         //established a dialog(or else we would just have a dialogset)
         if (msg.header(h_To).exists(p_tag))
         {
            mRemoteTag = msg.header(h_To).param(p_tag);
         }
      }
      else
      {
         if (msg.header(h_From).exists(p_tag))
         {
            mRemoteTag = msg.header(h_From).param(p_tag);
         }
      }
   }
   DebugLog ( << "DialogId::DialogId: " << *this);   
}

DialogId::DialogId(const Data& callId, const Data& localTag, const Data& remoteTag) : 
   mDialogSetId(callId, localTag),
   mRemoteTag(remoteTag)
{
}

DialogId::DialogId(const DialogSetId& id, const Data& remoteTag) :
   mDialogSetId(id),
   mRemoteTag(remoteTag)
{
   DebugLog ( << "DialogId::DialogId: " << *this);   
}

bool
DialogId::operator==(const DialogId& rhs) const
{
   return mDialogSetId == rhs.mDialogSetId && mRemoteTag == rhs.mRemoteTag;
}

bool
DialogId::operator!=(const DialogId& rhs) const
{
   return mDialogSetId != rhs.mDialogSetId || mRemoteTag != rhs.mRemoteTag;
}

bool
DialogId::operator<(const DialogId& rhs) const
{
   if (mDialogSetId < rhs.mDialogSetId)
   {
      return true;
   }
   if (mDialogSetId > rhs.mDialogSetId)
   {
      return false;
   }
   return mRemoteTag < rhs.mRemoteTag;
}

const DialogSetId& 
DialogId::getDialogSetId() const
{
   return mDialogSetId;
}

const Data& 
DialogId::getCallId() const 
{
   return getDialogSetId().getCallId(); 
}

const Data& 
DialogId::getLocalTag() const 
{
   return getDialogSetId().getLocalTag(); 
}

const Data& 
DialogId::getRemoteTag() const 
{
   return mRemoteTag; 
}


EncodeStream&
resip::operator<<(EncodeStream& os, const DialogId& id)
{
    return os << id.mDialogSetId << "-" << id.mRemoteTag;
}


size_t DialogId::hash() const 
{
   return mDialogSetId.hash() ^ mRemoteTag.hash();
}

HashValueImp(resip::DialogId, data.hash());

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
