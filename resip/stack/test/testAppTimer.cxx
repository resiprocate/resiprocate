#include <iostream>
#include "resip/stack/TransactionMessage.hxx"
#include "resip/stack/TimerQueue.hxx"
#include "resip/stack/TuSelector.hxx"
#include "rutil/Fifo.hxx"
#include "rutil/Logger.hxx"
#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#ifdef WIN32
#define usleep(x) Sleep(x/1000)
#define sleep(x) Sleep(x*1000)
#endif

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace resip;
using namespace std;

bool
isNear(int value, int reference, int epsilon=250)
{
   int diff = ::abs(value-reference);
   return (diff < epsilon);
}

class AppMessage : public Message
{
   public:
      AppMessage(const Data& text)
         : mText(text)
      {}

      virtual EncodeStream& encode(EncodeStream& s) const {s << mText; return s;}
      virtual EncodeStream& encodeBrief(EncodeStream& s) const {return s << mText;}

      const Data& getText() const
      {
         return mText;
      }
      
      virtual Message* clone() const
      {
         return new AppMessage(mText);
      }

   private:
      const Data mText;
};

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   TimeLimitFifo<Message> f(0, 0);
   TimeLimitTimerQueue timer(f);

   cerr << "Before Fifo size: " << f.size() << endl;
   assert(f.size() == 0);
   cerr << "next timer = " << timer.msTillNextTimer() << endl;
   assert(timer.msTillNextTimer() == INT_MAX);

   timer.add(1000, new AppMessage("first"));
   cerr << timer;
   assert(f.size() == 0);
   timer.process();
   cerr << "Immediately after Fifo size: " << f.size() << endl;
   assert(f.size() == 0);

   sleep(1);
   timer.process();
   assert(f.size() == 1);

   Message* msg = f.getNext();

   cerr << *msg << endl;

   delete msg;
   return 0;
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
