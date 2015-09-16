#include "rutil/DnsUtil.hxx"
#include "tfm/DnsUtils.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Inserter.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/Tuple.hxx"
#include "resip/stack/Via.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/TestProxy.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

TestProxy::TestProxy(const Data& name,
                     const Data& host, 
                     const std::set<int>& udpPorts, 
                     const std::set<int>& tcpPorts, 
                     const std::set<int>& tlsPorts, 
                     const std::set<int>& dtlsPorts, 
                     const Data& interfaceObj)

   : mName(name)
{
   //mPort = (port == 0 ? PortAllocator::getNextPort() : port);

   bool proxyHasNameAddr=false;

   std::set<int>::const_iterator p;
   for(p=udpPorts.begin();p!=udpPorts.end();++p)
   {
      if(*p>0)
      {
         if(!proxyHasNameAddr)
         {
            mProxyUrl.uri().host() = host;
            mProxyUrl.uri().port() = *p;
            proxyHasNameAddr=true;
         }
         
         if(!interfaceObj.empty())
         {
            if(!DnsUtil::isIpAddress(interfaceObj))
            {
               try
               {
                  Source s(resip::DnsUtil::getLocalIpAddress(interfaceObj), *p, UDP);
                  addSource(s);
               }
               catch (resip::DnsUtil::Exception& e)
               {
                  DebugLog(<<"No external address found:" << e);
               }
            }
            else
            {
               Source s(interfaceObj,*p,UDP);
               addSource(s);
            }
         }
         else
         {
            addSources(DnsUtils::makeSourceSet(host,*p,UDP));
         }

      }
   }

   if(!interfaceObj.empty())
   {
      if(!DnsUtil::isIpAddress(interfaceObj))
      {
         try
         {
            Source s(resip::DnsUtil::getLocalIpAddress(interfaceObj), 5060, TCP);
            addSource(s);
         }
         catch (resip::DnsUtil::Exception& e)
         {
            DebugLog(<<"No external address found:" << e);
         }
      }
      else
      {
         Source s(interfaceObj,5060,TCP);
         addSource(s);
      }
   }
   else
   {
      addSources(DnsUtils::makeSourceSet(host,5060,TCP));
      
   }

   if(interfaceObj.empty() || !DnsUtil::isIpAddress(interfaceObj))
   {
      std::list<std::pair<Data, Data> > ifs(resip::DnsUtil::getInterfaces(interfaceObj));
      for(std::list<std::pair<Data, Data> >::iterator i=ifs.begin(); i!=ifs.end(); ++i)
      {
         Source s(i->second, 5060, SCTP);
         addSource(s);
      }
   }
   else
   {
      Source s(interfaceObj, 5060, SCTP);
      addSource(s);
   }
   
   if(!interfaceObj.empty())
   {
      if(!DnsUtil::isIpAddress(interfaceObj))
      {
         try
         {
            Source s(resip::DnsUtil::getLocalIpAddress(interfaceObj), 5061, TLS);
            addSource(s);
         }
         catch (resip::DnsUtil::Exception& e)
         {
            DebugLog(<<"No external address found:" << e);
         }
      }
      else
      {
         Source s(interfaceObj,5061,TLS);
         addSource(s);
      }
   }
   else
   {
      addSources(DnsUtils::makeSourceSet(host,5061,TLS));
   }

}

TestProxy::TestProxy()
{
   mPort = PortAllocator::getNextPort();
   mProxyUrl.uri().host() = resip::DnsUtil::getLocalHostName();
   mProxyUrl.uri().port() = mPort;

   Source s1(resip::DnsUtil::getLocalIpAddress(), mPort, UDP);
   addSource(s1);
   Source s2(resip::DnsUtil::getLocalIpAddress(), mPort, TCP);
   addSource(s2);
   Source s3(resip::DnsUtil::getLocalIpAddress(), mPort, TLS);
   addSource(s3);
}

TestProxy::~TestProxy()
{
}

void TestProxy::addSource(const Data& host, int port, TransportType transport)
{
   DebugLog(<< "TestProxy::addSource(" << host << ":" << port << " " << Tuple::toData(transport) << " for " << mName);
   Source s(host, port, transport);
   addSource(s);
}

void TestProxy::addSources(const set<Source>& sources)
{
   mSources.insert(sources.begin(), sources.end());
}

void TestProxy::addSource(const Source& src)
{
   DebugLog(<< "TestProxy::addSource(" << src << ") for " << mName);
   mSources.insert(src);
}


const resip::NameAddr& 
TestProxy::getProxyUrl() const
{
   return mProxyUrl;
}

const Data& 
TestProxy::getDomain() const
{
   return mProxyUrl.uri().host();
}

const int 
TestProxy::getPort() const
{
   return mProxyUrl.uri().port();
}

const resip::Uri& 
TestProxy::getUri() const
{
   return mProxyUrl.uri();
}

const resip::NameAddr&
TestProxy::getContact() const
{
   if (mContact.uri().host().empty())
   {
      mContact.displayName() = mName;
      mContact.uri() = getUri();
      if (!DnsUtil::isIpV4Address(mContact.uri().host()))
      {
         list<Data> ips = DnsUtil::lookupARecords(mContact.uri().host());
         resip_assert(!ips.empty());
         mContact.uri().host() = ips.front();
      }
   }

   return mContact;
}

bool 
TestProxy::isFromMe(const SipMessage& msg)
{
   //DebugLog (<< "Received: " << msg << " from " << msg.getSource());
      
   Source src;
   src.host = resip::Tuple::inet_ntop(msg.getSource());
   src.port = msg.getSource().getPort();
   src.transportType = msg.getSource().getType();
   resip::Data transport=msg.header(h_Vias).front().transport();
   transport.lowercase();
   if(transport == "tcp" || transport == "tls")
   {
      if (msg.getSource().isLoopback())
      {
         // !dlb! can we do better?
         return true;
      }
      else
      {
         src.port=0;
      }
   }

   DebugLog(<<"Matching: " << src);
   DebugLog(<< "Sources: " << Inserter(mSources));
   
   return mSources.find(src) != mSources.end();
}

NameAddr
TestProxy::makeUrl(const Data& name) const
{
   NameAddr tmp = mProxyUrl;
   tmp.uri().user() = name;
   return tmp;
}

Data 
TestProxy::toString() const
{
   return mName;
}
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
