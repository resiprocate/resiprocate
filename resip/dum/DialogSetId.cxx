#include "DialogSetId.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

const DialogSetId DialogSetId::Empty;

DialogSetId::DialogSetId(const SipMessage& msg) : 
   mCallId(msg.header(h_CallID).value())
{
   //find local tag, generate one as necessary
   if (msg.isExternal())
   {
      if(msg.isResponse())
      {        
         if(msg.header(h_From).exists(p_tag))
         {
            // .bwc. If no tag, leave mTag empty.
            mTag = msg.header(h_From).param(p_tag);
         }
      }
      else //external request; generate to tag if not present
      {
         if (msg.header(h_To).exists(p_tag))
         {
            mTag = msg.header(h_To).param(p_tag);
         }
         else
         {
            //DebugLog ( <<  "********** Generated Local Tag *********** " );            
            mTag = Helper::computeTag(Helper::tagSize);
         }
      }
   }
   else
   {
      if(msg.isRequest())
      {
         resip_assert(msg.header(h_From).exists(p_tag));
         mTag = msg.header(h_From).param(p_tag);
      }
      else
      {
         resip_assert(msg.header(h_To).exists(p_tag));
         mTag = msg.header(h_To).param(p_tag);
      }
   }
}

DialogSetId::DialogSetId(const Data& callId, const Data& tag)
   : mCallId(callId),
     mTag(tag)
{
}

DialogSetId::DialogSetId() 
   : mCallId(),
     mTag()
{
}

bool
DialogSetId::operator==(const DialogSetId& rhs) const
{
   return mCallId == rhs.mCallId && mTag == rhs.mTag;
}

bool
DialogSetId::operator!=(const DialogSetId& rhs) const
{
   return mCallId != rhs.mCallId || mTag != rhs.mTag;
}

bool
DialogSetId::operator<(const DialogSetId& rhs) const
{
   if (mCallId < rhs.mCallId)
   {
      return true;
   }
   if (mCallId > rhs.mCallId)
   {
      return false;
   }
   return mTag < rhs.mTag;
}

bool 
DialogSetId::operator>(const DialogSetId& rhs) const
{
   if (mCallId > rhs.mCallId)
   {
      return true;
   }
   if (mCallId < rhs.mCallId)
   {
      return false;
   }
   return mTag > rhs.mTag;
}

size_t DialogSetId::hash() const
{
    return mCallId.hash() ^ mTag.hash();
}


EncodeStream&
resip::operator<<(EncodeStream& os, const DialogSetId& id)
{
    return os << id.mCallId << '-' << id.mTag ;
}

HashValueImp(resip::DialogSetId, data.hash());

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
