#include "resiprocate/SipStack.hxx"
#include "resiprocate/KeepAliveMessage.hxx"
#include "resiprocate/dum/KeepAliveManager.hxx"
#include "resiprocate/dum/KeepAliveTimeout.hxx"
#include "resiprocate/dum/DialogUsageManager.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/os/compat.hxx"
#include "resiprocate/os/DnsUtil.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

void KeepAliveManager::setDialogUsageManager(DialogUsageManager* dum)
{
   assert(dum);
   mDum = dum;
   mDum->getSipStack().registerBlacklistListener(T_A, this);
   mDum->getSipStack().registerBlacklistListener(T_AAAA, this);
}

KeepAliveManager::~KeepAliveManager()
{
   mDum->getSipStack().unregisterBlacklistListener(T_A, this);
   mDum->getSipStack().unregisterBlacklistListener(T_AAAA, this);
}

void KeepAliveManager::add(const Tuple& target, int keepAliveInterval)
{
   assert(mDum);
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it == mNetworkAssociations.end())
   {
      InfoLog( << "First keep alive for: " << target );
      NetworkAssociationInfo info;
      info.refCount = 1;
      info.keepAliveInterval = keepAliveInterval;
      mNetworkAssociations.insert(NetworkAssociationMap::value_type(target, info));
      KeepAliveTimeout t(target);
      SipStack &stack = mDum->getSipStack();
      stack.post(t, keepAliveInterval, mDum);
   }
   else
   {
      (*it).second.refCount++;
      if(keepAliveInterval < (*it).second.keepAliveInterval)
      {
          (*it).second.keepAliveInterval = keepAliveInterval;  // !slg! only allow value to be shortened???  What if 2 different profiles with different keepAliveTime settings are sharing this network association?
      }
   }
}

void KeepAliveManager::remove(const Tuple& target)
{
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it != mNetworkAssociations.end())
   {
      if (0 == --(*it).second.refCount)
      {
         mNetworkAssociations.erase(target);
      }
   }
}

void KeepAliveManager::process(KeepAliveTimeout& timeout)
{
   assert(mDum);
   static KeepAliveMessage msg;
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(timeout.target());
   if (it != mNetworkAssociations.end())
   {
      SipStack &stack = mDum->getSipStack();
      stack.sendTo(msg, timeout.target(), mDum);
      KeepAliveTimeout t(it->first);
      stack.post(t, it->second.keepAliveInterval, mDum);
      InfoLog( << "Refreshing keep alive of " << it->second.keepAliveInterval << " seconds for: " << timeout.target());
   }
}

void KeepAliveManager::onBlacklisted(int rrType, const Data& target)
{
   switch (rrType)
   {
      case T_A:
      case T_AAAA:
         InfoLog(<< target << "has been blacklisted.");
         break;
      default:
         break;
   }
}

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
