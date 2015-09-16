#include <fstream>

#include "rutil/dns/LocalDns.hxx"
#include "rutil/WinLeakCheck.hxx"

#include "ares.h"
#include "ares_dns.h"
#ifdef WIN32
#undef write  // Note:  ares.h defines write to be _write for WIN32 - we don't want that here, since we use fdset.write and stream write
#endif

#include "AresCompat.hxx"

#if !defined(WIN32)
#include <arpa/nameser.h>
#endif

using namespace resip;
using namespace std;

std::map<Data, Data> LocalDns::files;
Data LocalDns::mTarget;

int 
LocalDns::init()
{
   int status;
   if ((status = ares_init(&mChannel)) != ARES_SUCCESS)
   {
      return status;
   }
   else
   {
      return 0;
   }
}

LocalDns::LocalDns()
{
   files["yahoo.com"] = "yahoo.dns";
   files["demo.xten.com"] = "demo.naptr";
   files["_ldap._tcp.openldap.org"] = "openldap.srv";
   files["quartz"] = "quartz.aaaa";
   files["crystal"] = "crystal.aaaa";
   files["www.google.com"] = "google.cname";
}

LocalDns::~LocalDns()
{
}

ExternalDnsHandler* 
LocalDns::getHandler(void* arg)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   ExternalDnsHandler *thisp = reinterpret_cast<ExternalDnsHandler*>(p->first);
   return thisp;
}

ExternalDnsRawResult 
LocalDns::makeRawResult(void *arg, int status, unsigned char *abuf, int alen)
{
   Payload* p = reinterpret_cast<Payload*>(arg);
   void* userArg = reinterpret_cast<void*>(p->second);
   
   if (status != ARES_SUCCESS)
   {
      return ExternalDnsRawResult(status, abuf, alen, userArg);
   }
   else
   {
      return ExternalDnsRawResult(abuf, alen, userArg);
   }
}
      
unsigned int
LocalDns::getTimeTillNextProcessMS()
{
   return 20; 
}

void 
LocalDns::buildFdSet(fd_set& read, fd_set& write, int& size)
{
}

void 
LocalDns::process(fd_set& read, fd_set& write)
{
}

void
LocalDns::lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData)
{
   mTarget = target;
   ares_query(mChannel, target, C_IN, type, LocalDns::localCallback, new Payload(handler, userData));
}

void LocalDns::message(const char* file, unsigned char* buf, int& len)
{
   len = 0;
   ifstream fs;
   fs.open(file, ios_base::binary | ios_base::in);
   resip_assert(fs.is_open());
   
   unsigned char* p = buf;
   
   while (!fs.eof())
   {
      unsigned char c = fs.get();
      if (c != char_traits<char>::eof())
      {
         *p++ = c;
         len++;
      }
   }
   
   fs.close();
}

void
LocalDns::localCallback(void *arg, int status, unsigned char *abuf, int alen)
{
   unsigned char msg[1024];
   int len = 0;
   map<Data, Data>::iterator it = files.find(mTarget);
   resip_assert(it != files.end());
   message(it->second.c_str(), msg, len);   
   resip_assert(0 != len);
   getHandler(arg)->handleDnsRaw(makeRawResult(arg, 0, msg, len));
   Payload* p = reinterpret_cast<Payload*>(arg);
   delete p;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
