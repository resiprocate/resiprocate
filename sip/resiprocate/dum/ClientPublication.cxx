#include <cassert>

#include "resiprocate/Helper.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/dum/ClientPublication.hxx"
#include "resiprocate/dum/Dialog.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/dum/DumTimeout.hxx"
#include "resiprocate/os/Logger.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;

ClientPublicationHandle 
ClientPublication::getHandle()
{
   return ClientPublicationHandle(mDum, getBaseHandle().getId());
}

ClientPublication::ClientPublication(DialogUsageManager& dum,
                                     Dialog& dialog,
                                     SipMessage& req)
   : BaseUsage(dum, dialog),
     mPublish(req)
{}

ClientPublication::~ClientPublication()
{
   mDialog.mClientPublication = 0;
}

SipMessage& 
ClientPublication::unpublish()
{
   mPublish.header(h_Expires).value() = 0;
   return mPublish;
}

void 
ClientPublication::dispatch(const SipMessage& msg)
{
   assert(msg.isResponse());
   const int& code = msg.header(h_StatusLine).statusCode();
   if (code < 200)
   {
      // throw it away
      return;
   }
   else if (code < 300) // success
   {
      mPublish.header(h_SIPIfMatch) = msg.header(h_SIPETag);
      
      mDum.addTimer(DumTimeout::Publication, 
                    Helper::aBitSmallerThan(mPublish.header(h_Expires).value()), 
                    getBaseHandle(),
                    ++mTimerSeq);
   }
   else
   {
      if (code == 412)
      {
         InfoLog(<< "SIPIfMatch failed -- republish");
         mPublish.remove(h_SIPIfMatch);
         refresh();
         return;
      }
      
      if (code == 423) // interval too short
      {
         delete this;
         return;
      }
   }
}

void 
ClientPublication::dispatch(const DumTimeout& timer)
{
    if (timer.seq() == mTimerSeq)
    {
       refresh();
    }
}

void
ClientPublication::refresh(unsigned int expiration)
{
   if (expiration == 0)
   {
      expiration = mPublish.header(h_Expires).value();
   }
   ++mPublish.header(h_CSeq).sequence();
   mDum.send(mPublish);
}

void
ClientPublication::update(const Contents* body)
{
   assert(body);
   mPublish.setContents(body);
   refresh();
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
