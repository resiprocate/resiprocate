#include "rutil/DnsUtil.hxx"
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
                     int port, 
                     const Data& interfaceObj)

   : mName(name)
{
   mPort = (port == 0 ? PortAllocator::getNextPort() : port);
   mProxyUrl.uri().host() = host;

   if (port != 5060) 
   {
      mProxyUrl.uri().port() = mPort;
   }
   
   try //throws if only loopback is available
   {
      Source stest(host, mPort, UDP);
      addSource(stest);
      Source s(resip::DnsUtil::getLocalIpAddress(), mPort, UDP);
      addSource(s);
      Source s2(resip::DnsUtil::getLocalIpAddress(), mPort, TCP);
      addSource(s2);
      Source s3(resip::DnsUtil::getLocalIpAddress(), mPort, TLS);
      addSource(s2);
   }
   catch (resip::DnsUtil::Exception& e)
   {
      DebugLog(<<"No external address found:" << e);
   }
   
   // .dlb. other ethi
   for (int i = 1; i < 11; ++i)
   {
      Source ssu("127.0.0." + Data(i), mPort, UDP);
      addSource(ssu);
      Source sst("127.0.0." + Data(i), mPort, TCP);
      addSource(sst);
      Source sss("127.0.0." + Data(i), mPort, TLS);
      addSource(sss);
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
         assert(!ips.empty());
         mContact.uri().host() = ips.front();
      }
   }
   DebugLog(<<"TestProxy::getContact returned " << mContact);
   
   return mContact;
}

bool 
TestProxy::isFromMe(const SipMessage& msg)
{
   //DebugLog (<< "Received: " << msg << " from " << msg.getSource());
      
   if (msg.header(h_Vias).front().transport() == "TCP")
   {
      // !dlb! can we do better?
      return true;
   }

   Source src;
   src.host = resip::Tuple::inet_ntop(msg.getSource());
   src.port = msg.getSource().getPort();
   src.transportType = msg.getSource().getType();
   
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
