#include <memory>

#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/Timer.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "Resolver.hxx"
#include "resiprocate/Dialog.hxx"

#include "InviteClient.hxx"
#include "Transceiver.hxx"

using namespace resip;
using namespace Loadgen;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

InviteClient::InviteClient(Transceiver& transceiver, const resip::Uri& proxy, 
                           int firstExtension, int lastExtension, 
                           int numInvites)
   : mTransceiver(transceiver),
     mProxy(proxy),
     mFirstExtension(firstExtension),
     mLastExtension(lastExtension),
     mNumInvites(numInvites)
{
   if (mNumInvites == 0)
   {
      mNumInvites = (mLastExtension - mFirstExtension) / 2;
   }
}

void
InviteClient::go()
{
   int numInvited = 0;
   
   Resolver target(mProxy);

   NameAddr to;
   to.uri() = mProxy;
   
   NameAddr from;
   from.uri() = mProxy;
   
   NameAddr contact;
   contact.uri() = mTransceiver.contactUri();
   
   UInt64 startTime = Timer::getTimeMs();
   InfoLog(<< "Invite client is attempting " << mNumInvites << " calls.");
   while (numInvited < mNumInvites)
   {
      for (int i=mFirstExtension; i < mLastExtension-1 && numInvited < mNumInvites; i+=2)
      {
         contact.uri().user() = Data(i);
         from.uri().user() = Data(i);
         to.uri().user() = Data(i+1);
         
         auto_ptr<SipMessage> invite(Helper::makeInvite(to, from, contact));
         
         mTransceiver.send(target, *invite);
         
         try
         {
            auto_ptr<SipMessage> i_100(waitForResponse(100, 1000));
            auto_ptr<SipMessage> i_180(waitForResponse(180, 1000));
            auto_ptr<SipMessage> i_200(waitForResponse(200, 1000));

            DebugLog(<< "Creating dialog.");
            
            Dialog dlog(contact);

            DebugLog(<< "Creating dialog as UAC.");
            dlog.createDialogAsUAC(*i_200);
            
            DebugLog(<< "making ack.");
            auto_ptr<SipMessage> ack(dlog.makeAck(*invite));
            DebugLog(<< "making bye.");
            auto_ptr<SipMessage> bye(dlog.makeBye());

            DebugLog(<< "Sending ack: << *ack");
            
            mTransceiver.send(*ack);
            mTransceiver.send(*bye);
            auto_ptr<SipMessage> b_200(waitForResponse(200, 1000));
            numInvited++;
         }
         catch(Exception e)
         {
            ErrLog(<< "Proxy not responding.");
            exit(-1);
         }
      }
   }
   UInt64 elapsed = Timer::getTimeMs() - startTime;
   cout << mNumInvites << " peformed in " << elapsed << " ms, a rate of " 
        << mNumInvites / ((float) elapsed / 1000.0) << " calls per second." << endl;
}

SipMessage* 
InviteClient::waitForResponse(int responseCode,
                              int waitMs)
{
   DebugLog(<< "Waiting for a " << responseCode << " for " << waitMs  << "ms");
   SipMessage* reg = mTransceiver.receive(waitMs);
   DebugLog(<< "Finished waiting for " << responseCode);
   if(reg)
   {         
      if (reg->isResponse() &&
          reg->header(h_StatusLine).responseCode() == responseCode)
      {
         return reg;
      }
      else
      {
         throw Exception("Invalid response.", __FILE__, __LINE__);
      }
   }
   else
   {
      throw Exception("Timed out.", __FILE__, __LINE__);
   }
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
