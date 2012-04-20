#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/StatusLine.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// StatusLine:
//====================
StatusLine::StatusLine() 
   : StartLine(), 
     mResponseCode(-1),
     mSipVersion(Data::Share,Symbols::DefaultSipVersion), 
     mReason()
{}

StatusLine::StatusLine(const HeaderFieldValue& hfv)
   : StartLine(hfv), 
     mResponseCode(-1), 
     mSipVersion(Data::Share,Symbols::DefaultSipVersion), 
     mReason() 
{}

StatusLine::StatusLine(const char* buf, int length) :
   StartLine(buf, length),
   mResponseCode(-1), 
   mSipVersion(Data::Share,Symbols::DefaultSipVersion), 
   mReason() 
{}

StatusLine::StatusLine(const StatusLine& rhs)
   : StartLine(rhs),
     mResponseCode(rhs.mResponseCode),
     mSipVersion(rhs.mSipVersion),
     mReason(rhs.mReason)
{}
     
StatusLine&
StatusLine::operator=(const StatusLine& rhs)
{
   if (this != &rhs)
   {
      StartLine::operator=(rhs);
      mResponseCode = rhs.mResponseCode;
      mSipVersion = rhs.mSipVersion;
      mReason = rhs.mReason;
   }
   return *this;
}

StartLine *
StatusLine::clone() const
{
   return new StatusLine(*this);
}

StartLine *
StatusLine::clone(void* location) const
{
   return new (location) StatusLine(*this);
}

int&
StatusLine::responseCode()
{
   checkParsed();
   return mResponseCode;
}

int 
StatusLine::responseCode() const
{
   checkParsed();
   return mResponseCode;
}

int& 
StatusLine::statusCode()
{
   checkParsed();
   return mResponseCode;
}

int 
StatusLine::statusCode() const
{
   checkParsed();
   return mResponseCode;
}

const Data& 
StatusLine::getSipVersion() const
{
   checkParsed();
   return mSipVersion;
}

Data& 
StatusLine::reason()
{
   checkParsed();
   return mReason;
}

const Data& 
StatusLine::reason() const
{
   checkParsed();
   return mReason;
}

void
StatusLine::parse(ParseBuffer& pb)
{
   const char* start = pb.skipWhitespace();
   pb.skipNonWhitespace();
   pb.data(mSipVersion, start);

   start = pb.skipWhitespace();
   mResponseCode = pb.integer();
   start = pb.skipWhitespace();
   pb.skipToEnd();
   pb.data(mReason, start);
}

EncodeStream&
StatusLine::encodeParsed(EncodeStream& str) const
{
   str << mSipVersion << Symbols::SPACE 
       << mResponseCode << Symbols::SPACE
       << mReason;
   return str;
}

const Data& 
StatusLine::errorContext() const
{
   static const Data statLine("Status Line");
   return statLine;
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
