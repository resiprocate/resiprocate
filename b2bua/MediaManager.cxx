
#include "rutil/Data.hxx"

#include "MediaManager.hxx"

using namespace b2bua;
using namespace resip;
using namespace std;

Data MediaManager::proxyAddress;

void MediaManager::setProxyAddress(const Data& proxyAddress) {
  MediaManager::proxyAddress = proxyAddress;
};

MediaManager::MediaManager(B2BCall& b2BCall) : b2BCall(b2BCall) {
  aLegProxy = NULL;
  bLegProxy = NULL;
  rtpProxyUtil = NULL;
};

MediaManager::MediaManager(B2BCall& b2BCall, const Data& callId, const Data& fromTag, const Data& toTag) : b2BCall(b2BCall), callId(callId), fromTag(fromTag), toTag(toTag) {
  aLegProxy = NULL;
  bLegProxy = NULL;
  rtpProxyUtil = NULL;
};

MediaManager::~MediaManager() {
  if(aLegProxy != NULL)
    delete aLegProxy;
  if(bLegProxy != NULL)
    delete bLegProxy;
  if(rtpProxyUtil != NULL)
    delete rtpProxyUtil;
};

void MediaManager::setFromTag(const Data& fromTag) {
  this->fromTag = fromTag;
};

void MediaManager::setToTag(const Data& toTag) {
  this->toTag = toTag;
};

int MediaManager::setALegSdp(const SdpContents& sdp, const in_addr_t& msgSourceAddress) {
  aLegSdp = sdp;
  if(aLegProxy == NULL)
    aLegProxy = new MediaProxy(*this);
  return aLegProxy->updateSdp(aLegSdp, msgSourceAddress);
};

SdpContents& MediaManager::getALegSdp() {
  if(aLegProxy == NULL) {
    throw new exception;
  }
  return aLegProxy->getSdp();
};

int MediaManager::setBLegSdp(const SdpContents& sdp, const in_addr_t& msgSourceAddress) {
  bLegSdp = sdp;
  if(bLegProxy == NULL)
    bLegProxy = new MediaProxy(*this);
  return bLegProxy->updateSdp(bLegSdp, msgSourceAddress);
};

SdpContents& MediaManager::getBLegSdp() {
  if(bLegProxy == NULL)
    throw new exception;
  return bLegProxy->getSdp();
};

void MediaManager::onMediaTimeout() {
  b2BCall.onMediaTimeout();
};

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

