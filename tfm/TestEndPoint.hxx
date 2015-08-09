#ifndef TestEndPoint_hxx
#define TestEndPoint_hxx

#include <iosfwd>
#include <exception>
#include <boost/lexical_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <list>
#include <boost/function.hpp>

#include "rutil/Data.hxx"

#include "tfm/TestException.hxx"
#include "tfm/AsciiGraphic.hxx"
#include "tfm/Event.hxx"
#include "tfm/ActionBase.hxx"
#include "tfm/ExpectAction.hxx"

//!dcm! -- deprecate this further
#define checkEchoName(_fn) check1(_fn, #_fn)

class SequenceClass;
class SequenceSet;
class TestEndPoint;

class TestEndPoint
{
   public:

      class GlobalFailure : public TestException
      {
         public:
            GlobalFailure(const resip::Data& msg, const resip::Data& file, int line);
            virtual resip::Data getName() const;
      };

      TestEndPoint() ;
      virtual ~TestEndPoint();

      virtual void clean() = 0;
      virtual resip::Data getName() const = 0;
      static int DebugTimeMult();
      
      boost::weak_ptr<SequenceSet> getSequenceSet() const;
      void setSequenceSet(boost::shared_ptr<SequenceSet> set);
      bool operator==(const TestEndPoint& endPoint) const;
      
      class Action : public ActionBase
      {
         public:
            Action();
            virtual ~Action();
            virtual void operator()() = 0;

            virtual void operator()(boost::shared_ptr<Event> event);
            
         private:
            Action(const Action&);
            Action& operator=(const Action&);
      };
      
      class EndPointAction : public Action
      {
         public:
            explicit EndPointAction(TestEndPoint* endPoint);
            virtual ~EndPointAction();
            using Action::operator();
            virtual void operator()();
            virtual void operator()(TestEndPoint& endPoint) = 0;

         private:
            TestEndPoint* mEndPoint;
      };

      class NoSeqAction : public EndPointAction
      {
         public:
            NoSeqAction();
            using EndPointAction::operator();
            virtual void operator()(TestEndPoint& user);
            virtual resip::Data toString() const;
      };
      
      class NoAction : public ExpectAction 
      {
         public:
            using ExpectAction::operator();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const { return "NoAction";}
      };
      ExpectAction* noAction();

      class Note : public ActionBase 
      {
         public:
            Note(TestEndPoint* endPoint, const char* message);
            virtual void operator()();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;
         private:
            TestEndPoint* mEndPoint;
            resip::Data mMessage;
      };
      ActionBase* note(const char* msg);

      class Pause : public ActionBase
      {
         public:
            explicit Pause(int msec, TestEndPoint* endPoint);
            virtual void exec(boost::shared_ptr<Event> event);
            virtual void exec();
            virtual void operator()() { resip_assert(0); }
            virtual void operator()(boost::shared_ptr<Event> event) { resip_assert(0); }
            virtual resip::Data toString() const;

         private:
            int mMsec;
            TestEndPoint* mEndPoint;
      };
      ActionBase* pause(int msec);

      /// Base for expectation matchers
      class ExpectPreCon
      {
         public:
            virtual ~ExpectPreCon() {}
            virtual bool passes(boost::shared_ptr<Event> event) = 0;
            virtual resip::Data toString() const = 0;
      };

      class AlwaysTrue : public ExpectPreCon
      {
         public:
            virtual resip::Data toString() const { return "Always true";}
            virtual bool passes(boost::shared_ptr<Event>) { return true; }
      };

      class ExpectBase : public AsciiGraphic
      {
         public:
            class Exception : public TestException
            {
               public:
                  Exception(const resip::Data& msg,
                            const resip::Data& file,
                            const int line);
                  virtual resip::Data getName() const ;
            };

         public:
            ExpectBase();
            virtual ~ExpectBase();

            virtual TestEndPoint* getEndPoint() const = 0;
            /// determine if the event matches
            virtual bool isMatch(boost::shared_ptr<Event> event) const = 0;
            virtual resip::Data explainMismatch(boost::shared_ptr<Event> event) const = 0;
            
            /// allow derived Expects to handle multiple messages

            // execute given the response
            virtual void onEvent(TestEndPoint& endPoint, boost::shared_ptr<Event> event) = 0;
            virtual resip::Data getMsgTypeString() const = 0;
            virtual unsigned int getTimeout() const = 0;

            virtual bool queue(SequenceClass* parent);
            virtual void setSequenceSet(boost::shared_ptr<SequenceSet> set);
            virtual EncodeStream& output(EncodeStream& s) const;
            virtual void prettyPrint(EncodeStream& str, bool& previousActive, int ind) const;

            friend ExpectBase* optional(ExpectBase* expect);
            bool isOptional() const;

         private:
            bool mIsOptional;
      };

      class AssertException : public resip::BaseException
      {
         public:
            AssertException(const resip::Data& msg,
                            const resip::Data& file,
                            const int line);
            virtual resip::Data getName() const ;
            virtual const char* name() const ;
      };

      typedef boost::function<bool (boost::shared_ptr<Event>) > PredicateFn;
      // general assertion mechanism -- allows general functor
      class Assert : public ExpectAction
      {
         public:
            Assert(TestEndPoint* from, PredicateFn fn, const resip::Data& label) ;
            using ExpectAction::operator();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;
         private:
            TestEndPoint& mEndPoint;
            resip::Data mLabel;
            PredicateFn mPredicate;
      };
      friend class Assert;
      ExpectAction* check1(PredicateFn fn, resip::Data label);

      typedef boost::function<void (boost::shared_ptr<Event>) > ExecFn;
      class Execute : public ExpectAction
      {
         public:
            Execute(TestEndPoint* from, ExecFn fn, const resip::Data& label) ;
            using ExpectAction::operator();
            virtual void operator()(boost::shared_ptr<Event> event);
            virtual resip::Data toString() const;
         private:
            TestEndPoint& mEndPoint;
            resip::Data mLabel;
            ExecFn mExec;
      };
      friend class Execute;

      class And : public ExpectBase
      {
         public:
            And(const std::list<SequenceClass*>& sequences,
                ActionBase* aferAllAction = 0);

            /// no associated endPoint
            virtual TestEndPoint* getEndPoint() const { return 0;}
            /// match if any sub-sequence matches
            virtual bool isMatch(boost::shared_ptr<Event> event) const;
            virtual resip::Data explainMismatch(boost::shared_ptr<Event> event) const;

            virtual void onEvent(TestEndPoint& endPoint, boost::shared_ptr<Event> event);
            /// done when all Sub-Sequences are done

            virtual bool queue(SequenceClass* parent);

            virtual void setSequenceSet(boost::shared_ptr<SequenceSet> set);
            
            /// never appears in output, but is called
            virtual resip::Data getMsgTypeString() const { return "And";}
            /// return a value indicating no timeout
            virtual unsigned int getTimeout() const { return 1000*60*60*10;}
            virtual ~And();

            virtual EncodeStream& output(EncodeStream& s) const;
            virtual void prettyPrint(EncodeStream& s, bool& previousActive, int ind) const;

            virtual void render(AsciiGraphic::CharRaster &craster) const;
            virtual Box layout() const;
            virtual Box& displaceDown(const Box& box);
            virtual Box& displaceRight(const Box& box);
            
         private:
            std::list<SequenceClass*> mSequences;
            bool mActive;
            ActionBase* mAfterAction;
      };

      class Exec : public ExpectBase
      {
      };

      static AlwaysTrue* AlwaysTruePred;

   protected:

      virtual void clear();

   private:
      // disabled
      TestEndPoint(const TestEndPoint&);
      TestEndPoint& operator=(const TestEndPoint&);

      boost::weak_ptr<SequenceSet> mSequenceSet;
};

EncodeStream& operator<<(EncodeStream& s, const TestEndPoint&);
EncodeStream& operator<<(EncodeStream& s, const TestEndPoint::ExpectBase& eb);

ExpectAction* noAction();

TestEndPoint::And*
And(SequenceClass* sequence1,
    SequenceClass* sequence2,
    ActionBase* afterAction = 0);

TestEndPoint::And*
And(SequenceClass* sequence1,
    SequenceClass* sequence2,
    SequenceClass* sequence3,
    ActionBase* afterAction = 0);

TestEndPoint::And*
And(SequenceClass* sequence1,
    SequenceClass* sequence2,
    SequenceClass* sequence3,
    SequenceClass* sequence4,
    ActionBase* afterAction = 0);

TestEndPoint::And*
And(SequenceClass* sequence1,
    SequenceClass* sequence2,
    SequenceClass* sequence3,
    SequenceClass* sequence4,
    SequenceClass* sequence5,
    ActionBase* afterAction = 0);

TestEndPoint::And*
And(const std::list<SequenceClass*>& sequences,
    ActionBase* afterAction = 0);

TestEndPoint::ExpectBase*
optional(TestEndPoint::ExpectBase* expect);

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
