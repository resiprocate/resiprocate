#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/predicates/NotifyContents.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST
using namespace resip;

NotifyContents::NotifyContents(int count)
   : mTuples(),
     mCount(count)
{}

NotifyContents::NotifyContents(const std::vector<Pidf::Tuple>& tuples)
   : mTuples(tuples),
     mCount(-1)
{}

NotifyContents::NotifyContents(Pidf::Tuple tuples[])
   : mTuples()
{
   while (!(*tuples).id.empty())
   {
      mTuples.push_back(*tuples);
      ++tuples;
   }
}

bool 
NotifyContents::operator()(boost::shared_ptr<Event> event)
{
   SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
   resip_assert(sipEvent);
   boost::shared_ptr<resip::SipMessage> msg = sipEvent->getMessage();
   
   return (*this)(msg);
}

bool
NotifyContents::operator()(boost::shared_ptr<resip::SipMessage> msg)
{
   if (!msg->getContents())
   {
      InfoLog(<< "NotifyContents failed: contents");
      return false;
   }

   Pidf* pidf = dynamic_cast<Pidf*>(msg->getContents());
   if (!pidf)
   {
      InfoLog(<< "NotifyContents failed: Pidf");
      return false;
   }

   if (mCount != -1)
   {
      if ((unsigned int)mCount != pidf->getTuples().size())
      {
         InfoLog(<< "NotifyContents failed: tuple size " << pidf->getTuples().size() << " needed " << mCount);
         return false;
      }
      return true;
   }

   if (pidf->getTuples().size() != mTuples.size())
   {
      InfoLog(<< "NotifyContents failed: tuple size");
      return false;
   }

   for (std::vector<Pidf::Tuple>::const_iterator i = mTuples.begin();
        i != mTuples.end(); ++i)
   {
      bool found = false;
      for (std::vector<Pidf::Tuple>::const_iterator j = pidf->getTuples().begin();
           j != pidf->getTuples().end(); ++j)
      {
         if (i->id == j->id)
         {
            found = true;
            if (i->contact != "*" &&
                i->contact != j->contact)
            {
               InfoLog(<< "NotifyContents failed tuple contact: " << *j);
               return false;
            }

            if (i->contactPriority != -1.0 &&
                i->contactPriority != j->contactPriority)
            {
               InfoLog(<< "NotifyContents failed tuple contact priority: " << *j);
               return false;
            }

            if (i->note != "*" &&
                i->note != j->note)
            {
               InfoLog(<< "NotifyContents failed tuple contact note: " << *j);
               return false;
            }

            // every attribute in i must be in j
            for (HashMap<Data, Data>::const_iterator a = i->attributes.begin();
                 a != i->attributes.end(); ++a)
            {
               HashMap<Data, Data>::const_iterator b = j->attributes.find(a->first);
               if (b == j->attributes.end())
               {
                  //InfoLog(<< "NotifyContents failed tuple attribute: " << *i << "[" << a->first << "]");
                  return false;
               }

               if (a->second != "*" &&
                   a->second != b->second)
               {
                  //InfoLog(<< "NotifyContents failed tuple attribute: " << *i << "[" << a->first << "] = " << a->second);
                  return false;
               }
            }
         }
      }

      if (!found)
      {
         InfoLog(<< "NotifyContents failed: " << *i);
         return false;
      }
   }

   return true;
}


// conditioners

boost::shared_ptr<SipMessage>& 
simulateRefresh(boost::shared_ptr<SipMessage>& msg)
{ 
   msg->setContents(0);
   msg->header(h_SIPIfMatch).value() = "2134";

   CerrLog(<< "simulateRefresh: " << *msg);

   return msg;
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
