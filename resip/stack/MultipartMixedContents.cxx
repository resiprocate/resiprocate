#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/MultipartMixedContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

bool 
MultipartMixedContents::init()
{
   static ContentsFactory<MultipartMixedContents> factory;
   (void)factory;
   return true;
}

MultipartMixedContents::MultipartMixedContents()
   : Contents(getStaticType()),
     mContents()
{
   setBoundary();
}

MultipartMixedContents::MultipartMixedContents(const Mime& contentsType)
   : Contents(contentsType),
     mContents()
{
   if (!mType.exists(p_boundary))
   {
      setBoundary();
   }
}

MultipartMixedContents::MultipartMixedContents(const HeaderFieldValue& hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mContents()
{
   if (!mType.exists(p_boundary))
   {
      setBoundary();
   }
}

MultipartMixedContents::MultipartMixedContents(const MultipartMixedContents& rhs)
   : Contents(rhs),
     mContents()
{
   vector<Contents*>::const_iterator j;

   // .bwc. Don't trigger a parse of the original by calling parts()
   const vector<Contents*>& list = rhs.mContents;
   
   for ( j = list.begin(); 
         j != list.end(); ++j)
   {
      resip_assert( *j );
      mContents.push_back( (*j)->clone() );
   }
}

void
MultipartMixedContents::setBoundary()
{
   Data boundaryToken = Random::getRandomHex(8);
   mType.param(p_boundary) = boundaryToken;
}

void
MultipartMixedContents::setBoundary(const Data& boundary)
{
    mType.param(p_boundary) = boundary;
}

void
MultipartMixedContents::clear()
{
   for (vector<Contents*>::iterator i = mContents.begin(); 
        i != mContents.end(); ++i)
   {
      delete *i;
   }
}

MultipartMixedContents::~MultipartMixedContents()
{
   clear();
}

MultipartMixedContents&
MultipartMixedContents::operator=(const MultipartMixedContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      clear();
      
      for (vector<Contents*>::iterator i = mContents.begin(); 
           i != mContents.end(); ++i)
      {
         mContents.push_back( (*i)->clone() );
      }
   }
   return *this;
}

Contents* 
MultipartMixedContents::clone() const
{
   return new MultipartMixedContents(*this);
}

const Mime& 
MultipartMixedContents::getStaticType() 
{
   static Mime type("multipart","mixed");
   return type;
}

EncodeStream& 
MultipartMixedContents::encodeParsed(EncodeStream& str) const
{
   const Data& boundaryToken = mType.param(p_boundary);
   Data boundary(boundaryToken.size() + 2, Data::Preallocate);
   boundary = Symbols::DASHDASH;
   boundary += boundaryToken;
   boundary.replace("\"", ""); // remove quotes

   resip_assert( mContents.size() > 0 );
   
   bool first = true;
   for (vector<Contents*>::const_iterator i = mContents.begin(); 
        i != mContents.end(); ++i)
   {
      if (!first)
      {
         str << Symbols::CRLF;
      }
      else
      {
         first = false;
      }
      str << boundary << Symbols::CRLF;
      (*i)->encodeHeaders(str);
      (*i)->encode(str);
   }

   // Note:  last CRLF isn't strictly required by RFC2046 ABNF, but some implementations seem to require it so we include it
   str << Symbols::CRLF << boundary << Symbols::DASHDASH << Symbols::CRLF;
   return str;
}

// The boundary delimiter MUST occur at the beginning of a line, i.e., following
// a CRLF, and the initial CRLF is considered to be attached to the boundary
// delimiter line rather than part of the preceding part.
void 
MultipartMixedContents::parse(ParseBuffer& pb)
{
   const Data& boundaryToken = mType.param(p_boundary);
   
   Data boundary(boundaryToken.size() + 4, Data::Preallocate);
   boundary += Symbols::CRLF;
   boundary += Symbols::DASHDASH;
   boundary += boundaryToken;

   Data boundaryNoCRLF(boundaryToken.size() + 2, Data::Preallocate);
   boundaryNoCRLF += Symbols::DASHDASH;
   boundaryNoCRLF += boundaryToken;

   pb.skipToChars(boundaryNoCRLF);
   pb.skipN((int)boundaryNoCRLF.size());
   pb.assertNotEof();

   do
   {
      // skip over boundary
 
      if( pb.eof() || *pb.position() != Symbols::CR[0] )
      {
         throw Exception("Invalid line ending, missing CR",__FILE__,__LINE__);
      }
      pb.skipChar();
      if( pb.eof() || *pb.position() != Symbols::LF[0] )
      {
         throw Exception("Invalid line ending, missing LF",__FILE__,__LINE__);
      }
      pb.skipChar();
      
      pb.assertNotEof();

      const char* headerStart = pb.position();

      // pull out contents type only
      pb.skipToChars("Content-Type");
      if (pb.eof())
      {
         pb.reset(headerStart);
         pb.skipToChars("CONTENT-TYPE");
      }
      pb.assertNotEof();

      pb.skipToChar(Symbols::COLON[0]);
      pb.skipChar();
      pb.assertNotEof();
      
      pb.skipWhitespace();
      const char* typeStart = pb.position();
      pb.assertNotEof();
      
      // determine contents-type header buffer
      pb.skipToTermCRLF();
      pb.assertNotEof();

      ParseBuffer subPb(typeStart, pb.position() - typeStart);
      Mime contentType;
      contentType.parse(subPb);
      
      pb.assertNotEof();

      // determine body start
      pb.reset(typeStart);
      const char* bodyStart = pb.skipToChars(Symbols::CRLFCRLF);
      pb.assertNotEof();
      bodyStart += 4;

      // determine contents body buffer
      pb.skipToChars(boundary);
      pb.assertNotEof();
      Data tmp;
      pb.data(tmp, bodyStart);
      // create contents against body
      mContents.push_back(createContents(contentType, tmp));
      // pre-parse headers
      ParseBuffer headersPb(headerStart, bodyStart-4-headerStart);
      mContents.back()->preParseHeaders(headersPb);

      pb.skipN((int)boundary.size());

      const char* loc = pb.position();
      pb.skipChar();
      pb.skipChar();
      Data next;
      pb.data(next, loc);

      if ( next == Symbols::DASHDASH )
      {
         break;
      }
      pb.reset( loc );
   }
   while ( !pb.eof() );

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
