#if !defined(RESIP_PUBLICATIONHANDLER_HXX)
#define RESIP_PUBLICATIONHANDLER_HXX

#include "resip/dum/Handles.hxx"
#include "resip/stack/Mime.hxx"
#include "resip/stack/Contents.hxx"

namespace resip
{

class ClientPublication;
class ServerPublication;
class SipMessage;
class SecurityAttributes;

class ClientPublicationHandler
{
   public:
      virtual ~ClientPublicationHandler() {}

      /// Called when the publication succeeds or each time it is sucessfully
      /// refreshed.
      virtual void onSuccess(ClientPublicationHandle, const SipMessage& status)=0;

      //publication was successfully removed
      virtual void onRemove(ClientPublicationHandle, const SipMessage& status)=0;

      //call on failure. The usage will be destroyed.  Note that this may not
      //necessarily be 4xx...a malformed 200, etc. could also reach here.
      virtual void onFailure(ClientPublicationHandle, const SipMessage& status)=0;

      /// call on Retry-After failure.
      /// return values: -1 = fail, 0 = retry immediately, N = retry in N seconds
      virtual int onRequestRetry(ClientPublicationHandle, int retrySeconds, const SipMessage& status)=0;

      // ?dcm? -- when should this be called
      virtual void onStaleUpdate(ClientPublicationHandle, const SipMessage& /*status*/)
      {}
};

class ServerPublicationHandler
{
   public:
      virtual ~ServerPublicationHandler() {}

      virtual void onInitial(ServerPublicationHandle,
                             const Data& etag,
                             const SipMessage& pub,
                             const Contents* contents,
                             const SecurityAttributes* attrs,
                             UInt32 expires)=0;
      virtual void onExpired(ServerPublicationHandle, const Data& etag)=0;
      virtual void onRefresh(ServerPublicationHandle, const Data& etag,
                             const SipMessage& pub,
                             const Contents* contents,
                             const SecurityAttributes* attrs,
                             UInt32 expires)=0;
      virtual void onUpdate(ServerPublicationHandle,
                            const Data& etag,
                            const SipMessage& pub,
                            const Contents* contents,
                            const SecurityAttributes* attrs,
                            UInt32 expires)=0;
      virtual void onRemoved(ServerPublicationHandle,
                             const Data& etag,
                             const SipMessage& pub,
                             UInt32 expires)=0;

      const Mimes& getSupportedMimeTypes() const;
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
