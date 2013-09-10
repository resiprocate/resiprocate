#if !defined(TFM_ActionBase_hxx)
#define TFM_ActionBase_hxx

#include <boost/shared_ptr.hpp>
#include "rutil/Data.hxx"

class Event;

class ActionBase
{
   public:
      ActionBase();
      virtual ~ActionBase();
      void addNext(ActionBase* action);
      
      virtual void exec(boost::shared_ptr<Event> event);
      virtual void exec();
      virtual resip::Data toString() const;
      virtual void operator()(boost::shared_ptr<Event> event) = 0;
      virtual void operator()() = 0;
   protected:
      ActionBase* mNext;
};

// executes the specified ExpectActions in order
ActionBase*
chain(ActionBase* action1,
      ActionBase* action2);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14,
      ActionBase* action15);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14,
      ActionBase* action15,
      ActionBase* action16);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14,
      ActionBase* action15,
      ActionBase* action16,
      ActionBase* action17);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14,
      ActionBase* action15,
      ActionBase* action16,
      ActionBase* action17,
      ActionBase* action18);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14,
      ActionBase* action15,
      ActionBase* action16,
      ActionBase* action17,
      ActionBase* action18,
      ActionBase* action19);

ActionBase*
chain(ActionBase* action1,
      ActionBase* action2,
      ActionBase* action3,
      ActionBase* action4,
      ActionBase* action5,
      ActionBase* action6,
      ActionBase* action7,
      ActionBase* action8,
      ActionBase* action9,
      ActionBase* action10,
      ActionBase* action11,
      ActionBase* action12,
      ActionBase* action13,
      ActionBase* action14,
      ActionBase* action15,
      ActionBase* action16,
      ActionBase* action17,
      ActionBase* action18,
      ActionBase* action19,
      ActionBase* action20);

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
