#if !defined(SEQUENCE_HXX)
#define SEQUENCE_HXX

#include <iostream>
#include <list>
#include <set>
#include <memory>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>

#include "rutil/Data.hxx"
#include "resip/stack/ValueFifo.hxx"
#include "tfm/TestEndPoint.hxx"

class SequenceHandler;

/**
   A Sequence specifies:
   1. an intitial action
   2. a list of expects

   Expects specify:
   1. type specific data for determining if an event matches this expect
   2. a maximum allowed delay from the last accepted event
   3. an Action to take on matching an event. An action may throw to indicate
      that the message is ill-formed.

   void testFoo()
   {
       Sequence(Action, Expect+, hangAroundTime);
       +
     
       ExecuteSequences(); // CPPUNIT_ASSERT macro
   }

   Derived TestEndPoints produce Events and queue them to the Sequence worker
   thread via a fifo. These events may be spontaenous or transforms of other
   kings of events (e.g. received a SIP message on the SIP tranceiver).

   The Sequence worker thread dequeues an event, scans for a matching Sequence
   and executes the matching exepct's actions.

   If there is no Sequence whose first expect matches the event, a failure is
   triggered.

   Sequences may branch, allowing expression of parallel event processing. Each
   sub-Sequence is added to the active set of Sequences. The parent equence is
   suspended until each of the sub-Sequences has completed.

 */

class SequenceSet;
class SequenceClass : public AsciiGraphic
{
   public:
      friend class SequenceSet;
      // global SequenceSet for CPU tests (old interface)
      static boost::shared_ptr<SequenceSet> CPUSequenceSet;
      static bool CPUSequenceSetCleanup;

      static void CPUSequenceSetup();

#include "SequenceClassConstructorDecls.hxx"

      ~SequenceClass();

      friend class TestEndPoint::And;

      void fail(const resip::Data& message);

      void prettyPrint(EncodeStream& str, bool& previousActive, int ind = 5) const;

      virtual void render(AsciiGraphic::CharRaster &craster) const;
      virtual Box layout() const;
      virtual Box& displaceDown(const Box& box);
      virtual Box& displaceRight(const Box& box);
      unsigned int mContainerBottom;

   protected:
      bool isMatch(boost::shared_ptr<Event> event) const;
      void handleEvent(boost::shared_ptr<Event> event);
      int size();

      // returns true iff there are no non-optional Expects and 
      // the hangAround time has passed
      bool isSequenceDone() const;
      bool isFailed() const;
      void start();
      bool queue();
      void setParent(SequenceClass* parent);
      SequenceClass* getParent() const;
      const SequenceClass& getRoot() const;
      void setBranchCount(unsigned int count);
      void setAfterAction(ActionBase* action);
      bool decrementBranches();
      void addToActiveSet();
      bool removeFromActiveSet();
      void addExpect(TestEndPoint::ExpectBase* expect);
      void scheduleTimeout();
      void cancelTimeout();

      friend class SequenceHandler;
   private:
      void setSequenceSet(boost::shared_ptr<SequenceSet> set);
      boost::shared_ptr<SequenceSet> getSequenceSet() const {return mSet;}

      ActionBase* mAction;
      std::list<TestEndPoint::ExpectBase*> mExpects;
      std::list<TestEndPoint::ExpectBase*> mUsedExpects;
      unsigned int mHangAroundTimeMs;
      boost::shared_ptr<SequenceSet> mSet;

      // remember the linenumber of the Sequence
      int mLineNumber;
      bool mFailed;

      SequenceClass* mParent;
      unsigned int mBranchCount;
      ActionBase* mAfterAction;
      bool mTimingOut;
      resip::ValueFifo< boost::shared_ptr<Event> >::TimerId mTimerId;

      friend EncodeStream& operator<<(EncodeStream& s, const SequenceClass& sequence);

      // no value semantics
      SequenceClass(const SequenceClass& sequence);
      SequenceClass& operator=(const SequenceClass&);
};

EncodeStream&
operator<<(EncodeStream& s, const SequenceClass& sequence);

bool
operator<(const SequenceClass& lhs, const SequenceClass& rhs);

// Trigger timeout failure
class TimeoutEvent : public Event
{
   public:
      TimeoutEvent(SequenceClass& sequence);
      virtual ~TimeoutEvent();
      SequenceClass& getSequence();
      
      virtual resip::Data toString() const;
      virtual resip::Data briefString() const;

   private:
      SequenceClass& mSequence;
};

// treat as the event, but don't trigger expect actions
class OptionalTimeoutEvent : public Event
{
   public:
      OptionalTimeoutEvent(SequenceClass& sequence, TestEndPoint::ExpectBase* expect);
      virtual ~OptionalTimeoutEvent();
      SequenceClass& getSequence() const;
      TestEndPoint::ExpectBase* getExpect() const;

      virtual resip::Data toString() const;
      virtual resip::Data briefString() const;

   private:
      SequenceClass& mSequence;
      TestEndPoint::ExpectBase* mExpect;
};

class SequenceDoneEvent : public Event
{
   public:
      SequenceDoneEvent(SequenceClass* sequence);
      SequenceClass* getSequence();
      virtual resip::Data toString() const;
      virtual resip::Data briefString() const;

   private:
      SequenceClass* mSequence;
};

#include "SeqDecls.hxx"
#include "SubDecls.hxx"

// macro that excutes the Sequences of a test and CPPUNIT_ASSERTs on failure.
#define ExecuteSequences()                                                                      \
do                                                                                              \
{                                                                                               \
   /* get rid of this sequence set before next run -- see Seq functions */                      \
   SequenceClass::CPUSequenceSetCleanup = true;                                                 \
   boost::shared_ptr<SequenceSet> sset(SequenceClass::CPUSequenceSet);                          \
   sset->outputToInfo();                                                                        \
                                                                                                \
   /* local forces order of evalulation */                                                      \
   bool succ = !sset->exec();                                                                   \
   ::CppUnit::Asserter::failIf(succ,                                                            \
                               sset->getExplanation().c_str(),                                  \
                               ::CppUnit::SourceLine(__FILE__,                                  \
                                                     sset->getLineNumber()                      \
                                                     ? sset->getLineNumber()                    \
                                                     : __LINE__));                              \
} while (false)

#endif

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
