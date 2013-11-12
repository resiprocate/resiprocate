#include "rutil/BaseException.hxx"
#include "rutil/Logger.hxx"
#include "rutil/DataStream.hxx"
#include "tfm/Renderer.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SequenceSet.hxx"
#include "tfm/ExpectActionEvent.hxx"

#include <boost/version.hpp>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace boost;
using namespace std;
using resip::Data;
using resip::DataStream;


SequenceSet::SequenceSet()
   : mSequences(),
     mActiveSet(),
     mEventFifo("SequenceSet"),
     mFailed(false),
     mFileName(),
     mLineNumber(0),
     mExplanation(),
     mReset(false),
     mHandle(this)
{}

SequenceSet::~SequenceSet()
{
   mReset = true;
   clear();
}

void
SequenceSet::outputToInfo()
{
   for (list<SequenceClass*>::const_iterator i = mSequences.begin();
        i != mSequences.end(); i++)
   {
      InfoLog(<< "----------------------------------------------------------------------");
      InfoLog(<< Renderer(**i));
   }
   InfoLog(<< "=======================================================================");
}

void 
SequenceSet::clear()
{
   if (mReset)
   {
      for (list<SequenceClass*>::const_iterator i = mSequences.begin();
           i != mSequences.end(); i++)
      {
         delete *i;
      }
      mSequences.clear();      

      mActiveSet.clear();

      mEventFifo.clear();
   }
}
     
void
SequenceSet::enqueue(boost::shared_ptr<Event> event)
{
   DebugLog(<< "SequenceSet::enqueue(" << &*event << ") " << event->briefString());
   mEventFifo.add(event);
}

SequenceSet::EventFifo::TimerId
SequenceSet::enqueue(boost::shared_ptr<Event> event, int delay)
{
   InfoLog(<< "SequenceClass::enqueue " << event->briefString() << " with delay " << delay);
   return mEventFifo.addDelayMs(event, delay);
}

bool 
SequenceSet::executionFailed() const 
{
   if (!mFailed)
   {
      for (list<SequenceClass*>::const_iterator i = mSequences.begin();
           i != mSequences.end(); i++)
      {
         if ((*i)->isFailed())
         {
            return true;
         }
      }
   }
   return mFailed;
}

void
SequenceSet::globalFailure(const resip::Data& message)
{
   // keep first fail info
   if (this && !mFailed) 
   {
      mFailed = true;
      mExplanation = message;
   }
}

void
SequenceSet::globalFailure(resip::BaseException& e)
{
   // keep first fail info
   if (this && !mFailed) 
   {
      mFailed = true;
      mExplanation.clear();
      {
         resip::DataStream str(mExplanation);
         str << e;
      }
   }
}

bool
SequenceSet::exec()
{
   preLoop();
   bool succ = loop();
   postLoop();

   // next sequence addition clears old sequences
   mReset = true;
   clear();
   release();
   return succ;
}

void
SequenceSet::preLoop()
{
#ifndef WIN32//usleep for windows in a "compat"
	usleep(100000); // wait for provisioning updates
#else
	Sleep(100000/1000);
#endif
   // start each sequence by executing the action
   for (list<SequenceClass*>::const_iterator i = mSequences.begin();
        i != mSequences.end(); i++)
   {      
      try
      {
         // recursively set the sequence set on all end points
         (*i)->setSequenceSet(getHandle());
         // let it throw out to CPP
         (*i)->start();
      }
      catch (ExpectAction::Exception& e)
      {
         WarningLog(<< "SequenceSet::exec " << e);
         if (!mFailed)
         {
            resip::DataStream str(mExplanation);
            str << e;
            mFailed = true;
         }
         return;
      }
   }
}

bool 
SequenceSet::loop()
{
   DebugLog(<< "SequenceSet::exec");

   //CerrLog(<< "exec loop start: " << mActiveSet.size() << " " << mFailed);
   //!dcm!--turn on/off event acceptance in the Executive
   while (!mActiveSet.empty() && !mFailed)
   {
      DebugLog(<<"Looking for event...");
      handle(mEventFifo.getNext());
      DebugLog(<<"Found and handled an event.");
   }
   
   return !mFailed;
}

void
SequenceSet::postLoop()
{
   //usleep(1000); //!dcm! -- this causes better performance, but it shouldn't be necessary

// .dcm. weak_ptr in endpoints now 
//    for (list<SequenceClass*>::const_iterator i = mSequences.begin();
//         i != mSequences.end(); i++)
//    {
//       // recursively unset the sequence set on all end points
//       (*i)->setSequenceSet(0);
//    }

   DebugLog(<< "after exec loop: " << !mFailed);
}

void
SequenceSet::handle(boost::shared_ptr<Event> event)
{
   //CerrLog( << "Current ActiveSet");
   for (set<SequenceClass*>::const_iterator i = mActiveSet.begin();
        i != mActiveSet.end(); i++) 
   {
      //CerrLog( << **i);
   }
         
   // queueing exposes new sequences that may also need to queue
   bool done;
   do
   {
      done = true;
      for (set<SequenceClass*>::iterator i = mActiveSet.begin();
           i != mActiveSet.end(); i++) 
      {
         if ((*i)->queue())
         {
            done = false;
            // queue invalidates iterator if returns true
            break;
         }
      }
   } while (!done);
         
   DebugLog(<< "handle_event::dequeue(" << event << ") " << event->briefString());
   //CerrLog(<< "Dequeued: " << event->briefString());
         
#if BOOST_VERSION >= 103500
   boost::shared_ptr<ExpectActionEvent> action = dynamic_pointer_cast<ExpectActionEvent>(event);
#else
   boost::shared_ptr<ExpectActionEvent> action = shared_dynamic_cast<ExpectActionEvent>(event);
#endif   
   if (action)
   {
      try
      {
         DebugLog(<<"Executing stored action.");
         action->doIt();
      }
      catch (ExpectAction::Exception& e)
      {
         globalFailure(e);
      }
      return;
   }
         
#if BOOST_VERSION >= 103500
   boost::shared_ptr<TimeoutEvent> timeout = dynamic_pointer_cast<TimeoutEvent>(event);
#else
   boost::shared_ptr<TimeoutEvent> timeout = shared_dynamic_cast<TimeoutEvent>(event);
#endif   
   if (timeout)
   {
      Data errorMessage;
      {
         DataStream str(errorMessage);
         if (!timeout->getSequence().mExpects.empty()) 
         {
            str << "Sequence timed out on expect: " << *timeout->getSequence().mExpects.front()
                << endl << "     ";
            bool previousActive = false;
            timeout->getSequence().getRoot().prettyPrint(str, previousActive);
         }
         else
         {
            str << "Sequence timed out, but no pending expects!" << endl << "     ";
         }
      
         str << endl;
      }
      ErrLog (<< "Sequence timed out: " << errorMessage);
      globalFailure(errorMessage);
      //.dcm. globalFailure will caused all future events to be discarded; in
      //any case, throwing was making globalFailure dead code, casuing tiemouts
      //to show up as errors.
//      throw TestEndPoint::AssertException(errorMessage, __FILE__, __LINE__);
      return;
   }
         
#if BOOST_VERSION >= 103500
   boost::shared_ptr<SequenceDoneEvent> seqDone = dynamic_pointer_cast<SequenceDoneEvent>(event);
#else
   boost::shared_ptr<SequenceDoneEvent> seqDone = shared_dynamic_cast<SequenceDoneEvent>(event);
#endif   
   if (seqDone)
   {
      // remove sequences that have completed
      SequenceClass *seq = seqDone->getSequence();
      if (seq->removeFromActiveSet())
      {
         if (seq->getParent() && seq->getParent()->decrementBranches())
         {
            //InfoLog(<< "Sequence: " << seq << " is waking up parent: " << seq->getParent());
            seq->getParent()->addToActiveSet();
         }
      }
            
      if (mActiveSet.empty())
      {
         DebugLog(<< "ActiveSet is empty");
         return;
      }
   } 
   else
   {
      handleEvent(event);
   }
         
   return;
}

void
SequenceSet::handleEvent(boost::shared_ptr<Event> event)
{
   // prevent debug output on cascading message mismatches
   if (mFailed)
   {
      DebugLog(<< "Already Failed... skipping");
      return;
   }
   
   DebugLog(<< "There are currently: " << mActiveSet.size() << " active, " 
            << mSequences.size() << " total");

   bool handled = false;
   for (set<SequenceClass*>::const_iterator i = mActiveSet.begin();
        i != mActiveSet.end(); i++)
   {
      SequenceClass* seq = *i;
      DebugLog(<< "matching:" << *seq);
      if (seq->isMatch(event))
      {
         handled = true;
         seq->handleEvent(event);
         break;
      }
   }

#if BOOST_VERSION >= 103500
   if (!handled && !dynamic_pointer_cast<OptionalTimeoutEvent>(event))
#else
   if (!handled && !shared_dynamic_cast<OptionalTimeoutEvent>(event))
#endif   
   {
      if (!mFailed)
      {
         InfoLog(<< "Unhandled event: " << *event);
         // set global failure
         
         Data errorMessage;
         {
            DataStream str(errorMessage);

            str <<"No Sequence expected " + event->toString() << endl << "Sequence state is:" << endl;

            bool previousActive;
            for (list<SequenceClass*>::const_iterator i = mSequences.begin();
                 i != mSequences.end(); i++)
            {
               str << "     ";
               previousActive = false;
               (*i)->prettyPrint(str, previousActive);
               str << endl << endl;
            }            
         }

         mExplanation = errorMessage;
         mFailed = true;
         ErrLog(<< "failed: " << errorMessage);
      }
   }
   //DebugLog(<<"!* Finished handleEvent");
}

//------------------------------------------------------------------------------
// scripting interface
//------------------------------------------------------------------------------


EncodeStream&
operator<<(EncodeStream& str, const SequenceSet& sset)
{
   if (sset.executionFailed())
   {
      str << "Failed: " << sset.getExplanation();
      // !dlb! include the state?
   }
   else
   {
      str << "<OK>";
   }
   str.flush();
   return str;
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
