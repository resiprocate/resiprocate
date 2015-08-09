#include "resip/stack/KeepAliveMessage.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "resip/dum/KeepAliveManager.hxx"
#include "resip/dum/KeepAliveTimeout.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/TransportType.hxx"
#include "resip/stack/SipStack.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

using namespace resip;
using namespace std;

int KeepAliveManager::mKeepAlivePongTimeoutMs = 10000;  // Defaults to 10000ms (10s) as specified in RFC5626 section 4.4.1

void 
KeepAliveManager::add(const Tuple& target, int keepAliveInterval, bool targetSupportsOutbound)
{
   resip_assert(mDum);
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it == mNetworkAssociations.end())
   {
      DebugLog(<< "First keep alive for id=" << mCurrentId << ": " << target << ", interval=" 
               << keepAliveInterval << "s, supportsOutbound=" << (targetSupportsOutbound ? "true" : "false"));

      NetworkAssociationInfo info;
      info.refCount = 1;
      info.keepAliveInterval = keepAliveInterval;
      info.id = mCurrentId;
      info.supportsOutbound = targetSupportsOutbound;
      info.pongReceivedForLastPing = false;
      mNetworkAssociations.insert(NetworkAssociationMap::value_type(target, info));
      KeepAliveTimeout t(target, mCurrentId);
      SipStack &stack = mDum->getSipStack();
      if(targetSupportsOutbound)
      {
         // Used randomized timeout between 80% and 100% of keepalivetime
         stack.post(t, Helper::jitterValue(keepAliveInterval, 80, 100), mDum);
      }
      else
      {
         stack.post(t, keepAliveInterval, mDum);
      }
      ++mCurrentId;
   }
   else
   {
      it->second.refCount++;
      if(keepAliveInterval < it->second.keepAliveInterval || targetSupportsOutbound)  // if targetSupportsOutbound, then always update the interval, as value may be from Flow-Timer header
      {
         // ?slg? only allow value to be shortened???  What if 2 different profiles 
         // with different keepAliveTime settings are sharing this network association?         
         it->second.keepAliveInterval = keepAliveInterval;  
      }
      if(targetSupportsOutbound)
      {
         // allow this to be updated to true only.  If any usage get's an indication of 
         // outbound support on this flow, then we accept it
         it->second.supportsOutbound = targetSupportsOutbound;  
      }
      DebugLog(<< "Association added for keep alive id=" << it->second.id << ": " << target 
               << ", interval=" << it->second.keepAliveInterval << "s, supportsOutbound=" 
               << (it->second.supportsOutbound ? "true" : "false") 
               << ", refCount=" << it->second.refCount);
   }
}

void 
KeepAliveManager::remove(const Tuple& target)
{
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(target);
   if (it != mNetworkAssociations.end())
   {
      if (0 == --it->second.refCount)
      {
         DebugLog(<< "Last association removed for keep alive id=" << it->second.id << ": " << target);
         mNetworkAssociations.erase(it);
      }
      else
      {
         DebugLog(<< "Association removed for keep alive id=" << it->second.id << ": " << target << ", refCount=" << it->second.refCount);
      }
   }
}

void 
KeepAliveManager::process(KeepAliveTimeout& timeout)
{
   resip_assert(mDum);
   static KeepAliveMessage msg;
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(timeout.target());
   if (it != mNetworkAssociations.end() && timeout.id() == it->second.id)
   {
      SipStack &stack = mDum->getSipStack();
      DebugLog(<< "Refreshing keepalive for id=" << it->second.id << ": " << it->first
               << ", interval=" << it->second.keepAliveInterval << "s, supportsOutbound=" 
               << (it->second.supportsOutbound ? "true" : "false") 
               << ", refCount=" << it->second.refCount);

      if(InteropHelper::getOutboundVersion()>=8 && it->second.supportsOutbound && mKeepAlivePongTimeoutMs > 0)
      {
         // Assert if keep alive interval is too short in order to properly detect
         // missing pong responses - ie. interval must be greater than 10s
         resip_assert((it->second.keepAliveInterval*1000) > mKeepAlivePongTimeoutMs);

         // Start pong timeout if transport is TCP based (note: pong processing of Stun messaging is currently not implemented)
         if(isReliable(it->first.getType()))
         {
            DebugLog( << "Starting pong timeout for keepalive id " << it->second.id);
            KeepAlivePongTimeout t(it->first, it->second.id);
            stack.postMS(t, mKeepAlivePongTimeoutMs, mDum);
         }
      }
      it->second.pongReceivedForLastPing = false;  // reset flag

      stack.sendTo(msg, timeout.target(), mDum);
      KeepAliveTimeout t(it->first, it->second.id);
      if(it->second.supportsOutbound)
      {
         // Used randomized timeout between 80% and 100% of keepalivetime
         stack.post(t, Helper::jitterValue(it->second.keepAliveInterval, 80, 100), mDum);
      }
      else
      {
         stack.post(t, it->second.keepAliveInterval, mDum);
      }
   }
}

void 
KeepAliveManager::process(KeepAlivePongTimeout& timeout)
{
   resip_assert(mDum);
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(timeout.target());
   if (it != mNetworkAssociations.end() && timeout.id() == it->second.id)
   {
      if(!it->second.pongReceivedForLastPing)
      {
         // Timeout expecting pong response
         InfoLog(<< "Timed out expecting pong response for keep alive id=" << it->second.id << ": " << it->first);
         mDum->getSipStack().terminateFlow(it->first);
      }
   }
}

void 
KeepAliveManager::receivedPong(const Tuple& flow)
{
   NetworkAssociationMap::iterator it = mNetworkAssociations.find(flow);
   if (it != mNetworkAssociations.end())
   {
      DebugLog(<< "Received pong response for keep alive id=" << it->second.id << ": " << it->first);
      it->second.pongReceivedForLastPing = true;
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
