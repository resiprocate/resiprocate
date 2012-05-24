
#ifndef __CallHandle_h
#define __CallHandle_h

#include <iostream>
#include <list>

#include "resip/stack/NameAddr.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Data.hxx"
#include "rutil/SharedPtr.hxx"

namespace b2bua
{

#define CC_PERMITTED 0				// Call is permitted	
#define CC_AUTH_REQUIRED 1			// Auth is required
#define CC_PAYMENT_REQUIRED 2			// Payment is required
#define CC_IP_DENIED 3				// IP is blocked/unrecognised
#define CC_ERROR 4				// Server error 
#define CC_INVALID 5				// Request is invalid
#define CC_PENDING 6				// Waiting for server
#define CC_DESTINATION_INCOMPLETE 7		// Destination number too short
#define CC_DESTINATION_INVALID 8		// Destination invalid
#define CC_TIMEOUT 9				// Timeout waiting for server
#define CC_USER_UNKNOWN 10			// User unknown
#define CC_REALM_UNKNOWN 11			// Realm unknown
#define CC_REQUEST_DENIED 12			// Request denied for 
						// administrative reason

class CallRoute {
public:
  virtual ~CallRoute();
  virtual const resip::Data& getAppRef1() = 0;
  virtual const resip::Data& getAppRef2() = 0;
  virtual const resip::Data& getAuthRealm() = 0;
  virtual const resip::Data& getAuthUser() = 0;
  virtual const resip::Data& getAuthPass() = 0;
  virtual const resip::NameAddr& getSourceAddr() = 0;
  virtual const resip::NameAddr& getDestinationAddr() = 0;
  virtual const resip::Uri& getOutboundProxy() = 0;
  virtual const bool isSrvQuery() = 0;
};

class CallHandle {
public:
  virtual ~CallHandle();
  virtual int getAuthResult() = 0; 
  virtual const resip::Data& getRealm() = 0;	// Realm for client auth,
						// if we require further auth
  virtual time_t getHangupTime() = 0;		// The time to hangup,
						// 0 for no limit
  virtual bool mustHangup() = 0;
  virtual void connect(time_t *connectTime) = 0; // The call connected
  virtual void fail(time_t *finishTime) = 0;	// The attempt failed, or
						// the caller gave up
  virtual void finish(time_t *finishTime) = 0;	// The call hungup
  virtual std::list<CallRoute *>& getRoutes() = 0;
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

