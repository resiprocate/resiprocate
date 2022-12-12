#ifndef REGISTRATIONFORWARDER_HXX
#define REGISTRATIONFORWARDER_HXX

#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/ConfigParse.hxx"
#include "rutil/Data.hxx"
#include "resip/stack/NameAddr.hxx"
#include "resip/stack/SipStack.hxx"
#include "resip/stack/TransactionUser.hxx"

namespace reconserver
{

class RegistrationForwarder : public resip::TransactionUser
{
   public:
      RegistrationForwarder(resip::ConfigParse& cp, resip::SipStack& stack);
      virtual ~RegistrationForwarder();

      bool process(resip::Mutex* mutex = 0);

      virtual const resip::Data& name() const;

   private:
      void internalProcess(std::unique_ptr<resip::Message>);

      resip::SipStack& mStack;

      unsigned int mMaxExpiry;
      resip::Data mPath;
      resip::NameAddr mRegistrationRoute;

      typedef enum
      {
         Running,
         ShutdownRequested, // while ending usages
         RemovingTransactionUser, // while removing TU from stack
         Shutdown,  // after TU has been removed from stack
         Destroying // while calling destructor
      } ShutdownState;
      ShutdownState mShutdownState;
};

}

#endif

/* ====================================================================
 *
 * Copyright 2016 Daniel Pocock http://danielpocock.com  All rights reserved.
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

