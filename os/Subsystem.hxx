#ifndef resip_Subsystem_hxx
#define resip_Subsystem_hxx

#include <iostream>
#include "resiprocate/os/Data.hxx"

namespace resip
{

class Subsystem 
{
   public:
      // Add new systems below
      static const Subsystem APP;
      static const Subsystem CONTENTS;
      static const Subsystem TEST;   
      static const Subsystem NONE; // default subsystem
      static const Subsystem UTIL;
      static const Subsystem BASE;
      static const Subsystem SIP;    // SIP Stack / Parser
      static const Subsystem SDP;
      static const Subsystem TRANSACTION;
      static const Subsystem TRANSPORT;
      
   protected:
      Subsystem(const char* rhs) : mSubsystem(rhs) {};
      Subsystem(const Data& rhs);
      Subsystem& operator=(const Data& rhs);

      Data mSubsystem;

      friend std::ostream& operator<<(std::ostream& strm, const Subsystem& ss);
};


// in order to have subsystems in your application, subclass from this class
/*
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Subsystem.hxx"

namespace MyNamespace
{

class Subsystem : public resip::Subsystem
{
   public:
      // Add new systems below
      static const Subsystem SPECIAL_SUBSYSTEM;

   private:
      Subsystem(const char* rhs) : resip::Subsystem(rhs) {};
      Subsystem(const resip::Data& rhs);
      Subsystem& operator=(const resip::Data& rhs);
};
 
}
*/

 
}

#endif // Subsystem.hxx
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
