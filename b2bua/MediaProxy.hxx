
#ifndef __MediaProxy_h
#define __MediaProxy_h

#include "resip/stack/SdpContents.hxx"

namespace b2bua
{

class MediaProxy;

}

#include "MediaManager.hxx"

namespace b2bua
{

/*
  A generic MediaProxy is based on the SDP from a single endpoint.

  Two MediaProxys must work together to provide a symmetric RTP service
  that will get through NAT.  The two MediaProxy instances are
  synchronised by the MediaManager.

  For an implementation based on rtpproxy, we must also use SIP callID,
  from-tag and to-tag, as these are the values rtpproxy expects to use
  for uniquely identifying each call.

  Alternatively, we could use our own unique identifier system.
*/

class MediaProxy {

protected:
  struct EndPoint {
    resip::Data address;		// the address we should forward to
    unsigned int originalPort;		// the port on the dest address
    unsigned int proxyPort;		// the port we listen with
  };

  static bool mNatHelper;

private:
  MediaManager& mediaManager;		// the MediaManager who controls us
  std::list<EndPoint> endpoints;	// the endpoints for each media
					// offer in the SDP
  resip::SdpContents *originalSdp;	// the original SDP
  resip::SdpContents *newSdp;		// the modified SDP

public:
  static void setNatHelper(bool natHelper);
  MediaProxy(MediaManager& mediaManager);
  virtual ~MediaProxy();
  int updateSdp(const resip::SdpContents& sdp, const in_addr_t& msgSourceAddress);		// update the SDP,
							// as a result of a
							// new offer
  resip::SdpContents& getSdp();				// get the SDP
							// that should be sent
							// to the other party
							// we correspond with
  bool allowProtocol(const resip::Data& protocol);	// discover if protocol
							// is permitted/handled
							// by this proxy
							// implementation

  bool isAddressPrivate(const in_addr_t& subj_addr);

};

}

#endif 

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

