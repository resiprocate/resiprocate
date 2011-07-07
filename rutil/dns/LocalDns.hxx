#if !defined(RESIP_LOCAL_DNS_HXX)
#define RESIP_LOCAL_DNS_HXX

#include <map>
#include "rutil/Data.hxx"
#include "rutil/dns/ExternalDns.hxx"

extern "C"
{
struct ares_channeldata;
}


namespace resip
{
class LocalDns : public ExternalDns
{
   public:
      LocalDns();
      virtual ~LocalDns();

      virtual int init(); 
      virtual unsigned int getTimeTillNextProcessMS();
      virtual void buildFdSet(fd_set& read, fd_set& write, int& size);
      virtual void process(fd_set& read, fd_set& write);

      virtual void freeResult(ExternalDnsRawResult /* res */) {}
      virtual void freeResult(ExternalDnsHostResult /* res */) {}

      void lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData);

      virtual void lookupARecords(const char* target, ExternalDnsHandler* handler, void* userData) {}
      virtual void lookupAAAARecords(const char* target, ExternalDnsHandler* handler, void* userData) {}
      virtual void lookupNAPTR(const char* target, ExternalDnsHandler* handler, void* userData) {}
      virtual void lookupSRV(const char* target, ExternalDnsHandler* handler, void* userData) {}
      virtual char* errorMessage(long errorCode) 
      {
         const char* msg = "Local Dns";

         int len = (int)strlen(msg);
         char* errorString = new char[len+1];

         strncpy(errorString, msg, len);
         errorString[len] = '\0';
         return errorString;
      }

   private:

	   struct ares_channeldata* mChannel;

      typedef std::pair<ExternalDnsHandler*, void*> Payload;
      static ExternalDnsRawResult makeRawResult(void *arg, int status, unsigned char *abuf, int alen);
      static ExternalDnsHandler* getHandler(void* arg);

      static void localCallback(void *arg, int status, unsigned char* abuf, int alen);

      static void message(const char* file, unsigned char* buf, int& len);
      static std::map<Data, Data> files;
      static Data mTarget;
};
   
}

#endif

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
