#include "rutil/ResipAssert.h"

#include "resip/stack/ParserContainerBase.hxx"
#include "resip/stack/Embedded.hxx"

using namespace resip;
using namespace std;;

const ParserContainerBase::HeaderKit ParserContainerBase::HeaderKit::Empty;

ParserContainerBase::ParserContainerBase(Headers::Type type)
   : mType(type),
     mPool(0)
{}

ParserContainerBase::ParserContainerBase(const ParserContainerBase& rhs)
   : mType(rhs.mType),
     mParsers(),
     mPool(0)
{
   copyParsers(rhs.mParsers);
}

ParserContainerBase::ParserContainerBase(Headers::Type type,
                                          PoolBase& pool)
   : mType(type),
     mParsers(StlPoolAllocator<HeaderKit, PoolBase>(&pool)),
     mPool(&pool)
{}

ParserContainerBase::ParserContainerBase(const ParserContainerBase& rhs,
                                          PoolBase& pool)
   : mType(rhs.mType),
     mParsers(StlPoolAllocator<HeaderKit, PoolBase>(&pool)),
     mPool(&pool)
{
   copyParsers(rhs.mParsers);
}

ParserContainerBase::~ParserContainerBase()
{
   freeParsers();
}

ParserContainerBase&
ParserContainerBase::operator=(const ParserContainerBase& rhs)
{
   if (this != &rhs)
   {
      freeParsers();
      mParsers.clear();
      copyParsers(rhs.mParsers);
   }
   return *this;
}

void
ParserContainerBase::pop_front() 
{
   resip_assert(!mParsers.empty());
   freeParser(mParsers.front());
   mParsers.erase(mParsers.begin());
}
 
void
ParserContainerBase::pop_back() 
{
   resip_assert(!mParsers.empty());
   freeParser(mParsers.back());
   mParsers.pop_back(); 
}

void
ParserContainerBase::append(const ParserContainerBase& source) 
{
   copyParsers(source.mParsers);
}

EncodeStream& 
ParserContainerBase::encode(const Data& headerName, 
                            EncodeStream& str) const
{
   // !jf! this is not strictly correct since some headers are allowed to
   // be empty: Supported, Accept-Encoding, Allow-Events, Allow,
   // Accept,Accept-Language 
   if (!mParsers.empty())
   {
      if (!headerName.empty())
      {
         str << headerName << Symbols::COLON[0] << Symbols::SPACE[0];
      }
         
      for (Parsers::const_iterator i = mParsers.begin(); 
           i != mParsers.end(); ++i)
      {
         if (i != mParsers.begin())
         {
            if (Headers::isCommaEncoding(mType))
            {
               str << Symbols::COMMA[0] << Symbols::SPACE[0];
            }
            else
            {
               str << Symbols::CRLF << headerName << Symbols::COLON[0] << Symbols::SPACE[0];
            }
         }

         i->encode(str);
      }

      str << Symbols::CRLF;
   }
         
   return str;
}

EncodeStream&
ParserContainerBase::encodeEmbedded(const Data& headerName, 
                                    EncodeStream& str) const
{
   resip_assert(!headerName.empty());

   if (!mParsers.empty())
   {

      bool first = true;
      for (Parsers::const_iterator i = mParsers.begin(); 
           i != mParsers.end(); ++i)
      {
         if (first)
         {
            first = false;
         }
         else
         {
            str << Symbols::AMPERSAND;
         }

         str << headerName << Symbols::EQUALS;
         Data buf;
         {
            DataStream s(buf);
            i->encode(s);
         }
         str << Embedded::encode(buf);
      }
   }
   return str;
}

void 
ParserContainerBase::copyParsers(const Parsers& parsers)
{
   mParsers.reserve(mParsers.size() + parsers.size());
   for(Parsers::const_iterator p=parsers.begin(); p!=parsers.end(); ++p)
   {
      // Copy c'tor and assignment operator for HeaderKit are actually poor
      // man's move semantics, so we have to implement real copy semantics here.
      mParsers.push_back(HeaderKit::Empty);

      HeaderKit& kit(mParsers.back());
      if(p->pc)
      {
         kit.pc = makeParser(*(p->pc));
      } 
      else 
      {
         kit.hfv = p->hfv;
      }
   }
}

void 
ParserContainerBase::freeParsers()
{
   for(Parsers::iterator p=mParsers.begin(); p!=mParsers.end(); ++p)
   {
      freeParser(*p);
   }
}


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2005
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
