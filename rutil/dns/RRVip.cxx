#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "AresCompat.hxx"

#include <map>
#include <list>
#include <vector>

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#else
#include <Winsock2.h>
#include <svcguid.h>
#ifdef USE_IPV6
#include <ws2tcpip.h>
#endif
#endif

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/RRVip.hxx"
#include "rutil/WinLeakCheck.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

using namespace resip;
using namespace std;

RRVip::RRVip()
{
   mFactories[T_A] = new HostTransformFactory;
   mFactories[T_AAAA] = new HostTransformFactory;
   mFactories[T_NAPTR] = new NaptrTransformFactroy;
   mFactories[T_SRV] = new SrvTransformFactory;
}

RRVip::~RRVip()
{
   for (map<MapKey, Transform*>::iterator it = mTransforms.begin(); it != mTransforms.end(); ++it)
   {
      delete (*it).second;
   }

   for (TransformFactoryMap::iterator it = mFactories.begin(); it != mFactories.end(); ++it)
   {
      delete (*it).second;
   }
}

void RRVip::vip(const Data& target,
                int rrType,
                const Data& vip)
{
   RRVip::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      it->second->updateVip(vip);
   }
   else
   {
      TransformFactoryMap::iterator it = mFactories.find(rrType);
      resip_assert(it != mFactories.end());
      Transform* transform = it->second->createTransform(vip);
      mTransforms.insert(TransformMap::value_type(key, transform));
   }
}

void RRVip::removeVip(const Data& target,
                      int rrType)
{
   RRVip::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      Data vip = it->second->vip();
      delete (*it).second;
      mTransforms.erase(it);
      DebugLog(<< "removed vip " << target << "(" << rrType << "): " << vip);
   }
}

void RRVip::transform(const Data& target,
                      int rrType,
                      std::vector<DnsResourceRecord*>& src)
{
   RRVip::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      bool invalidVip = false;
      it->second->transform(src, invalidVip);
      if (invalidVip) 
      {
         removeVip(target, rrType);
      }
   }
}

RRVip::Transform::Transform(const Data& vip) 
   : mVip(vip)
{
}

RRVip::Transform::~Transform()
{
}

void RRVip::Transform::updateVip(const Data& vip)
{
   DebugLog(<< "updating an existing vip: " << mVip << " with " << vip);
   mVip = vip;
}

void RRVip::Transform::transform(RRVector& src,
                                 bool& invalidVip)
{
   invalidVip = true;
   RRVector::iterator it;
   for (it = src.begin(); it != src.end(); ++it)
   {
      if ((*it)->isSameValue(mVip))
      {
         invalidVip = false;
         break;
      }
   }
   if(!invalidVip)
   {
      DebugLog( << "tranforming records");
      if (src.begin() != it)
      {
         DnsResourceRecord* vip = *it;
         src.erase(it);
         src.insert(src.begin(), vip);
      }
   }
}

RRVip::NaptrTransform::NaptrTransform(const Data& vip)
   : Transform(vip)
{
   DebugLog(<< "Creating a new Napter transform for " << vip);
}

void RRVip::NaptrTransform::transform(RRVector& src,
                                      bool& invalidVip)
{
   invalidVip = true;
   RRVector::iterator vip;
   for (RRVector::iterator it = src.begin(); it != src.end(); ++it)
   {
      if ((*it)->isSameValue(mVip))
      {
         DebugLog(<< "naptr vip record " << mVip << "found");
         invalidVip = false;
         vip = it;
         break;
      }
   }
   if(!invalidVip)
   {
      DebugLog(<< "Transforming Naptr records");
      int min = dynamic_cast<DnsNaptrRecord*>(*(src.begin()))->order();
      for (RRVector::iterator it = src.begin(); it != src.end(); ++it)
      {
         int order = ((dynamic_cast<DnsNaptrRecord*>(*it))->order())++;
         if (order < min) min = order;
      }
      dynamic_cast<DnsNaptrRecord*>((*vip))->order() = min;
   }
}

RRVip::SrvTransform::SrvTransform(const Data& vip)
   : Transform(vip)
{
   DebugLog(<< "Creating a new SRV transform for" << vip);
}

void RRVip::SrvTransform::transform(RRVector& src,
                                    bool& invalidVip)
{
   invalidVip = true;
   RRVector::iterator vip;
   for (RRVector::iterator it = src.begin(); it != src.end(); ++it)
   {
      if ((*it)->isSameValue(mVip))
      {
         invalidVip = false;
         vip = it;
         break;
      }
   }
   if(!invalidVip)
   {
      DebugLog(<< "Transforming SRV records");
      int min = dynamic_cast<DnsSrvRecord*>(*(src.begin()))->priority();
      for (RRVector::iterator it = src.begin(); it != src.end(); ++it)
      {
         int priority = ((dynamic_cast<DnsSrvRecord*>(*it))->priority())++;
         if (priority < min) min = priority;
      }
      dynamic_cast<DnsSrvRecord*>((*vip))->priority() = min;
   }
}

RRVip::MapKey::MapKey() 
  : mRRType(0)		// .kw. what is reasonable default?
{
}

RRVip::MapKey::MapKey(const Data& target, int rrType)
   : mTarget(target), 
     mRRType(rrType)
{
}

bool RRVip::MapKey::operator<(const MapKey& rhs) const
{
   if (mRRType < rhs.mRRType)
   {
      return true;
   }
   else if (mRRType > rhs.mRRType)
   {
      return false;
   }
   else
   {
      return mTarget < rhs.mTarget;
   }
}

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
 