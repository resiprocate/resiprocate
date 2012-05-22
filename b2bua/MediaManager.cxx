
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
 *
 * Copyright 2012 Daniel Pocock.  All rights reserved.
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 *
 *
 */

