#if !defined(TUUA_HXX)
#define TUUA_HXX

#include <map>
#include "resiprocate/util/Data.hxx"
#include "resiprocate/sipstack/DialogSet.hxx"

namespace Vocal2
{

class TuUA
{
   public:
      class Callback
      {
         public:
            // these call back functions receive a given message class - they
            // should return the SIP response code that they would like the
            // stack to send or just return 0 to ge the default handling
            virtual int receivedSipSession(const SipMessage* msg, 
                                            const Uri& from, 
                                            const Data& signedBy,  
                                            Security::SignatureStatus sigStatus,
                                            bool wasEncryped  ); 
            virtual int receivedSubscription(const SipMessage* msg, 
                                            const Uri& from, 
                                            const Data& signedBy,  
                                            Security::SignatureStatus sigStatus,
                                            bool wasEncryped  ); 
             virtual int receivedRegistration(const SipMessage* msg, 
                                            const Uri& from, 
                                            const Data& signedBy,  
                                            Security::SignatureStatus sigStatus,
                                            bool wasEncryped  ); 
             virtual int receivedNonStateRequest(const SipMessage* msg, 
                                            const Uri& from, 
                                            const Data& signedBy,  
                                            Security::SignatureStatus sigStatus,
                                            bool wasEncryped  ); 
            virtual int receivedNonDialogRequest(const SipMessage* msg, 
                                                  const Uri& from, 
                                                  const Data& signedBy,  
                                                  Security::SignatureStatus sigStatus,
                                                  bool wasEncryped  ); 
            virtual int receivedPage(const SipMessage* msg, 
                                      const Data& msg, 
                                      const Uri& from, 
                                      const Data& signedBy,  
                                      Security::SignatureStatus sigStatus,
                                      bool wasEncryped  ); 
            virtual int receivePresenseUpdate(const SipMessage* msg, 
                                              const Uri& dest, 
                                              bool open, 
                                              const Data& status );

            virtual void requestFailed(const Uri& dest,
                                       int respNumber,
                                       Data& msg);

            virtual void registrationFailed(const Uri& dest, 
                                            int respNumber,
                                            Data& msg);

            virtual void registrationWorked(const Uri& dest );

            virtual ~Callback();
      };
    
      TuUA(SipStack* stack, 
           const Uri& aor, 
           const Uri& contact,
           Callback* callback);

      // Registration related
      createRegistration();
      
      // Subscriptiont related
      
      // SIP Session related
      createSipSession();
      divertSipSession(); // blind transfer or 300
      endSipSession();
      completeTransfer();
      changeSession();
      
      // conference related
      createConference();
      addSipSession();
      dropSipSesssion();
      
      // presense related 
      changePresense();
      changePolicy();
      
      // IM Related 
      sendMessage();
      
   private:
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



