#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/Socket.hxx"

#if defined( WIN32 )
#  include <windows.h>
#  include <winbase.h>
#else
#  include <sys/time.h>
#  include <sys/syscall.h>
#  include <unistd.h>
#endif

#include "rutil/ResipAssert.h"
#include "rutil/Time.hxx"
#include "rutil/Timer.hxx"
#include "rutil/Logger.hxx"


#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;

unsigned long
resip::Timer::T1 = 500;

unsigned long
resip::Timer::T2 = 8 * T1;

unsigned long
resip::Timer::T4 = 10 * T1;

unsigned long
resip::Timer::T100 = 80;

unsigned long
resip::Timer::TB = 64*T1;

unsigned long
resip::Timer::TD = 32000;

unsigned long
resip::Timer::TC = 3*60*1000;

unsigned long
resip::Timer::TF = 64*T1;

unsigned long
resip::Timer::TH = 64*T1;

unsigned long
resip::Timer::TS = 32000;

unsigned long
resip::Timer::TcpConnectTimeout = 0;  // disabled

void 
Timer::resetT1(unsigned long t1)
{
   T1 = t1;
   T2 = 8 * T1;
   T4 = 10 * T1;
   TB = 64*T1;
   TF = 64*T1;
   TH = 64*T1;
}

Data
Timer::toData(Type timer)
{
   switch (timer)
   {
      case TimerA: // doubling
         return "Timer A";
      case TimerB:
         return "Timer B";
      case TimerC:
         return "Timer C";
      case TimerD:
         return "Timer D";
      case TimerE1:
         return "Timer E1";
      case TimerE2:
         return "Timer E2";
      case TimerF:
         return "Timer F";
      case TimerG: 
         return "Timer G";
      case TimerH:
         return "Timer H";
      case TimerI:
         return "Timer I";
      case TimerJ:
         return "Timer J";
      case TimerK:
         return "Timer K";
      case TimerTrying:
         return "Timer Trying";
      case TimerStaleClient:
         return "Timer StaleClient";
      case TimerStaleServer:
         return "Timer StaleServer";
      case TimerStateless:
         return "Timer Stateless";
      case TimerCleanUp:
         return "Timer Cleanup";
      case TcpConnectTimer:
          return "TcpConnectTimer";
      default:
         resip_assert(0);
   }
   return "Bad Bad Bad in timer";
}


TransactionTimer::TransactionTimer(unsigned long ms, 
                                    Timer::Type type, 
                                    const Data& transactionId) :
   mWhen(ms + Timer::getTimeMs()),
   mType(type),
   mTransactionId(transactionId),
   mDuration(ms)
{}

std::ostream& 
TransactionTimer::encode(std::ostream& str) const
{
   UInt64 now(Timer::getTimeMs());
   str << "TransactionTimer[ when=" << mWhen << " rel=";
   if (mWhen < now)
   {
      str << "past";
   }
   else
   {
      str << (mWhen - now);
   }
   str << "]";
   return str;
}

#ifndef RESIP_USE_STL_STREAMS
EncodeStream& 
TransactionTimer::encode(EncodeStream& str) const
{
   UInt64 now(Timer::getTimeMs());
   str << "TransactionTimer[ when=" << mWhen << " rel=";
   if (mWhen < now)
   {
      str << "past";
   }
   else
   {
      str << (mWhen - now);
   }
   str << "]";
   return str;
}
#endif

TimerWithPayload::TimerWithPayload(unsigned long ms, Message* message) :
   mWhen(ms + Timer::getTimeMs()),
   mMessage(message)
{
   resip_assert(mMessage);
}

std::ostream& 
TimerWithPayload::encode(std::ostream& str) const
{
   UInt64 now(Timer::getTimeMs());
   str << "TimerWithPayload[ when=" << mWhen << " rel=";
   if (mWhen < now)
   {
      str << "past";
   }
   else
   {
      str << (mWhen - now);
   }
   str << "]";
   return str;
}

#ifndef RESIP_USE_STL_STREAMS
EncodeStream& 
TimerWithPayload::encode(EncodeStream& str) const
{
   UInt64 now(Timer::getTimeMs());
   str << "TimerWithPayload[ when=" << mWhen << " rel=";
   if (mWhen < now)
   {
      str << "past";
   }
   else
   {
      str << (mWhen - now);
   }
   str << "]";
   return str;
}
#endif


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
