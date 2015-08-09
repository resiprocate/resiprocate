#include <list>

#include "cppunit/TestCase.h"
#include "rutil/BaseException.hxx"
#include "rutil/Logger.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SequenceSet.hxx"

#include <boost/version.hpp>

using namespace resip;
using namespace std;
using namespace boost;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

boost::shared_ptr<SequenceSet> SequenceClass::CPUSequenceSet;
bool SequenceClass::CPUSequenceSetCleanup = true;

void SequenceClass::CPUSequenceSetup()
{
   if (SequenceClass::CPUSequenceSetCleanup)
   {
      // get rid of the old SequenceSet
      if (SequenceClass::CPUSequenceSet)
      {
         //.dcm. -- should rationalize clear and release
         SequenceClass::CPUSequenceSet->clear();
         SequenceClass::CPUSequenceSet->release();
      }      
      SequenceSet* sset = new SequenceSet();
      SequenceClass::CPUSequenceSet = sset->getHandle();
      // reuse this SequenceSet until executed
      SequenceClass::CPUSequenceSetCleanup = false;
   }
}

int 
SequenceClass::size()
{
   return mExpects.size();
}

bool 
SequenceClass::isFailed() const
{
   return mFailed || !mExpects.empty();
}

void 
SequenceClass::setParent(SequenceClass* parent)
{
   mParent = parent;
}

SequenceClass* 
SequenceClass::getParent() const
{
   return mParent;
}


const SequenceClass&
SequenceClass::getRoot() const
{
   if (this->getParent() == 0)
   {
      return *this;
   }
   return this->getParent()->getRoot();
}

void 
SequenceClass::setBranchCount(unsigned int count)
{
   resip_assert(count);
   resip_assert(mBranchCount == 0);

   mBranchCount = count;
   removeFromActiveSet();
}

void
SequenceClass::setAfterAction(ActionBase* action)
{
   delete mAfterAction;
   mAfterAction = action;
}

bool 
SequenceClass::decrementBranches()
{
   resip_assert(mBranchCount);
   if (!(--mBranchCount))
   {
      if (mAfterAction)
      {
         InfoLog(<< "All branches satisfied: triggering after action");
         // .dlb. what if this throws?
         mAfterAction->exec();
      }
      return true;
   }
   else
   {
      return false;
   }
}

void
SequenceClass::fail(const resip::Data& message)
{
   // keep first fail info
   if (!getSequenceSet()->mFailed) 
   {
      getSequenceSet()->mFailed = true;
      Data errorMessage;
      {
         DataStream str(errorMessage);
         str << message << endl << "Sequence state:" << endl;
         str << "     ";
         bool previousActive = false;
         this->getRoot().prettyPrint(str, previousActive);
         str << endl;
      }
      getSequenceSet()->mExplanation = errorMessage;
      getSequenceSet()->mLineNumber = mLineNumber;
   }
   mFailed = true;
}

void
SequenceClass::prettyPrint(EncodeStream& str, bool& previousActive, int ind) const
{
   if (find(getSequenceSet()->mSequences.begin(),
            getSequenceSet()->mSequences.end(), this) != getSequenceSet()->mSequences.end())
   {
      str << "Seq(";
      str << mAction->toString() << ", ";
   }
   else
   {      
      str << "Sub(";
   }

   bool first = true;
   ind += 4;
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mUsedExpects.begin();
        i != mUsedExpects.end(); i++)
   {
      if (!first)
      {
         str << ", ";
      }
      str << endl;
      str << indent(ind);
      (*i)->prettyPrint(str, previousActive, ind);
   }

   if (!mUsedExpects.empty() && !mExpects.empty())
   {
      str << ", " << endl;
   }

   first = true;
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      if (!first)
      {
         str << ",";
      }
      str << endl;

      if (!previousActive)
      {
         str << "===> " << indent(ind - 5);
         (*i)->prettyPrint(str, previousActive, ind);
      }
      else
      {
         str << indent(ind);
         (*i)->prettyPrint(str, previousActive, ind);
      }
      first = false;
      previousActive = true;
   }
   str << ")";
   str.flush();
}

bool
SequenceClass::isMatch(boost::shared_ptr<Event> event) const
{
#if BOOST_VERSION >= 103500
   boost::shared_ptr<OptionalTimeoutEvent> to = dynamic_pointer_cast<OptionalTimeoutEvent>(event);
#else
   boost::shared_ptr<OptionalTimeoutEvent> to = shared_dynamic_cast<OptionalTimeoutEvent>(event);
#endif
   for (std::list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      DebugLog(<< "expect matching: " << **i);
      if (to)
      {
         if (to->getExpect() == *i)
         {
            return true;
         }
         else
         {
            continue;
         }
      }
      if ((*i)->isMatch(event))
      {
         return true;
      }
      else if (!(*i)->isOptional())
      {
         return false;
      }
   }
   return false;
}

void
SequenceClass::handleEvent(boost::shared_ptr<Event> event)
{
   TestEndPoint* user = event->getEndPoint();

   InfoLog(<< "inner handleEvent: " << " to " << user << " " << *this << endl << event);
  next:
   if (!mExpects.empty())
   {
      TestEndPoint::ExpectBase* expect = mExpects.front();
      
      mUsedExpects.push_back(mExpects.front());
      mExpects.pop_front();

#if BOOST_VERSION >= 103500
      boost::shared_ptr<OptionalTimeoutEvent> ot = dynamic_pointer_cast<OptionalTimeoutEvent>(event);
#else
      boost::shared_ptr<OptionalTimeoutEvent> ot = shared_dynamic_cast<OptionalTimeoutEvent>(event);
#endif
      if (ot && ot->getExpect() == expect)
      {
         mTimerId=-7;
         scheduleTimeout();
         return;
      }

      if (!expect->isMatch(event))
      {
         if (expect->isOptional())
         {
            //DebugLog("handleEvent -- skipping optional: " << *expect);
            cancelTimeout();
            scheduleTimeout();
            goto next;
         }
         else
         {
            DebugLog(<< "Failed expect: " << expect->explainMismatch(event));
      
            fail("Received " + event->briefString() + " expected " + expect->getMsgTypeString());
            return;
         }
      }
      DebugLog(<< "Matched");

      cancelTimeout();
      scheduleTimeout();
      try
      {
         if(user)
         {
            expect->onEvent(*user, event);
         }
         else
         {
            resip_assert(0);
         }
      }
      catch (BaseException& e)
      {
         InfoLog(<< "Failed expect action with: " << e << " for " << event->briefString());
         Data errorMessage;
         {
            DataStream str(errorMessage);
            str << "Failed expect action: " << e << ",  Event: " << event->toString();
         }
         fail(errorMessage);
      }
      catch (std::exception& e)
      {
         InfoLog (<< "Failed expect action with: " << e.what() << " for " << event->briefString());
         Data errorMessage;
         {
            DataStream str(errorMessage);
            str << "Failed expect action: " << e.what() << ",  Event: " << event->toString();
         }
         fail(errorMessage);
      }
      catch (...)
      {
         InfoLog(<< "Failed expect action with an unknown exception, Event: " << event->briefString());
         fail("Failed expect action with an unknown exception, Event: " + event->toString());
      }
   }
   else
   {
      InfoLog(<< "Failed expect: received " << event->briefString() << " expected no more messages");
      fail("Unexpected " + event->toString());
   }
}

void
SequenceClass::addToActiveSet()
{
   //DebugLog( << "Adding sequence: " << this << " to activeSet");
   getSequenceSet()->mActiveSet.insert(this);
   scheduleTimeout();
}

bool
SequenceClass::removeFromActiveSet()
{
   //DebugLog(<< "removeFromActiveSet: " << *this);
   set<SequenceClass*>::iterator i = getSequenceSet()->mActiveSet.find(this);
   if (i != getSequenceSet()->mActiveSet.end())
   {
      getSequenceSet()->mActiveSet.erase(i);
      cancelTimeout();
      return true;
   }
   else
   {
      return false;
   }
}

void SequenceClass::cancelTimeout()
{
   if (mTimerId != -7)
   {
      InfoLog(<< "SequenceClass::cancelTimeout(" << mTimerId << ") " << this);
      if(!getSequenceSet()->mEventFifo.cancel(mTimerId))
      {
         // Race condition; it is likely that the timer is sitting, unprocessed, 
         // in our message queue, right as we satisfy the expect that the timer
         // was waiting for. It would be nice to figure out a way to fix this.
         resip_assert(0);
      }
      mTimerId = -7;
   }
}

void SequenceClass::scheduleTimeout()
{
   if (mExpects.empty())
   {
      // schedule sequence done event
      InfoLog(<< "Queuing Sequence done: hangAroundTime: " <<mHangAroundTimeMs);
      mTimingOut = true;
      getSequenceSet()->enqueue(boost::shared_ptr<Event>(new SequenceDoneEvent(this)), mHangAroundTimeMs);
   }
   else
   {
      TestEndPoint::ExpectBase* f = mExpects.front();
      if (f->isOptional())
      {
         mTimerId = getSequenceSet()->enqueue(boost::shared_ptr<Event>(new OptionalTimeoutEvent(*this, f)), 
                                              f->getTimeout());
         InfoLog(<< "SequenceClass::scheduleTimeout(" << mTimerId << ") optional");
      }
      else
      {
         mTimerId = getSequenceSet()->enqueue(boost::shared_ptr<Event>(new TimeoutEvent(*this)), 
                                              f->getTimeout());
         InfoLog(<< "SequenceClass::scheduleTimeout(" << mTimerId << ") for " << *f << " with timeout "<< f->getTimeout());
      }
   }
}   

// execute the starting Action
// (called only from Execute)
void
SequenceClass::start()
{
   mTimerId = -7;
   addToActiveSet();
   mAction->exec();
}

bool 
SequenceClass::queue()
{
   if (!mExpects.empty() && mExpects.front()->queue(this))
   {
      mUsedExpects.push_back(mExpects.front());
      mExpects.pop_front();
      return true;
   }
   return false;
}

void
SequenceClass::addExpect(TestEndPoint::ExpectBase* expect)
{
   //DebugLog(<< "addExpect " << expect);
   
   mExpects.push_back(expect);
}

#include "SequenceClassConstructorDefns.hxx"

SequenceClass::~SequenceClass()
{
   delete mAction;
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      delete *i;
   }

   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mUsedExpects.begin();
        i != mUsedExpects.end(); i++)
   {
      delete *i;
   }

   delete mAfterAction;

   mSet.reset();
}

EncodeStream&
operator<<(EncodeStream& s, const SequenceClass& sequence)
{
   s << "Sequence(" << &sequence << ", line: " << sequence.mLineNumber << ")[";
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = sequence.mExpects.begin();
        i != sequence.mExpects.end(); i++)
   {
      s << **i << ", ";
   }
   s << "]";
   s.flush();
   return s;
}

bool
operator<(const SequenceClass& lhs, const SequenceClass& rhs)
{
   return &lhs < &rhs;
}

Box
SequenceClass::layout() const
{
   //InfoLog(<< "SequenceClass::layout");

   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      // layout alone (0,0) corner
      Box sub = (*i)->layout();

      // push down to position in sequence
      sub = (*i)->displaceDown(mBox);
      //InfoLog(<< "after displace down: " << sub);
      mBox.boxUnion(sub);
   }

   // center each in the widest
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      unsigned displace = mBox.mWidth/2 - (*i)->mBox.mWidth/2;
      //InfoLog(<< "displace: " << displace);
      (*i)->displaceRight(Box(0, 0, displace, 0));
   }   
   
   //InfoLog(<< "SequenceClass::layout: " << mBox);

   return mBox;
}

void 
SequenceClass::render(CharRaster& out) const
{
   //InfoLog(<< "SequenceClass::render");
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      (*i)->render(out);
   }   

   // pad the | after the sequence
   for (unsigned int i = mBox.mY +mBox.mHeight; i < mContainerBottom; i++)
   {
      out[i][mBox.mX + mBox.mWidth/2+1] = '|';
   }
}

Box&
SequenceClass::displaceDown(const Box& box)
{
   for (list<TestEndPoint::ExpectBase*>::iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      (*i)->displaceDown(box);
   }   

   AsciiGraphic::displaceDown(box);
   mContainerBottom += box.mY + box.mHeight;
   return mBox;
}

Box&
SequenceClass::displaceRight(const Box& box)
{
   for (list<TestEndPoint::ExpectBase*>::iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      (*i)->displaceRight(box);
   }   

   AsciiGraphic::displaceRight(box);
   return mBox;
}

TimeoutEvent::TimeoutEvent(SequenceClass& sequence)
   : Event(0),
     mSequence(sequence)
{
}

TimeoutEvent::~TimeoutEvent()
{
}

SequenceClass&
TimeoutEvent::getSequence()
{
   return mSequence;
}

resip::Data 
TimeoutEvent::toString() const
{
   Data s;
   {
      DataStream str(s);
      str << "<TimeoutEvent> " << mSequence;
   }
   return s;
}
      
resip::Data
TimeoutEvent::briefString() const
{
   Data s;
   {
      DataStream str(s);
      str << "<TimeoutEvent> " << &mSequence;
   }
   return s;
}

OptionalTimeoutEvent::OptionalTimeoutEvent(SequenceClass& sequence,
                                           TestEndPoint::ExpectBase* expect)
   : Event(0),
     mSequence(sequence),
     mExpect(expect)
{
}

OptionalTimeoutEvent::~OptionalTimeoutEvent()
{
}

SequenceClass&
OptionalTimeoutEvent::getSequence() const
{
   return mSequence;
}
      
TestEndPoint::ExpectBase*
OptionalTimeoutEvent::getExpect() const
{
   return mExpect;
}

resip::Data 
OptionalTimeoutEvent::toString() const
{
   Data s;
   {
      DataStream str(s);
      str << "<OptionalTimeoutEvent> " << mSequence;
   }
   return s;
}
      
resip::Data
OptionalTimeoutEvent::briefString() const
{
   Data s;
   {
      DataStream str(s);
      str << "<OptionalTimeoutEvent> " << &mSequence;
   }
   return s;
}

SequenceDoneEvent::SequenceDoneEvent(SequenceClass* sequence)
   : Event(0),
     mSequence(sequence)
{
}
      
SequenceClass* 
SequenceDoneEvent::getSequence()
{
   return mSequence;
}

resip::Data 
SequenceDoneEvent::toString() const
{
   Data s;
   {
      DataStream str(s);
      str << "<SequenceDoneEvent> " << mSequence;
   }
   return s;
}
      
resip::Data
SequenceDoneEvent::briefString() const
{
   Data s;
   {
      DataStream str(s);
      str << "<SequenceDoneEvent> " << &mSequence;
   }
   return s;
}

void
SequenceClass::setSequenceSet(boost::shared_ptr<SequenceSet> set)
{
   mSet = set;
   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mExpects.begin();
        i != mExpects.end(); i++)
   {
      (*i)->setSequenceSet(set);
   }

   for (list<TestEndPoint::ExpectBase*>::const_iterator i = mUsedExpects.begin();
        i != mUsedExpects.end(); i++)
   {
      (*i)->setSequenceSet(set);
   }
}

#include "SeqDefns.hxx"
#include "SubDefns.hxx"

// Copyright 2005 Purplecomm, Inc.
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
