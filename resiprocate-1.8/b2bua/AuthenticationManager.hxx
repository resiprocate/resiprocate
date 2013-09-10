
#ifndef __AuthenticationManager_h
#define __AuthenticationManager_h

#include "rutil/Data.hxx"

namespace b2bua
{

class AuthenticationManager {

//  const resip::Data& realm;
//  const resip::Data& user;

public:

  /**
   * The application should implement SecretListener to receive callbacks
   * when authentication is complete.
   */
  class SecretListener {
  public:
    virtual ~SecretListener();
    // Called when the secret is found successfully
    virtual void onSuccess(const resip::Data& realm, const resip::Data& user, const resip::Data& secret) = 0;
    // Called when the user ID is not recognised
    virtual void onUserUnknown(const resip::Data& realm, const resip::Data& user) = 0;
    // Called when an internal failure or timeout occurs
    virtual void onFailure(const resip::Data& realm, const resip::Data& user) = 0;
  };

  virtual ~AuthenticationManager() {};

  virtual void getSecret(const resip::Data& realm, const resip::Data& user, AuthenticationManager::SecretListener *secretListener) = 0;

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

