
#ifndef __AuthorizationManager_h
#define __AuthorizationManager_h

#include "rutil/Data.hxx"

#include "CallHandle.hxx"

namespace b2bua
{

class AuthorizationManager {

public:

  typedef enum DenialReason {
    AuthenticationRequired, 		// A username is required
    PeerUnknown,  			// IP or username unrecognised by
					// the AuthorizationManager
    InsufficientFunds,			// Deposit more funds
    Unspecified,			// Request denied for an unspecified
					// reason (e.g. account suspended),
					// but not due to error
    DestinationIncomplete,		// Number is too short
    DestinationInvalid,			// Unrecognised destination prefix
					// or name
    InvalidRequest,			// Some other aspect of the request
					// is invalid
    NetworkError,			// An error occurred communicating
					// with a remote auth server
    Timeout,				// A timeout occurred
    GeneralError			// An error while processing
  };

  class AuthorizationListener {
  public:
    virtual ~AuthorizationListener() {};
    // Call is authorized 
    virtual void onSuccess() = 0;
    // Call is denied
    virtual void onDeny(AuthorizationManager::DenialReason reason) = 0;
  };

  //virtual void getAuthorization();

  virtual ~AuthorizationManager() {};

  virtual CallHandle *authorizeCall(const resip::NameAddr& sourceAddr, const resip::Uri& destinationAddr, const resip::Data& authRealm, const resip::Data& authUser, const resip::Data& authPass, const resip::Data& srcIp, const resip::Data& contextId, const resip::Data& accountId, const resip::Data& baseIp, const resip::Data& controlId, time_t startTime) = 0;

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

