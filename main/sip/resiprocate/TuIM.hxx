#if !defined(UDPTRANSPORT_HXX)
#define UDPTRANSPORT_HXX

#include <vector>

#include "sip2/sipstack/Security.hxx"

namespace Vocal2
{

class TuIM
{
   public:

      class PageCallback
      {
         public:
            virtual void receivedPage(const Data& msg, const Uri& from, 
                                      const Data& signedBy,  Security::SignatureStatus sigStatus,
                                      bool wasEncryped  ) = 0; 
            virtual ~PageCallback();
      };
      
      class ErrCallback
      {
         public:
            virtual void sendPageFailed(const Uri& dest ) =0;
            virtual ~ErrCallback();
      };
      
      class PresCallback
      {
         public:
            virtual void presenseUpdate(const Uri& dest, bool open, const Data& status ) =0;
            virtual ~PresCallback();
      };
      
      TuIM(SipStack* stack, 
           const Uri& aor, 
           const Uri& contact,
           PageCallback* pageCallback, 
           ErrCallback* errCallback,
           PresCallback* pressCallback);
      
      void sendPage(const Data& text, const Uri& dest, bool sign, const Data& encryptFor );

      void process();

      // Registration management 
      void registerAor( const Uri& uri, const Data& password = Data::Empty );

      // Buddy List management
      int getNumBudies() const;
      const Uri getBuddyUri(const int index);
      const Data getBuddyGroup(const int index);
      void addBuddy( const Uri& uri, const Data& group );
      void removeBudy( const Uri& name);

      // Presense management
      void setMyPresense( const bool open, const Data& status = Data::Empty );

   private:
      PageCallback* mPageCallback;
      ErrCallback* mErrCallback; 
      PresCallback* mPressCallback;
      SipStack* mStack;
      Uri mAor;
      Uri mContact;
      Data mPassword;

      class Buddy
      {
         public:
            Uri uri;
            Data group;
      };

      vector<Buddy> mBuddy;
};

}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0  *  * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved. *  * Redistribution and use in source and binary forms, with or without * modification, are permitted provided that the following conditions * are met: *  * 1. Redistributions of source code must retain the above copyright *    notice, this list of conditions and the following disclaimer. *  * 2. Redistributions in binary form must reproduce the above copyright *    notice, this list of conditions and the following disclaimer in *    the documentation and/or other materials provided with the *    distribution. *  * 3. The names "VOCAL", "Vovida Open Communication Application Library", *    and "Vovida Open Communication Application Library (VOCAL)" must *    not be used to endorse or promote products derived from this *    software without prior written permission. For written *    permission, please contact vocal@vovida.org. * * 4. Products derived from this software may not be called "VOCAL", nor *    may "VOCAL" appear in their name, without prior written *    permission of Vovida Networks, Inc. *  * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL, * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH * DAMAGE. *  * ==================================================================== *  * This software consists of voluntary contributions made by Vovida * Networks, Inc. and many individuals on behalf of Vovida Networks, * Inc.  For more information on Vovida Networks, Inc., please see * <http://www.vovida.org/>. *
 */


