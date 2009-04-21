#include <iostream>
#include "assert.h"

#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char* argv[])
{
   Log::Level l = Log::Debug;
   Log::initialize(Log::Cout, l, argv[0]);
   
   {
      resipCerr << "Network Interfaces: " << endl << Inserter(DnsUtil::getInterfaces()) << endl << endl;
   }
   
   {
      Data addr("1:1");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("1:1:192.168.2.233");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("1:1:::::");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("1:1::::::168.192.2.233");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("5f1b:df00:ce3e:e200:20:800:2b37:6426");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("5f1b:df00:ce3e:e200:20:800:2b37:6426:121.12.131.12");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("192.168.2.233");
      resipCerr << "!! "<< addr << endl;
      assert(!DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("u@a.tv:1290");
      resipCerr << "!! "<< addr << endl;
      assert(!DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("::1");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("::");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data addr("FF01::43");
      resipCerr << "!! "<< addr << endl;
      assert(DnsUtil::isIpV6Address(addr));
   }

   {
      Data c("apple:5060");
      Data addr(Data::Share, c.c_str(), 5);
      resipCerr << "!! " << addr << endl;
      assert(!DnsUtil::isIpV6Address(addr));
   }
   
   {
      Data addr(":zzz");
      if(DnsUtil::isIpV6Address(addr))
      {
         DnsUtil::canonicalizeIpV6Address(addr);
      }
   }

   resipCerr << "All OK" << endl;
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
