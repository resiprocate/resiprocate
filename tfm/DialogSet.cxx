#include "rutil/Subsystem.hxx"
#include "tfm/DialogSet.hxx"
#include "tfm/TestSipEndPoint.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/SipMessage.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace std;
using namespace resip;

DialogSet::DialogSet(boost::shared_ptr<SipMessage> msg,
                     const TestSipEndPoint& testSipEndPoint)
   : mMsg(msg),
     mDialogs(),
     mTestSipEndPoint(&testSipEndPoint)
{}

boost::shared_ptr<resip::SipMessage> 
DialogSet::getMessage() const
{
   return mMsg;
}

std::vector<resip::DeprecatedDialog>&
DialogSet::getDialogs()
{
   return mDialogs;
}

bool
DialogSet::isMatch(boost::shared_ptr<SipMessage> msg) const
{
   if (msg->isRequest() &&
       msg->header(h_CSeq).method() == NOTIFY)
   {
      return (msg->header(h_CallId) == mMsg->header(h_CallId) &&
              msg->header(h_To).param(p_tag) == mMsg->header(h_From).param(p_tag) &&
              // comparison not trivial?
              msg->header(h_Event) == mMsg->header(h_Event));
   }
   else if (msg->isResponse() &&
            msg->header(h_CSeq).method() == SUBSCRIBE)
   {
      return (msg->header(h_CallId) == mMsg->header(h_CallId) &&
              msg->header(h_From).param(p_tag) == mMsg->header(h_From).param(p_tag) &&
              msg->header(h_CSeq) == mMsg->header(h_CSeq));
   }
   else if (msg->isRequest() && // see no reason why this would be different
            msg->header(h_CSeq).method() == SUBSCRIBE)
   {
      return (msg->header(h_CallId) == mMsg->header(h_CallId) &&
              msg->header(h_From).param(p_tag) == mMsg->header(h_From).param(p_tag) &&
              msg->header(h_CSeq) == mMsg->header(h_CSeq));
   }
   else
   {
      CritLog(<< *msg);
      resip_assert(0);
   }
   resip_assert(0);
   return false;   
}

void
DialogSet::dispatch(boost::shared_ptr<SipMessage> msg)
{
   for (vector<DeprecatedDialog>::iterator i = mDialogs.begin();
        i != mDialogs.end(); ++i)
   {
      if (msg->isResponse() &&
          msg->header(h_CSeq).method() == SUBSCRIBE &&
          msg->header(h_StatusLine).statusCode()/100 == 2)
      {
         if (msg->header(h_To).exists(p_tag) &&
             i->getRemoteTag() == msg->header(h_To).param(p_tag))
         {
            InfoLog(<< "SUBSCRIBE/2xx targetRefreshResponse " << *i);
            i->targetRefreshResponse(*msg);
            return;
         }
             
      }
      else if (msg->isRequest() &&
               msg->header(h_CSeq).method() == NOTIFY)
      {
         if (msg->header(h_From).exists(p_tag) &&
             i->getRemoteTag() == msg->header(h_From).param(p_tag))
         {
            if (msg->header(h_SubscriptionState).value() == "terminated")
            {
               InfoLog(<< "removing terminated " << *i);
               mDialogs.erase(i);
            }
            else
            {
               InfoLog(<< "NOTIFY targetRefreshRequest " << *i);
               i->targetRefreshRequest(*msg);
            }
            return;
         }
      }
   }

   if (msg->isRequest() &&
       msg->header(h_CSeq).method() == NOTIFY &&
       msg->header(h_SubscriptionState).value() == "terminated")
   {
      WarningLog(<< "Terminating NOTIFY with no matching dialog: " << *msg);
      return;
   }

   if (msg->isResponse() &&
       msg->header(h_CSeq).method() == SUBSCRIBE &&
       msg->header(h_StatusLine).statusCode()/100 == 2)
   {
      DeprecatedDialog dialog(mTestSipEndPoint->getContact());
      dialog.createDialogAsUAC(*msg);
      mDialogs.push_back(dialog);
      InfoLog(<< "Creating dialog for SUBSCRIBE/2xx " << dialog);

   }
   else if (msg->isRequest() &&
            msg->header(h_CSeq).method() == NOTIFY &&
            (msg->header(h_SubscriptionState).value() == "active" ||
             msg->header(h_SubscriptionState).value() == "pending"))
   {
      DeprecatedDialog dialog(mTestSipEndPoint->getContact());
      dialog.createDialogAsUAC(*msg);
      mDialogs.push_back(dialog);
      InfoLog(<< "Creating dialog for NOTIFY " << dialog);
   }
}
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
