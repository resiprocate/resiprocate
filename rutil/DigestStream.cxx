// For testing - also uncomment this in testDigestStream.cxx to test non-SSL MD5 and SHA1
//#undef USE_SSL

#include "rutil/DigestStream.hxx"

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

#ifdef USE_SSL
   #include <openssl/evp.h> 
   #if OPENSSL_VERSION_NUMBER < 0x10100000L // openssl 1.1.0 api changes
   static EVP_MD_CTX* EVP_MD_CTX_new()
   {
      return new EVP_MD_CTX;
   }
   static void EVP_MD_CTX_free(EVP_MD_CTX* pCtx)
   {
      EVP_MD_CTX_cleanup(pCtx);
      delete pCtx;
   }
   #endif
#else
   #include "rutil/vmd5.hxx"
   #include "rutil/Sha1.hxx"
#endif

using namespace resip;

DigestBuffer::DigestBuffer(DigestType digestType) :
   mDigestType(digestType),
   mContext(nullptr),
   mBytesTaken(0),
#ifdef USE_SSL
   mFinalBin(EVP_MAX_MD_SIZE, Data::Preallocate)
#else
   mFinalBin(16, Data::Preallocate)
#endif
{
   int ret = 0;
   switch (digestType)
   {
      case MD5:
#ifdef USE_SSL
         mContext = EVP_MD_CTX_new();
         EVP_MD_CTX_init((EVP_MD_CTX*)mContext);
         ret = EVP_DigestInit_ex((EVP_MD_CTX*)mContext, EVP_md5(), nullptr);
#else
         mContext = new MD5Context;
         MD5Init((MD5Context*)mContext);
         ret = 1; 
#endif
         break;
      case SHA1:
#ifdef USE_SSL
         mContext = EVP_MD_CTX_new();
         EVP_MD_CTX_init((EVP_MD_CTX*)mContext);
         ret = EVP_DigestInit_ex((EVP_MD_CTX*)mContext, EVP_sha1(), nullptr);
#else
         mContext = new Sha1();
         ret = 1;
#endif
         break;
#ifdef USE_SSL
      case SHA256:
         mContext = EVP_MD_CTX_new();
         EVP_MD_CTX_init((EVP_MD_CTX*)mContext);
         ret = EVP_DigestInit_ex((EVP_MD_CTX*)mContext, EVP_sha256(), nullptr);
         break;
      case SHA512:
         mContext = EVP_MD_CTX_new();
         EVP_MD_CTX_init((EVP_MD_CTX*)mContext);
         ret = EVP_DigestInit_ex((EVP_MD_CTX*)mContext, EVP_sha512(), nullptr);
         break;
      case SHA512_256:
         mContext = EVP_MD_CTX_new();
         EVP_MD_CTX_init((EVP_MD_CTX*)mContext);
         ret = EVP_DigestInit_ex((EVP_MD_CTX*)mContext, EVP_sha512_256(), nullptr);
         break;
#endif
      default:
         resip_assert(false);
         break;
   }
   if (!ret)
   {
      throw std::runtime_error("Failed to initialize digest context");
   }
   setp(mBuf, mBuf + sizeof(mBuf));
}

DigestBuffer::~DigestBuffer()
{
#ifdef USE_SSL
   EVP_MD_CTX_free((EVP_MD_CTX*)mContext);
#else
   if (mDigestType == MD5)
   {
      delete (MD5Context*)mContext;
   }
   else if (mDigestType == SHA1)
   {
      delete (resip::Sha1*)mContext;
   }
#endif
}

int
DigestBuffer::sync()
{
   if (!mContext) return -1;

   size_t len = pptr() - pbase();
   if (len > 0) 
   {
#ifdef USE_SSL
      EVP_DigestUpdate((EVP_MD_CTX*)mContext, reinterpret_cast <unsigned const char*>(pbase()), len);
#else
      if (mDigestType == MD5)
      {
         MD5Update((MD5Context*)mContext, reinterpret_cast <unsigned const char*>(pbase()), (unsigned int)len);
      }
      else if (mDigestType == SHA1)
      {
         ((Sha1*)mContext)->update(std::string(pbase(), len));
      }
#endif
      // reset the put buffer
      setp(mBuf, mBuf + sizeof(mBuf));
      mBytesTaken += len;
   }
   return 0;
}

int
DigestBuffer::overflow(int c)
{
   sync();
   if (c != -1) 
   {
      mBuf[0] = c;
      pbump(1);
      return c;
   }
   return 0;
}

Data
DigestBuffer::getHex()
{
   return getBin().hex();
}

const Data&
DigestBuffer::getBin()
{
   // If not already calculated, then calcuate it
   if (mFinalBin.size() == 0)
   {
#ifdef USE_SSL
      unsigned int len = EVP_MAX_MD_SIZE;
      EVP_DigestFinal_ex((EVP_MD_CTX*)mContext, (unsigned char*)mFinalBin.getBuf(len), &len);
      mFinalBin.truncate(len);
#else
      if (mDigestType == MD5)
      {
         MD5Final((unsigned char*)mFinalBin.getBuf(16), (MD5Context*)mContext);
      }
      else if (mDigestType == SHA1)
      {
         mFinalBin = ((Sha1*)mContext)->finalBin();
      }
#endif
   }
   return mFinalBin;
}

DigestStream::DigestStream(DigestType digestType)
   : DigestBuffer(digestType), std::ostream(this)
{
}

DigestStream::~DigestStream()
{
}

Data
DigestStream::getHex()
{
   flush();
   return DigestBuffer::getHex();
}

const Data&
DigestStream::getBin()
{
   flush();
   return DigestBuffer::getBin();
}

size_t
DigestStream::bytesTaken()
{
   return DigestBuffer::bytesTaken();
}

static Data md5Name("MD5");
static Data sha1Name("SHA1");
#ifdef USE_SSL
static Data sha256Name("SHA-256");
static Data sha512Name("SHA-512");
static Data sha512_256Name("SHA-512-256");
#endif
const Data&
DigestStream::getDigestName(DigestType digestType)
{
   // Note:  This name formatting aligns with what is needed in the SIP
   //        Digest authentication header 'algorithm' parameter (where it makes sense).
   switch (digestType)
   {
      case MD5:
         return md5Name;
      case SHA1:
         return sha1Name;
#ifdef USE_SSL
      case SHA256:
         return sha256Name;
      case SHA512:
         return sha512Name;
      case SHA512_256:
         return sha512_256Name;
#endif
      default:
         resip_assert(false);
         return Data::Empty;
   }
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
