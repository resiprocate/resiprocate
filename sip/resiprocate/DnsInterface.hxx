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
#include "resiprocate/os/BaseException.hxx"

namespace resip
{
class DnsHandler;
class DnsResult;
class TransactionState;
class Uri;
class Via;
   
// 
class DnsInterface
{
   public:
      typedef const Data (TransportArray)[Transport::MAX_TRANSPORT];

      // These are sets of transports that can be passed to the DnsInterface on
      // construction to indicate which transports a client supports
      static TransportArray UdpOnly;       // UDP
      static TransportArray TcpAndUdp;     // TCP, UDP (DEFAULT)
      static TransportArray AllTransports; // TCP, UDP, TLS

      class Exception : public BaseException
      {
         public:
            Exception(const Data& msg, const Data& file, const int line) : BaseException(msg,file,line){}
            const char* name() const { return "DnsInterface::Exception"; }
      };

      // Used to create an asynchronous Dns Interface. Any lookup requests will
      // be queued for later processing. It is critical that the consumer of the
      // DnsResult be in the same thread that is processing the async results
      // since there is no locking on the DnsResult
      // Will throw DnsInterface::Exception if ares fails to initialize
      DnsInterface(bool synchronous=false);
      virtual ~DnsInterface();
      
      // set the supported set of types that a UAC wishes to use
      void setSupportedTransports(const TransportArray& transports);
      
      // return if the client supports the specified service (e.g. SIP+D2T)
      bool isSupported(const Data& service);
      
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
      DnsResult* lookup(const Uri& url, DnsHandler* handler=0);
      DnsResult* lookup(const Via& via, DnsHandler* handler=0);
      
   protected: 
      // When complete or partial results are ready, call DnsHandler::process()
      // For synchronous DnsInterface, set to 0
      DnsHandler* mHandler;
      TransportArray* mSupportedTransports;
      
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
