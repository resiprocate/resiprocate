
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

