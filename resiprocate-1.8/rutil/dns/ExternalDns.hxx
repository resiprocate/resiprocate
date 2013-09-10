#if !defined(RESIP_EXTERNAL_DNS_HXX)
#define RESIP_EXTERNAL_DNS_HXX

#include <vector>

#include "rutil/AsyncID.hxx"
#include "rutil/GenericIPAddress.hxx"

struct hostent;

namespace resip
{
class ExternalDnsHandler;
class ExternalDnsRawResult;
class ExternalDnsHostResult;
class FdPollGrp;

//used by the asynchronous executive
class ExternalDns
{
   public:
      enum Features
      {
         None = 0,
         TryServersOfNextNetworkUponRcode3 = 1 << 0   // 'No such name'
      };

      //returns Success, BuildMismatch, otherwise ExternalDns specific 
      //error message can be pulled from errorMessage
      enum InitResult 
      {
         Success = 0,
         BuildMismatch = 4777
      };      
      

      //
      virtual int init(const std::vector<GenericIPAddress>& additionalNameservers,
                       AfterSocketCreationFuncPtr,
                       int dnsTimeout = 0,
                       int dnsTries = 0,
                       unsigned int features = 0) = 0; // bit mask of Features

      //returns 'true' only is there are changes in the DNS server list
      virtual bool checkDnsChange() = 0;

      //For use in select timeout
      virtual unsigned int getTimeTillNextProcessMS() = 0;

      //this is scary on windows; the standard way to get a bigger fd_set is to
      //redefine FD_SETSIZE befor each inclusion of winsock2.h, so make sure
      //external libraries have been properly configured      
      // MUST not be called when pollGrp (below) is active
      virtual void buildFdSet(fd_set& read, fd_set& write, int& size) = 0;
      virtual void process(fd_set& read, fd_set& write) = 0;

      virtual void setPollGrp(FdPollGrp *grp) = 0;

      // called by DnsStub to process timers requested by getTimeTillNext...()
      // only used when poll group is active
      virtual void processTimers() = 0;

      virtual void freeResult(ExternalDnsRawResult res) = 0;
      virtual void freeResult(ExternalDnsHostResult res) = 0;

      //caller must clean up memory
      virtual char* errorMessage(long errorCode) = 0;
      
      virtual ~ExternalDns()  {}

      virtual void lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData) = 0;

      virtual bool hostFileLookup(const char* target, in_addr &addr) = 0;
      virtual bool hostFileLookupLookupOnlyMode() = 0;
};
 
class ExternalDnsResult : public AsyncResult
{
   public:
      ExternalDnsResult(long errorCode, void* uData) : AsyncResult(errorCode) , userData(uData) {}
      ExternalDnsResult(void* uData) : userData(uData) {}
      void* userData;
};

//should this be nested?
class ExternalDnsRawResult : public ExternalDnsResult
{
   public:
      ExternalDnsRawResult(unsigned char* buf, int len, void* uData) : 
         ExternalDnsResult(uData),

         abuf(buf),
         alen(len) 
      {}
      ExternalDnsRawResult(long errorCode, unsigned char* buf, int len, void* uData) : 
         ExternalDnsResult(errorCode, uData),
         abuf(buf),
         alen(len)
         {}
         
      unsigned char* abuf;
      int alen;
};

class ExternalDnsHostResult : public ExternalDnsResult
{
   public:
      ExternalDnsHostResult(hostent* h, void* uData) :
         ExternalDnsResult(uData), 
         host(h)
      {}
      ExternalDnsHostResult(long errorCode, void* uData) : ExternalDnsResult(errorCode, uData) {}
         
      hostent* host;
};

class ExternalDnsHandler
{
   public:
      //underscores are against convention, but pretty impossible to read
      //otherwise. ?dcm? -- results stack or heap? 
      //the free routines can be dealt w/ iheritence instead if pointers are used
      //virtual void handle_NAPTR(ExternalDnsRawResult res) = 0;
      //virtual void handle_SRV(ExternalDnsRawResult res) = 0;
      //virtual void handle_AAAA(ExternalDnsRawResult res) = 0;
      //virtual void handle_host(ExternalDnsHostResult res) = 0;

      // new version
      virtual ~ExternalDnsHandler() {}
      virtual void handleDnsRaw(ExternalDnsRawResult res) = 0;
};

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
