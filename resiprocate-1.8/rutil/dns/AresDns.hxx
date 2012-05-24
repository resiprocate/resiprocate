#if !defined(RESIP_ARES_DNS_HXX)
#define RESIP_ARES_DNS_HXX

#include "rutil/FdSetIOObserver.hxx"
#include "rutil/GenericIPAddress.hxx"
#include "rutil/dns/ExternalDns.hxx"

extern "C"
{
struct ares_channeldata;
}

//struct fd_set;

extern "C" {
  void resip_AresDns_aresCallback(void *arg, int status, unsigned char* abuf, int alen);
  void resip_AresDns_caresCallback(void *arg, int status, int timeouts, unsigned char* abuf, int alen);
}

namespace resip
{
class AresDnsPollItem;
class FdPollGrp;

class AresDns : public ExternalDns, public FdSetIOObserver
{
   friend class AresDnsPollItem;
   public:
      AresDns() {mChannel = 0; mFeatures = 0; mPollGrp=NULL;}
      virtual ~AresDns();

      virtual int init(const std::vector<GenericIPAddress>& additionalNameservers,
                       AfterSocketCreationFuncPtr socketfunc, int timeout=0, int tries=0, unsigned int features=0);

      int internalInit(const std::vector<GenericIPAddress>& additionalNameservers,
                       AfterSocketCreationFuncPtr socketfunc, unsigned int features=0, ares_channeldata** channel = 0, int timeout=0, int tries=0);

      virtual bool checkDnsChange();

      virtual unsigned int getTimeTillNextProcessMS();
      virtual void process(FdSet& fdset);
      virtual void buildFdSet(FdSet& fdset);

      virtual void buildFdSet(fd_set& read, fd_set& write, int& size);
      virtual void process(fd_set& read, fd_set& write);

      virtual void setPollGrp(FdPollGrp *pollGrp);
      virtual void processTimers();

      //?dcm?  I believe these need to do nothing in the ARES case.
      virtual void freeResult(ExternalDnsRawResult /* res */) {}
      virtual void freeResult(ExternalDnsHostResult /* res */) {}

      virtual char* errorMessage(long errorCode);

      void lookup(const char* target, unsigned short type, ExternalDnsHandler* handler, void* userData);

      virtual bool hostFileLookup(const char* target, in_addr &addr);
      virtual bool hostFileLookupLookupOnlyMode() { return mHostFileLookupOnlyMode; }
      static void enableHostFileLookupOnlyMode(bool enable) { mHostFileLookupOnlyMode = enable; }

      friend void ::resip_AresDns_aresCallback(void *arg, int status, unsigned char* abuf, int alen);
      friend void ::resip_AresDns_caresCallback(void *arg, int status, int timeouts, unsigned char* abuf, int alen);

      // used for epoll() interface to ares lib
      // time_t mNow;

   private:

      typedef std::pair<ExternalDnsHandler*, void*> Payload;
      static ExternalDnsRawResult makeRawResult(void *arg, int status, unsigned char *abuf, int alen);
      static ExternalDnsHandler* getHandler(void* arg);
      struct ares_channeldata* mChannel;
      std::vector<GenericIPAddress> mAdditionalNameservers;
      unsigned int mFeatures;
      volatile static bool mHostFileLookupOnlyMode;

      FdPollGrp*	mPollGrp;
      std::vector<AresDnsPollItem*> mPollItems;

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
 * vi: set shiftwidth=3 expandtab:
 */
