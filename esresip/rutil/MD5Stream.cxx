/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.

   Portions of this code are copyright Estacado Systems. Its use is
   subject to the terms of the license agreement under which it has been
   supplied.
 *********************************************************************** */

#include "rutil/MD5Stream.hxx"

#ifdef USE_SSL
#include "rutil/ssl/sslmd5.hxx"
#else
#include "rutil/vmd5.hxx"
#endif

// Remove warning about 'this' use in initiator list - pointer is only stored
#if defined(WIN32) && !defined(__GNUC__)
#pragma warning( disable : 4355 ) // using this in base member initializer list 
#endif

using namespace resip;

MD5Buffer::MD5Buffer() :
#ifdef USE_SSL
   mSSLContext(new SSLMD5Context)
#else
   mContext(new MD5Context)
#endif
{
#ifdef USE_SSL
   SSLMD5Init(mSSLContext);
#else
   MD5Init(mContext);
#endif
   setp(mBuf, mBuf + sizeof(mBuf));
}

MD5Buffer::~MD5Buffer()
{
#ifdef USE_SSL
   delete mSSLContext;
   mSSLContext=0;
#else
   delete mContext;
   mContext=0;
#endif
}

int
MD5Buffer::sync()
{
   size_t len = pptr() - pbase();
   if (len > 0) 
   {
#ifdef USE_SSL
      SSLMD5Update(mSSLContext, reinterpret_cast <unsigned const char*>(pbase()), (unsigned int)len);
#else
      MD5Update(mContext, reinterpret_cast <unsigned const char*>(pbase()), (unsigned int)len);
#endif
      // reset the put buffer
      setp(mBuf, mBuf + sizeof(mBuf));
   }
   return 0;
}

Data 
MD5Buffer::getHex()
{
#ifdef USE_SSL
   SSLMD5Context tmp = *mSSLContext;
   SSLMD5Final((unsigned char*)mBuf, &tmp);
#else
   MD5Context tmp = *mContext;
   MD5Final((unsigned char*)mBuf, &tmp);
#endif
   Data digest(Data::Share, (const char*)mBuf,16);
   return digest.hex();   
}

Data
MD5Buffer::getBin()
{
#ifdef USE_SSL
   SSLMD5Context tmp = *mSSLContext;
   SSLMD5Final((unsigned char*)mBuf, &tmp);
#else
   MD5Context tmp = *mContext;
   MD5Final((unsigned char*)mBuf, &tmp);
#endif
   Data digest(Data::Share, (const char*)mBuf,16);
   return digest;
}

int
MD5Buffer::overflow(int c)
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

MD5Stream::MD5Stream()
   : std::ostream(this)
{
}

MD5Stream::~MD5Stream()
{}

Data 
MD5Stream::getHex()
{
   flush();
   return MD5Buffer::getHex();
   //return mStreambuf.getHex();
}

Data
MD5Stream::getBin()
{
   flush();
   return MD5Buffer::getBin();
}

/* ====================================================================
 * 
 * Portions of this file may fall under the following license. The
 * portions to which the following text applies are available from:
 * 
 *   http://www.resiprocate.org/
 * 
 * Any portion of this code that is not freely available from the
 * Resiprocate project webpages is COPYRIGHT ESTACADO SYSTEMS, LLC.
 * All rights reserved.
 * 
 * ====================================================================
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
