#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/Logger.hxx"

#include "resiprocate/DnsInterface.hxx"
#include "resiprocate/DnsResult.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

const Data DnsInterface::UdpOnly[] =
{
   Data("SIP+D2U"),
   Data::Empty
};

const Data DnsInterface::TcpAndUdp[] =
{
   Data("SIP+D2T"),
   Data("SIP+D2U"),
   Data::Empty
};

const Data DnsInterface::AllTransports[] =
{
   Data("SIPS+D2T"),
   Data("SIP+D2T"),
   Data("SIP+D2U"),
   Data::Empty
};

DnsInterface::DnsInterface() 
   : mHandler(0),
     mSupportedTransports(&TcpAndUdp)
{
#if defined(USE_ARES)
   int status=0;
   if ((status = ares_init(&mChannel)) != ARES_SUCCESS)
   {
      ErrLog (<< "Failed to initialize async dns library (ares)");
      char* errmem=0;
      ErrLog (<< ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
      throw Exception("failed to initialize ares", __FILE__,__LINE__);
   }
#endif
}

DnsInterface::DnsInterface(DnsInterface::Handler* handler)
   : mHandler(handler),
     mSupportedTransports(&TcpAndUdp)
{
#if defined(USE_ARES)
   int status=0;
   if ((status = ares_init(&mChannel)) != ARES_SUCCESS)
   {
      ErrLog (<< "Failed to initialize async dns library (ares)");
      char* errmem=0;
      ErrLog (<< ares_strerror(status, &errmem));
      ares_free_errmem(errmem);
      throw Exception("failed to initialize ares", __FILE__,__LINE__);
   }
#endif
}

DnsInterface::~DnsInterface()
{
}

void 
DnsInterface::setSupportedTransports(TransportArray& transports)
{
   mSupportedTransports = &transports;
}

bool
DnsInterface::isSupported(const Data& service)
{
   for (int i=0; !mSupportedTransports[i]->empty(); i++)
   {
      if (service == (*mSupportedTransports)[i]) 
      {
         return true;
      }
   }
   return false;
}

void 
DnsInterface::buildFdSet(FdSet& fdset)
{
#if defined(USE_ARES)
   int size = ares_fds(mChannel, &fdset.read, &fdset.write);
   if ( size > fdset.size )
   {
      fdset.size = size;
   }
#endif
}

void 
DnsInterface::process(FdSet& fdset)
{
#if defined(USE_ARES)
   ares_process(mChannel, &fdset.read, &fdset.write);
#endif
}


DnsResult*
DnsInterface::lookup(const Uri& uri, const Data& transactionId)
{
   DnsResult* result = new DnsResult(*this);
   result->lookup(uri, transactionId);
   return result;
}

DnsResult* 
DnsInterface::lookup(const Via& via, const Data& transactionId)
{
   assert(0);
   //DnsResult* result = new DnsResult(*this);
   return NULL;
}


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
