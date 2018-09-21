#if !defined(RESIP_DOMAINMATCHER_HXX)
#define RESIP_DOMAINMATCHER_HXX

#include "resip/stack/Contents.hxx"
#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"
#include "rutil/ParseBuffer.hxx"

namespace resip
{

class DomainMatcher
{

   public:

      /**
         @brief Returns true iff domain matches one of the domains that this
            TransactionUser is responsible for. (added with addDomain).
         @param domain The domain name to check.
         @return True iff this TransactionUser is responsible for domain.
         @note The comparison performed is case-sensitive; make sure you
            lower-case everything you put in here.
      */
      virtual bool isMyDomain(const Data& domain) const = 0;

      /**
         @brief Adds a domain to the set of domains that this TransactionUser is
            responsible for.
         @note The comparison performed is case-sensitive; make sure you
            lower-case everything you put in here.
         @todo Make this case-insensitive.
         @warning This method is NOT thread-safe.  mDomainList is accessed from both the
            thread that queues to the TU (iff a DomainIsMe MessageFilterRule is used)
            and from within the TU itself.  The only way calling this is safe, is if you
            can ensure both of these threads are not calling isMyDomain when you call
            this method.  For example an application that does NOT use a DomainIsMe
            MessageFilterRule and that posts the addDomain call to the TU processing
            thread for execution there, will be thread safe.  This API is also safe to call
            before any of the processing threads have been started.
         */
      virtual void addDomain(const Data& domain) = 0;

      /**
         @brief Removes a domain from the set of domains that this TransactionUser is
            responsible for.
         @note This API will lowercase the domain when searching for the domain to
            remove
         @warning This method is NOT thread-safe.  mDomainList is accessed from both the
            thread that queues to the TU (iff a DomainIsMe MessageFilterRule is used)
            and from within the TU itself.  The only way calling this is safe, is if you
            can ensure both of these threads are not calling isMyDomain when you call
            this method.  For example an application that does NOT use a DomainIsMe
            MessageFilterRule and that posts the removeDomain call to the TU processing
            thread (ie: DUMThread) for execution there, will be thread safe.
      */
      virtual void removeDomain(const Data& domain) = 0;

};

}

#endif

/* ====================================================================
 *
 * Copyright 2017 Daniel Pocock http://danielpocock.com  All rights reserved.
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

