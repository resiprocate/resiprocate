#ifndef SequenceSet_hxx
#define SequenceSet_hxx

#include <iosfwd>
#include <list>
#include <set>
#include <memory>
#include <boost/shared_ptr.hpp>

#include "tfm/TestEndPoint.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/ValueFifo.hxx"

// .dlb. add an onComplete action?

// container for a set of Sequences that execute together
class SequenceSet
{
   public:
      friend class SequenceClass;

      SequenceSet();
      ~SequenceSet();

      void enqueue(boost::shared_ptr<Event> event);
      long enqueue(boost::shared_ptr<Event> event, int delay);
      void globalFailure(const resip::Data& message);
      void globalFailure(resip::BaseException& e);
      bool executionFailed() const;

      bool exec();
      resip::Data getExplanation() const { return mExplanation; }
      resip::Data getFileName() const { return mFileName; }
      int getLineNumber() const { return mLineNumber; }

      // .dlb. boy is this wrong...
      void outputToInfo();

      void clear();

      boost::shared_ptr<SequenceSet> getHandle() { return mHandle; }
      void release() { mHandle.reset(); }

   protected:
      void handle(boost::shared_ptr<Event> event);
      std::list<SequenceClass*> mSequences;
      std::set<SequenceClass*> mActiveSet;

      void handleEvent(boost::shared_ptr<Event> event);

      typedef resip::ValueFifo< boost::shared_ptr<Event> > EventFifo;
      EventFifo mEventFifo;

      bool mFailed;

      resip::Data mFileName;
      int mLineNumber;
      resip::Data mExplanation;

   private:
      void preLoop();
      bool loop();
      void postLoop();

      bool mReset;

   public:

   private:
      boost::shared_ptr<SequenceSet> mHandle;      
      // no value semanitcs
      SequenceSet(const SequenceSet&);
      SequenceSet& operator=(const SequenceSet&);
};

EncodeStream&
operator<<(EncodeStream& str, const SequenceSet& sset);

#endif
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
