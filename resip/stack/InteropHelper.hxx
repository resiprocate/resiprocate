#ifndef INTEROP_HELPER_HXX
#define INTEROP_HELPER_HXX

namespace resip
{

/**
   This class is intended to encapsulate what version/s of various drafts are
   supported by the stack. This also allows for configurable version support at
   runtime.
*/
class InteropHelper
{
   public:
      static bool getRportEnabled() {return rport;}
      static void setRportEnabled(bool enable) {rport=enable;}

      static int getOutboundVersion() {return theOutboundVersion;}
      static void setOutboundVersion(int version) {theOutboundVersion=version;}
      static bool getOutboundSupported() {return isOutboundSupported;}
      static void setOutboundSupported(bool supported) {isOutboundSupported=supported;}

      // If this value is set, then DUM/repro will populate a Flow-Timer header in a 
      // successful registration reponse
      static unsigned int getFlowTimerSeconds() {return flowTimerSeconds;}
      static void setFlowTimerSeconds(unsigned int seconds) {flowTimerSeconds=seconds;}

      // Only relevant if setFlowTimerSeconds is set to value greater than 0.
      // Specifies the amount of time beyond the FlowTimer time, before the stack
      // will consider any Flow-Timer based connection to be in a bad state.  This
      // is used by the ConnectionManager garbage collection logic to cleanup
      // flow-timer based connections for which we are no-longer receiving keepalives.
      static unsigned int getFlowTimerGracePeriodSeconds() {return flowTimerGracePeriodSeconds;}
      static void setFlowTimerGracePeriodSeconds(unsigned int seconds) {flowTimerGracePeriodSeconds=seconds;}
      
      // .bwc. If this is enabled, we will record-route with flow tokens 
      // whenever possible. This will make things work with endpoints that don't
      // use NAT traversal tricks. However, this will break several things:
      // 1) Target-refreshes won't work.
      // 2) Proxies that do not record-route may be implicitly included in the
      //    route-set by this proxy, because a flow token may point to them.
      // 3) Third-party registrations won't work.
      static bool getRRTokenHackEnabled(){return useRRTokenHack;}
      static void setRRTokenHackEnabled(bool enabled) {useRRTokenHack=enabled;}
      
      // If EnableFlowTokens is enabled, then by default flow tokens are only used for inbound
      // Record-Routes if the client is directly connected(ie: has only a single Via header).If you
      // enable this setting then inbound flow tokens will be used for non-directly connected clients
      // as well(ie: any number of Via headers).
      // This is particularly useful for TLS based connections between two SIP proxies, to help ensure
      // a single TLS connection per dialog.Avoiding an issue where a UAC request may be using an IP
      // addresses in it's TLS based Record-Route but is presenting a certificate that does not contain
      // the IP address.
      static bool getAllowInboundFlowTokensForNonDirectClients() { return allowInboundFlowTokensForNonDirectClients; }
      static void setAllowInboundFlowTokensForNonDirectClients(bool enabled) { allowInboundFlowTokensForNonDirectClients = enabled; }

      enum ClientNATDetectionMode
      {
         ClientNATDetectionDisabled,
         ClientNATDetectionEnabled,
         ClientNATDetectionPrivateToPublicOnly
      };

      // If this is enabled, and we have clients not explicitly supporting outbound
      // that we detect to be behind a NAT device, we will record-route with flow tokens 
      // whenever possible. However, this will break several things:
      // 1) Target-refreshes won't work.
      // 2) Proxies that do not record-route may be implicitly included in the
      //    route-set by this proxy, because a flow token may point to them.
      // 3) Third-party registrations won't work.
      static InteropHelper::ClientNATDetectionMode getClientNATDetectionMode(){return clientNATDetection;}
      static void setClientNATDetectionMode(InteropHelper::ClientNATDetectionMode mode) {clientNATDetection=mode;}

      // There are cases where the first hop in a particular network supports the concept of outbound
      // and ensures all messaging for a client is delivered over the same connection used for
      // registration.  This could be a SBC or other NAT traversal aid router that uses the Path 
      // header.  However such endpoints may not be 100% compliant with outbound RFC and may not 
      // include a ;ob parameter in the path header.  This parameter is required in order for repro
      // to have knowledge that the first hop does support outbound, and it will reject registrations
      // that appear to be using outboud (ie. instanceId and regId) with a 439 (First Hop Lacks Outbound
      // Support).  In this case it can be desirable when using repro as the registrar to not reject
      // REGISTRATION requests that contain an instanceId and regId with a 439.
      // If this setting is enabled, then repro will assume the first hop supports outbound 
      // and not return this error.
      static bool getAssumeFirstHopSupportsOutboundEnabled(){return assumeFirstHopSupportsOutbound;}
      static void setAssumeFirstHopSupportsOutboundEnabled(bool enabled) {assumeFirstHopSupportsOutbound=enabled;}

      // AssumeFirstHopSupportsOutbound only relaxes the Outbound logic for registrations from clients who send the
      // instance-id and reg-id parameters.  If the registrations pass through an edge proxy or SBC with the
      // useRRTokenHack or clientNATDetection hacks enabled before reaching the registration server,
      // the registration server can also potentially accept registrations that have come through that proxy with Path headers.
      // If this setting is enabled, repro will assume the first hop supports outbound or flow token hacks
      // and will not reject registrations with the 439 error.
      static bool getAssumeFirstHopSupportsFlowTokensEnabled(){return assumeFirstHopSupportsFlowTokens;}
      static void setAssumeFirstHopSupportsFlowTokensEnabled(bool enabled) {assumeFirstHopSupportsFlowTokens=enabled;}

   private:
      InteropHelper();
      ~InteropHelper();
      
      static bool rport;
      static int theOutboundVersion;
      static bool isOutboundSupported;
      static unsigned int flowTimerSeconds;
      static unsigned int flowTimerGracePeriodSeconds;
      static bool useRRTokenHack;
      static bool allowInboundFlowTokensForNonDirectClients;
      static ClientNATDetectionMode clientNATDetection;
      static bool assumeFirstHopSupportsOutbound;
      static bool assumeFirstHopSupportsFlowTokens;
};
}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000
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

