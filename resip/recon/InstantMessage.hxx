#if !defined(InstantMessage_hxx)
#define InstantMessage_hxx

#include <resip/dum/PagerMessageHandler.hxx>

#include "HandleTypes.hxx"

namespace recon
{

/**
  This class represents the Instant Message (IM) mechanism.
  It was based on RFC 3428:
  https://tools.ietf.org/html/rfc3428
  Author: Mateus Bellomo (mateusbellomo AT gmail DOT com)
*/

class InstantMessage : public resip::ServerPagerMessageHandler,
                       public resip::ClientPagerMessageHandler
{
   public:
      InstantMessage();
      virtual ~InstantMessage();

      ///////////////////////////////////////////////////////////////////////
      // Pager Message Handlers /////////////////////////////////////////////
      ///////////////////////////////////////////////////////////////////////

     /**
	Callback used when received a MESSAGE SIP message 
	from the server.
	@note An application should override this method.
     */
      virtual void onMessageArrived(resip::ServerPagerMessageHandle handle, const resip::SipMessage& message);

     /**
	Callback used when MESSAGE has been successfully sent
	@note An application should override this method.
     */
      virtual void onSuccess(resip::ClientPagerMessageHandle handle, const resip::SipMessage& status);

     /**
	Callback used when a MESSAGE has been failed to send
	@note An application should override this method. Application could re-page the failed contents or just ingore it.
     */
      virtual void onFailure(resip::ClientPagerMessageHandle handle, const resip::SipMessage& status, std::auto_ptr<resip::Contents> contents);

};

}

#endif


/* ====================================================================
 *
 * Copyright 2016 Mateus Bellomo https://mateusbellomo.wordpress.com/  All rights reserved.
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

