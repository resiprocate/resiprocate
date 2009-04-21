#if !defined(RESIP_SUBSYSTEM_HXX)
#define RESIP_SUBSYSTEM_HXX 

#include <iostream>
#include "rutil/Data.hxx"
#include "rutil/Log.hxx"

namespace resip
{

/**
   @brief Class used to specify what sub-system given sections of code belong
      to, for use by the logging system.

   @note The logging macros defined in Logger.hxx assume that the preprocessor
      macro RESIPROCATE_SUBSYSTEM is defined to an instance of this class. The
      logging macro uses this object to determine what logging level should be
      used, and what sub-system the logging statement should come from. So, in
      your code you might do something like the following:

      @code
      #define RESIPROCATE_SUBSYSTEM Subsystem::APP
      //...
      void doStuff()
      {
         DebugLog(<< "Doing some stuff...");
      }
      @endcode

      This would cause your log statement to be marked as coming from 
      Subsystem::APP.
*/
class Subsystem 
{
   public:
      // Add new systems below
      static Subsystem APP;
      static Subsystem CONTENTS;
      static Subsystem DNS;
      static Subsystem DUM;
      static Subsystem NONE; // default subsystem
      static Subsystem PRESENCE; 
      static Subsystem SDP;
      static Subsystem SIP;    // SIP Stack / Parser
      static Subsystem TEST;   
      static Subsystem TRANSACTION;
      static Subsystem TRANSPORT;
      static Subsystem STATS;
      static Subsystem REPRO;
      
      const Data& getSubsystem() const;
      Log::Level getLevel() const { return mLevel; }
      void setLevel(Log::Level level) { mLevel = level; }
   protected:
      explicit Subsystem(const char* rhs) : mSubsystem(rhs), mLevel(Log::None) {};
      explicit Subsystem(const Data& rhs) : mSubsystem(rhs), mLevel(Log::None) {};
      Subsystem& operator=(const Data& rhs);

      Data mSubsystem;
      Log::Level mLevel;

      friend EncodeStream& operator<<(EncodeStream& strm, const Subsystem& ss);
};


// in order to have subsystems in your application, subclass from this class
/*
#include "rutil/Data.hxx"
#include "rutil/Subsystem.hxx"

namespace MyNamespace
{

class Subsystem : public resip::Subsystem
{
   public:
      // Add new systems below
      static const Subsystem SPECIAL_SUBSYSTEM;

   private:
      explicit Subsystem(const char* rhs) : resip::Subsystem(rhs) {};
      explicit Subsystem(const resip::Data& rhs);
      Subsystem& operator=(const resip::Data& rhs);
};
 
}
*/
 
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
