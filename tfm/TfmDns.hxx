#if !defined(RESIP_TFM_DNS_HXX)
#define RESIP_TFM_DNS_HXX

#include <list>
#include <map>
#include "rutil/dns/ares/ares.h"
#include "rutil/Data.hxx"
#include "rutil/dns/ExternalDns.hxx"

namespace resip
{
class TfmDns : public ExternalDns
{
   public:
      class Host
      {
         public:
            Host(const Data& ip)
               : mIp(ip)
            {
            }

            Data mIp;
      };

      struct SRV
      {
         public:
            SRV(int priority, int weight, int port, const Data& target)
               : mPriority(priority),
                 mWeight(weight),
                 mPort(port),
                 mTarget(target)
            {
            }

            int mPriority;
            int mWeight;
            int mPort;
            Data mTarget;
      };

      TfmDns();
      virtual ~TfmDns();

      virtual int init(const std::vector<GenericIPAddress>& additionalNameservers,
                       AfterSocketCreationFuncPtr socketfunc, int dnsTimeout = 0, int dnsTries = 0,
                       unsigned int features = 0);
      virtual bool requiresProcess();
      virtual unsigned int getTimeTillNextProcessMS(void) { return 0; }
      // Synchronous, so no need to do anything.
      virtual void setPollGrp(resip::FdPollGrp *) {}
      virtual void processTimers(void) {}
      virtual bool hostFileLookupLookupOnlyMode(void) { return false; }
      virtual bool checkDnsChange() { return false; }
      virtual void buildFdSet(fd_set& read, fd_set& write, int& size);
      virtual void process(fd_set& read, fd_set& write);

      virtual void freeResult(ExternalDnsRawResult /* res */) {}
      virtual void freeResult(ExternalDnsHostResult /* res */) {}

      virtual bool hostFileLookup(const char* target, in_addr &addr) { return false; }

      void lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData);

      virtual char* errorMessage(long errorCode) 
      {
         const char* msg = ares_strerror(errorCode);

         size_t len = strlen(msg);
         char* errorString = new char[len+1];

         strncpy(errorString, msg, len);
         errorString[len] = '\0';
         return errorString;
      }

      void addSrvs(const resip::Data&, const std::list<SRV>&);
      void addHosts(const resip::Data&, const std::list<Host>&);

   private:
      typedef std::list<Host> Hosts;
      typedef std::list<SRV> SRVs;

      typedef std::map<Data, Hosts> HostMap;
      HostMap mHosts;

      typedef std::map<Data, SRVs> SRVMap;
      SRVMap mSRVs;

      unsigned char mResponse[512];
      int mResponseLength;

      void setup();
      void makeSrvResponse(const char* target, const std::list<SRV>&);
      void makeAResponse(const char* target, const std::list<Host>&);
      void lookupA(const char*, ExternalDnsHandler* handler, void* userData);
      void lookupSrv(const char*, ExternalDnsHandler* handler, void* userData);
      int computeLength(const Data& name);
      int computeSrvDataLength(const Data& target);

      static unsigned short getNextTransactionId() { return mTransactionId++; }
      static unsigned short mTransactionId;

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
