#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include <rutil/Logger.hxx>
#include <resip/stack/Helper.hxx>

#include "AppSubsystem.hxx"
#include "IChatIPPortData.hxx"
#include <rutil/WinLeakCheck.hxx>

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

using namespace gateway;
using namespace resip;
using namespace std;

namespace gateway 
{

typedef struct
{
   unsigned short initialPad;
   unsigned short family;
   char interfaceName[16];
   char ipAddress[16];
   unsigned short port;
   unsigned short finalPad;
} IPPortDataBlob;

IChatIPPortData::IChatIPPortData()
{
}

IChatIPPortData::IChatIPPortData(const std::string& hexblob) : mHexBlob(hexblob)
{
   unsigned int pad = 0;
   // Note: gloox passes us an initial line feed, so be tollerant of this
   if(hexblob.at(0) == 0xa) 
   {
      pad = 1;
   }

   // Convert hexblob to binary blob
   if(((hexblob.size()-pad) % 40) != 0)
   {
      ErrLog(<< "Data size provided for IChatIPPortData is not correct, size=" << hexblob.size());
      return;
   }
   Data blob((unsigned int)hexblob.size()-1 / 2, Data::Preallocate);
   for(unsigned int i = pad; i < hexblob.size(); i=i+8)
   {
      unsigned int block = ntohl(Helper::hex2integer(&hexblob.at(i)));
      blob.append((const char*)&block, 4);
   }         

   for(unsigned int j = 0; j < blob.size(); j=j+40)
   {
      IPPortDataBlob* ipPortDataBlob = (IPPortDataBlob*)(blob.data()+j);

      if(ipPortDataBlob->family == V4)
      {
         in_addr addr;
         memcpy(&addr, ipPortDataBlob->ipAddress, 4);
         addIPPortData(Data(ipPortDataBlob->interfaceName), Tuple(addr,ntohs(ipPortDataBlob->port),UDP));
      }
      else
      {
#ifdef USE_IPV6
         in6_addr addr;
         memcpy(&addr, ipPortDataBlob->ipAddress, 16);
         addIPPortData(Data(ipPortDataBlob->interfaceName), Tuple(addr,ntohs(ipPortDataBlob->port),UDP));
#else
         ErrLog(<< "IPv6 support not enabled at compile time.");
         resip_assert(0);
#endif
      }
      InfoLog(<< "IChatIPPortData: name=" << mIPPortDataList.back().first << " addr=" << mIPPortDataList.back().second);
   }
}

void 
IChatIPPortData::addIPPortData(const Data& name, const Tuple& ipPortData)
{
   mIPPortDataList.push_back(std::make_pair(name, ipPortData));
   mHexBlob.clear();  // clear any existing blob string - it will now need to be re-generated if requested
}

std::string& 
IChatIPPortData::hexBlob()
{
   if(mHexBlob.empty())
   {
      Data dataBlob((int)mIPPortDataList.size() * sizeof(IPPortDataBlob), Data::Preallocate);
      IPPortDataList::const_iterator it;
      for(it = mIPPortDataList.begin(); it != mIPPortDataList.end(); it++)
      {
         IPPortDataBlob ipPortDataBlob;
         memset(&ipPortDataBlob, 0, sizeof(ipPortDataBlob));
         memcpy(ipPortDataBlob.interfaceName, it->first.data(), it->first.size() < 16 ? it->first.size() : 16);
         ipPortDataBlob.port = htons(it->second.getPort());
         if(it->second.ipVersion() == V4)
         {
            ipPortDataBlob.family = V4;
            memcpy(ipPortDataBlob.ipAddress, &reinterpret_cast<const sockaddr_in&>(it->second.getSockaddr()).sin_addr, 4);
         }
         else
         {
            ipPortDataBlob.family = V6;
            memcpy(ipPortDataBlob.ipAddress, &reinterpret_cast<const sockaddr_in6&>(it->second.getSockaddr()).sin6_addr, 16);
         }
         dataBlob.append((const char*)&ipPortDataBlob, sizeof(ipPortDataBlob));
      }

      // Hexify
      for(unsigned int i = 0; i < dataBlob.size(); i=i+4)
      {
         unsigned int block;
         memcpy(&block, dataBlob.data()+i, 4);
         char hexBlock[8];
         Helper::integer2hex(hexBlock, htonl(block)); 
         mHexBlob.append(hexBlock, 8);
      }
   }
   return mHexBlob;
}

const IChatIPPortData::IPPortDataList& 
IChatIPPortData::getIPPortDataList() const
{
   return mIPPortDataList;
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

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

 ==================================================================== */

