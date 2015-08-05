#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif
#endif

#include "rutil/compat.hxx"
#include "rutil/Logger.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/Socket.hxx"

#include "rutil/dns/DnsStub.hxx"
#include "rutil/dns/RRVip.hxx"
#include "resip/stack/DnsInterface.hxx"
#include "rutil/dns/DnsHandler.hxx"
#include "resip/stack/DnsResult.hxx"

//#include "rutil/dns/ExternalDnsFactory.hxx"
#include "rutil/WinLeakCheck.hxx"


using namespace resip;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

DnsInterface::DnsInterface(DnsStub& dnsStub) : 
   mUdpOnlyOnNumeric(false),
   mDnsStub(dnsStub)
{
#ifdef USE_DNS_VIP
   mDnsStub.setResultTransform(&mVip);
#endif
}

DnsInterface::~DnsInterface()
{
}

void 
DnsInterface::addTransportType(TransportType type, IpVersion version)
{
   Lock lock(mSupportedMutex);
   mSupportedTransports[std::make_pair(type, version)]++;
   const Data* pNaptrType = getSupportedNaptrType(type);
   if(pNaptrType)
   {
       mSupportedNaptrs[*pNaptrType]++;
   }
   //logSupportedTransports();
}

void 
DnsInterface::removeTransportType(TransportType type, IpVersion version)
{
   Lock lock(mSupportedMutex);
   TransportMap::iterator itTrans = mSupportedTransports.find(std::make_pair(type, version));
   if(itTrans != mSupportedTransports.end())
   {
      // Remove from map if ref count hits zero
      if(--itTrans->second == 0)
      {
         mSupportedTransports.erase(itTrans);
      }
   }

   const Data* pNaptrType = getSupportedNaptrType(type);
   if(pNaptrType)
   {
      SupportedNaptrMap::iterator itNT = mSupportedNaptrs.find(*pNaptrType);
      if(itNT != mSupportedNaptrs.end())
      {
         // Remove from map if ref count hits zero
         if(--itNT->second == 0)
         {
            mSupportedNaptrs.erase(itNT);
         }
      }
   }
   //logSupportedTransports();
}

static Data UdpNAPTRType("SIP+D2U");
static Data TcpNAPTRType("SIP+D2T");
static Data TlsNAPTRType("SIPS+D2T");
static Data DtlsNAPTRType("SIPS+D2U");
static Data WsNAPTRType("SIP+D2W");
static Data WssNAPTRType("SIPS+D2W");
const Data* 
DnsInterface::getSupportedNaptrType(TransportType type)
{
   Data* pNaptrType = 0;
   switch (type)
   {
   case UDP:
      pNaptrType = &UdpNAPTRType;
      break;
   case TCP:
      pNaptrType = &TcpNAPTRType;
      break;
   case TLS:
      pNaptrType = &TlsNAPTRType;
      break;
   case DTLS:
      pNaptrType = &DtlsNAPTRType;
      break;
   case WS:
      pNaptrType = &WsNAPTRType;
      break;
   case WSS:
      pNaptrType = &WssNAPTRType;
      break;
   default:
      resip_assert(0);
      break;
   }
   return pNaptrType;
}

void 
DnsInterface::logSupportedTransports()
{
   TransportMap::iterator itTrans = mSupportedTransports.begin();
   for(; itTrans != mSupportedTransports.end(); itTrans++)
   {
      InfoLog(<< "logSupportedTransports: mSupportedTransports[" << toData(itTrans->first.first) << "," << (itTrans->first.second == V4 ? "V4" : "V6") << "] = " << itTrans->second);
   }

   SupportedNaptrMap::iterator itNT = mSupportedNaptrs.begin();
   for(; itNT != mSupportedNaptrs.end(); itNT++)
   {
      InfoLog(<< "logSupportedTransports: mSupportedNaptrs[" << itNT->first << "] = " << itNT->second);
   }
}

bool
DnsInterface::isSupported(const Data& service)
{
   Lock lock(mSupportedMutex);
   return mSupportedNaptrs.count(service) != 0;
}

bool
DnsInterface::isSupported(TransportType t, IpVersion version)
{
   Lock lock(mSupportedMutex);
   return mSupportedTransports.find(std::make_pair(t, version)) != mSupportedTransports.end();
}

bool
DnsInterface::isSupportedProtocol(TransportType t)
{
   Lock lock(mSupportedMutex);
   for (TransportMap::const_iterator i=mSupportedTransports.begin(); i != mSupportedTransports.end(); ++i)
   {
      if (i->first.first == t)
      {
         return true;
      }
   }
   return false;
}

int DnsInterface::supportedProtocols()
{
   Lock lock(mSupportedMutex);
   return (int)mSupportedTransports.size();
}

DnsResult*
DnsInterface::createDnsResult(DnsHandler* handler)
{
   DnsResult* result = new DnsResult(*this, mDnsStub, mVip, handler);
   return result;
}

void 
DnsInterface::lookup(DnsResult* res, const Uri& uri)
{
   res->lookup(uri);
}

//?dcm? -- why is this here?
DnsHandler::~DnsHandler()
{
}


//  Copyright (c) 2003, Jason Fischl 
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

