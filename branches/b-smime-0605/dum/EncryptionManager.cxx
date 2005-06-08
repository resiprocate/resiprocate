#include <cassert>
#include "resiprocate/os/BaseException.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Helper.hxx"
#include "resiprocate/dum/BaseUsage.hxx"
#include "resiprocate/dum/Handles.hxx"
#include "resiprocate/TransactionUser.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/Contents.hxx"
#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/MultipartAlternativeContents.hxx"
#include "resiprocate/Pkcs7Contents.hxx"
#include "resiprocate/dum/EncryptionManager.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::DUM

EncryptionManager::EncryptionManager(Security& security)
   : mSecurity(security)
{
}

EncryptionManager::~EncryptionManager()
{
}

EncryptionManager::Exception::Exception(const Data& msg, const Data& file, const int line)
   : BaseException(msg,file,line)
{
}

//Todo: complete async part.
Contents* EncryptionManager::sign(Contents* src, 
                                  const Data& senderAor)
{
   bool async  = false;

   if (!mSecurity.hasUserCert(senderAor))
   {
      async = true;
   }

   if (!mSecurity.hasUserPrivateKey(senderAor))
   {
      async = true;
   }

   if (async)
   {
      return 0;
   }

   MultipartSignedContents* contents = 0;
   contents = mSecurity.sign(senderAor, src);
   
   if (!contents)
   {
      throw Exception("Failed to sign", __FILE__, __LINE__);
   }

   return contents;
}

//Todo: complete async part.
Contents* EncryptionManager::encrypt(Contents* src,
                                     const Data& recipAor)
{
   if (!mSecurity.hasUserCert(recipAor))
   {
      return 0;
   }
   Pkcs7Contents* contents = 0;
   contents = mSecurity.encrypt(src, recipAor);
   if (!contents)
   {
      throw Exception("Failed to encrypt", __FILE__, __LINE__);
   }
   return contents;
}

//Todo: complete async part.
Contents* EncryptionManager::signAndEncrypt(Contents* src,
                                            const Data& senderAor,
                                            const Data& recipAor)
{
   bool async = false;

   if (!mSecurity.hasUserCert(recipAor))
   {
      async = true;
   }

   if (!mSecurity.hasUserCert(senderAor))
   {
      async = true;
   }

   if (!mSecurity.hasUserPrivateKey(senderAor))
   {
      async = true;
   }

   if (async)
   {
      return 0;
   }

   MultipartSignedContents* contents = 0;
   contents = mSecurity.signAndEncrypt(senderAor, src, recipAor);

   if (contents)
   {
      throw Exception("Failed to sign and encrypt", __FILE__, __LINE__);
   }

   return contents;
}

//Todo: complete async part.
bool EncryptionManager::decrypt(SipMessage& msg)
{
   Data decryptor;
   Data signer;
   if (msg.isResponse())
   {
      decryptor = msg.header(h_From).uri().getAor();
      signer = msg.header(h_To).uri().getAor();
   }
   else
   {
      decryptor = msg.header(h_To).uri().getAor();
      signer = msg.header(h_From).uri().getAor();
   }

   bool async = false;

   if (isEncrypted(msg))
   {
      if (!mSecurity.hasUserPrivateKey(decryptor))
      {
         async = true;
      }

      if (!mSecurity.hasUserCert(decryptor))
      {
         async = true;
      }
   }

   if (!async)
   {
      if (isSigned(msg, decryptor))
      {
         if (!mSecurity.hasUserCert(signer))
         {
            async = true;
         }
      }
   }

   if (async)
   {
      return true;
   }

   Helper::ContentsSecAttrs secAttr = Helper::extractFromPkcs7(msg, mSecurity);
   msg.setSecurityAttributes(secAttr.mAttributes);
   msg.setContents(secAttr.mContents);
   return false;
}

bool EncryptionManager::isEncrypted(SipMessage& msg)
{
   return isEncryptedRecurse(msg.getContents());
}

bool EncryptionManager::isSigned(SipMessage& msg,
                              const Data& decryptorAor)
{
   return isSignedRecurse(msg.getContents(), decryptorAor);
}

bool EncryptionManager::isEncryptedRecurse(Contents* contents)
{
   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(contents)))
   {
#if defined(USE_SSL)
      return true;
#else
      return false;
#endif
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(contents)))
   {
      if (isEncryptedRecurse(*(mps->parts().begin())))
      {
         return true;
      }
      else
      {
         return false;
      }
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(contents)))
   {
      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         if (isEncryptedRecurse(*i))
         {
            return true;
         }
      }
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(contents)))
   {
      for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
           i != mult->parts().end(); ++i)
      {
         if (isEncryptedRecurse(*i))
         {
            return true;
         }
      }
   }

   return false;
}

bool EncryptionManager::isSignedRecurse(Contents* contents,
                                        const Data& decryptorAor)
{
   Pkcs7Contents* pk;
   if ((pk = dynamic_cast<Pkcs7Contents*>(contents)))
   {
#if defined(USE_SSL)
      Contents* decrypted = mSecurity.decrypt(decryptorAor, pk);
      return isSignedRecurse(decrypted, decryptorAor);
#else
      return isSignedRecurse(contents, decryptorAor);
#endif
   }

   MultipartSignedContents* mps;
   if ((mps = dynamic_cast<MultipartSignedContents*>(contents)))
   {
#if defined(USE_SSL)
      return true;
#else
      return false;
#endif
   }

   MultipartAlternativeContents* alt;
   if ((alt = dynamic_cast<MultipartAlternativeContents*>(contents)))
   {
      for (MultipartAlternativeContents::Parts::reverse_iterator i = alt->parts().rbegin();
           i != alt->parts().rend(); ++i)
      {
         if (isSignedRecurse(*i, decryptorAor))
         {
            return true;
         }
      }
   }

   MultipartMixedContents* mult;
   if ((mult = dynamic_cast<MultipartMixedContents*>(contents)))
   {
      for (MultipartMixedContents::Parts::iterator i = mult->parts().begin();
           i != mult->parts().end(); ++i)
      {
         if (isSignedRecurse(*i, decryptorAor))
         {
            return true;
         }
      }
   }

   return false;
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
