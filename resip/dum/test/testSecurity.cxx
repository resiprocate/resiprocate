#include "resip/stack/PlainContents.hxx"
#include "resip/stack/Pkcs7Contents.hxx"
#include "resip/stack/MultipartSignedContents.hxx"
#include "resip/stack/MultipartAlternativeContents.hxx"
#include "resip/stack/Mime.hxx"

#include "resip/stack/SecurityAttributes.hxx"
#include "resip/stack/Helper.hxx"

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"

#ifdef WIN32
#include "resip/stack/XWinSecurity.hxx"
#endif

#include <iostream>
#include <string>
#include <sstream>

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST


int main(int argc, char *argv[])
{

   //Log::initialize(Log::Cout, Log::Debug, argv[0]);
   Log::initialize(Log::Cout, Log::Info, argv[0]);


#ifdef WIN32
   Security* security = new XWinSecurity;
#else
   Security* security = new Security;
#endif

   Data aor("jdoe@internal.xten.net");

   security->preload();
   security->hasUserPrivateKey(aor);
   security->hasUserCert(aor);

   Contents* contents = new PlainContents(Data("v=0\r\n"
                        "o=1900 369696545 369696545 IN IP4 192.168.2.15\r\n"
                        "s=X-Lite\r\n"
                        "c=IN IP4 192.168.2.15\r\n"
                        "t=0 0\r\n"
                        "m=audio 8001 RTP/AVP 8 3 98 97 101\r\n"
                        "a=rtpmap:8 pcma/8000\r\n"
                        "a=rtpmap:3 gsm/8000\r\n"
                        "a=rtpmap:98 iLBC\r\n"
                        "a=rtpmap:97 speex/8000\r\n"
                        "a=rtpmap:101 telephone-event/8000\r\n"
                        "a=fmtp:101 0-15\r\n"));

   //Pkcs7Contents* encrypted = security->encrypt(contents, aor);
   //InfoLog(<< "Encrytped content: " << *encrypted );
   //Contents* decrypted = security->decrypt(aor, encrypted);
   //if (decrypted) 
   //{
   //   InfoLog(<< "Decrypted content: " << decrypted->getBodyData() );
   //}
   MultipartSignedContents* msc = security->sign(aor, contents);
   InfoLog(<< "Signed: " << *msc);


   SecurityAttributes attr;
   Data signer;
   SignatureStatus sigStatus;
   Contents* ret = security->checkSignature(msc, &signer, &sigStatus);
   assert(ret);
   InfoLog(<< "Signature status:" << sigStatus);   
   
   delete contents;
   delete msc;
   //delete decrypted;
   //delete encrypted;
   delete security;
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
