#if !defined(RESIP_DNSINTERFACE_HXX)
#define RESIP_DNSINTERFACE_HXX 

#if defined(USE_ARES)
extern "C"
{
#include "ares.h"
#include "ares_dns.h"
}
#endif

#include <set>

#include "resiprocate/Transport.hxx"
#include "resiprocate/os/Data.hxx"

namespace resip
{
class DnsResult;
class Uri;
class Via;

// 
class DnsInterface
{
   public:
      class Handler
      {
         public:
            // call when dns entries are available (or nothing found)
            // this may be called synchronously with the call to lookup
            virtual void handle()=0;
      };

      // Used to create a synchronous Dns Interface. Any lookup requests passed
      // to this DnsInterface will be completely processed and returned to the caller
      DnsInterface();

      // Used to create an asynchronous Dns Interface. Any lookup requests will
      // be queued for later processing. It is critical that the consumer of the
      // DnsResult be in the same thread that is processing the async results
      // since there is no locking on the DnsResult
      DnsInterface(DnsInterface::Handler* handler);
      virtual ~DnsInterface()=0;
      
      // set the supported set of types that a UAC wishes to use
      void setSupportedTypes(std::set<Transport::Type>& supported);
      
      // adds the appropriate file descriptors to the fdset to allow a
      // select/poll call to be made 
      void buildFdSet(FdSet& fdset);

      // process any dns results back from the async dns library (e.g. ares). If
      // there are results to report, post an event to the fifo
      void process(FdSet& fdset);
      
      // For each of the following calls, immediately return a DnsResult to the
      // caller. If synchronous, the DnsResult is complete and may block for an
      // arbitrary amount of time. In the synchronous case, the transactionId is
      // not useful. If asynchronous, the DnsResult will be returned immediately
      // and is owned by the caller. If queries are outstanding, it is not valid
      // for the caller to delete the DnsResult.
      // 
      // First determine a transport.  Second, determine a set of ports and ip
      // addresses. These can be returned to the client by asking the DnsResult
      // for the next result in the form of a Transport:Tuple. The client can
      // continue to ask the DnsResult to return more tuples. If the tuples for
      // the current transport are exhausted, move on to the next preferred
      // transport (if there is one)
      DnsResult* lookup(const Uri& url, const Data& transactionId=Data::Empty);
      DnsResult* lookup(const Via& via, const Data& transactionId=Data::Empty);
      
   protected: 
      // When complete or partial results are ready, call Handler::process()
      // For synchronous DnsInterface, set to 0
      Handler* mHandler;
      std::set<Transport::Type> mSupportedTransports;
      bool mSupportTLS;
      
#if defined(USE_ARES)
      ares_channel mChannel;
#endif

      friend class DnsResult;
};

}

#endif
/* 
   Copyright (c) 2003, Jason Fischl
 
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are
   met:
    
   * Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
   copyright notice, this list of conditions and the following
   disclaimer in the documentation and/or other materials provided
   with the distribution.
    
   * Neither the name of any of the copyright holders nor the names of
   its contributors may be used to endorse or promote products
   derived from this software without specific prior written
   permission.
    
   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
   LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
   DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
   THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
   (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
