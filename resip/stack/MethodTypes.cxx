#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <string.h>
#include <cstdio>
#include "rutil/ResipAssert.h"

#include "resip/stack/MethodTypes.hxx"
#include "resip/stack/Symbols.hxx"
#include "rutil/Data.hxx"

using namespace resip;

#define defineMethod(_enum, _name, _rfc) _name

// !dlb! this file will be subsumed by MethodHash.cxx on auto-generate
// move into MethodHash.cxx -- generated from MethodTypes.hxx -- !ah! indeed, yuck!
namespace resip{

Data MethodNames[] = 
{
   defineMethod(UNKNOWN, "UNKNOWN", "NA"),
   defineMethod(ACK, "ACK", "RFC ????"),
   defineMethod(BYE, "BYE", "RFC ????"),
   defineMethod(CANCEL, "CANCEL", "RFC ????"),
   defineMethod(INVITE, "INVITE", "RFC ????"),
   defineMethod(NOTIFY, "NOTIFY", "RFC ????"),
   defineMethod(OPTIONS, "OPTIONS", "RFC ????"),
   defineMethod(REFER, "REFER", "RFC ????"),
   defineMethod(REGISTER, "REGISTER", "RFC ????"),
   defineMethod(SUBSCRIBE, "SUBSCRIBE", "RFC ????"),
   defineMethod(RESPONSE, "RESPONSE", "RFC ????"),
   defineMethod(MESSAGE, "MESSAGE", "RFC ????"),
   defineMethod(INFO, "INFO", "RFC ????"),
   defineMethod(PRACK, "PRACK", "RFC ????"),
   defineMethod(PUBLISH, "PUBLISH", "RFC ????"),
   defineMethod(SERVICE, "SERVICE", "!RFC"),
   defineMethod(UPDATE,"UPDATE", "RFC ????")
};
}
#include "MethodHash.hxx"

const Data&
resip::getMethodName(MethodTypes t) 
{
    if (t < UNKNOWN ||  t >= MAX_METHODS) 
        t=UNKNOWN;
    return MethodNames[t];
}

MethodTypes
resip::getMethodType(const Data& name)
{
   // note: use data to prevent copying shared data
   return getMethodType(name.data(), (int)name.size());
}

MethodTypes
resip::getMethodType(const char* name, int len)
{
   const struct methods* m = MethodHash::in_word_set(name, len);
   return m ? m->type : UNKNOWN;
}

// ?dlb? why aren't we using the lib strncasecmp?
int strncasecmp(const char* a, const char* b, int len)
{
    //!ah! whoever implemented this should be shot.
    //!ah! should use library based strncasecmp() !
    //!ah! have fixed it up a bit.
   for (int i = 0; i < len; i++)
   {
      int c = tolower(a[i]) - tolower(b[i]);
      if (c) return c;
   }
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
