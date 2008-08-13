#include "resip/stack/SipStack.hxx"
#include "resip/stack/TransactionUser.hxx"
#include "resip/stack/Security.hxx"
#include "resip/dum/Handles.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/PayloadEncrypter.hxx"
#include "resip/dum/DumEncrypted.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/Mime.hxx"

#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Helper.hxx"

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#ifdef WIN32
#include "resip/stack/WinSecurity.hxx"
#endif

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST



/*****************************************************************************/

class Tu : public TransactionUser
{
   public:
      Tu(Security* security, const Data& sender, const Data& recip) 
         : mSecurity(security),
           mSender(sender),
           mRecip(recip)
      {
      }

      const Data& name() const
      {
         static Data name("Tu");
         return name;
      }

      bool process();
   private:
      Security* mSecurity;
      Data mSender;
      Data mRecip;
};

bool Tu::process()
{
   if (mFifo.empty()) return false;
   DumEncrypted* encrypted = dynamic_cast<DumEncrypted*>(mFifo.getNext());
   if (encrypted->success())
   {
      /*Contents* contents = encrypted->encrypted()->clone();
      Data txt(
        "To: sip:fluffy@h1.cs.sipit.net\r\n"
        "From: tofu <sip:tofu@ua.ntt.sipit.net>;tag=5494179792598219348\r\n"
        "CSeq: 1 SUBSCRIBE\r\n"
        "Call-ID: 1129541551360711705\r\n"
        "Contact: sip:tofu@ua.ntt.sipit.net:5060\r\n"
        "Event: presence\r\n"
        "Content-Length: 0\r\n"
        "Expires: 3600\r\n"
        "User-Agent: NTT SecureSession User-Agent\r\n"
        "\r\n");

        SipMessage* msg = SipMessage::make(txt, false);
        msg->setContents(std::auto_ptr<Contents>(contents));
        Helper::ContentsSecAttrs csa(Helper::extractFromPkcs7(msg, *mSecurity));
        InfoLog( << "Body: " << *csa.mContents << "\n" );
        delete msg;
      */
      InfoLog(<<"Message has been successfully encrypted." << endl);
   }
   else
   {
      InfoLog(<<"Failed to encrypte message: " << encrypted->error() << endl);
   }
   delete encrypted;
   return true;
}

int main(int argc, char *argv[])
{

   if ( argc < 3 ) {
     cout << "usage: " << argv[0] <<"sip:sender sip:recip" << endl;
     return 0;
   }

   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   NameAddr senderAor(argv[1]);
   NameAddr recipAor(argv[2]);
   //Data passwd(argv[3]);

   InfoLog(<< "sender: " << senderAor << endl);
   InfoLog(<< "recipient: " << recipAor << endl);

#ifdef WIN32
   Security* security = new WinSecurity;
#else
   Security* security = new Security;
#endif

   assert(security);
   SipStack stack(security);
   Tu tu(security, senderAor.uri().getAor(), recipAor.uri().getAor());
   BaseUsageHandle handle;
   PayloadEncrypter encrypter(tu, security);

   Contents* contents1 = new PlainContents(Data("message signing"));
   // sign the message
   encrypter.encrypt(std::auto_ptr<Contents>(contents1), senderAor.uri().getAor(), handle);

   Contents* contents2 = new PlainContents(Data("message signing and encrypting"));
   // sign and encrypt the message
   encrypter.encrypt(std::auto_ptr<Contents>(contents2), senderAor.uri().getAor(), recipAor.uri().getAor(), handle);

   while (tu.process())
   {
   }


   return 0;
}

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
