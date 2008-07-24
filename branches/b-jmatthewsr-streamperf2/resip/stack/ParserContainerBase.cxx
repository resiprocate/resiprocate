#include <cassert>

#include "ParserContainerBase.hxx"
#include "resip/stack/Embedded.hxx"

using namespace resip;
using namespace std;;

ParserContainerBase::ParserContainerBase(Headers::Type type)
   : mType(type)
{}

ParserContainerBase::ParserContainerBase(const ParserContainerBase& rhs)
   : mType(rhs.mType)
{
   for (std::vector<ParserCategory*>::const_iterator i = rhs.mParsers.begin(); 
        i != rhs.mParsers.end(); ++i)
   {
      mParsers.push_back((*i)->clone());
   }
}

ParserContainerBase::~ParserContainerBase()
{
   clear();
}

void 
ParserContainerBase::clear()
{
   for (std::vector<ParserCategory*>::const_iterator i = mParsers.begin(); 
        i != mParsers.end(); i++)
   {
      delete *i;
   }
   mParsers.clear();
}

ParserContainerBase&
ParserContainerBase::operator=(const ParserContainerBase& rhs)
{
   if (this != &rhs)
   {
      clear();
      for (std::vector<ParserCategory*>::const_iterator i = rhs.mParsers.begin(); 
           i != rhs.mParsers.end(); ++i)
      {
         mParsers.push_back((*i)->clone());
      }
   }
   return *this;
}

bool 
ParserContainerBase::empty() const 
{ 
   return mParsers.empty(); 
}

size_t
ParserContainerBase::size() const 
{
   return mParsers.size(); 
}

ParserCategory* 
ParserContainerBase::front()
{
   return mParsers.front();
}

void
ParserContainerBase::pop_front() 
{
   delete mParsers.front(); 
   mParsers.erase(mParsers.begin()); 
}
 
void
ParserContainerBase::pop_back() 
{
   delete mParsers.back();
   mParsers.pop_back(); 
}

void
ParserContainerBase::append(const ParserContainerBase& source) 
{
   for (std::vector<ParserCategory*>::const_iterator i = source.mParsers.begin(); 
        i != source.mParsers.end(); ++i)
   {
      mParsers.push_back((*i)->clone());
   }
}

std::ostream& 
ParserContainerBase::encode(const Data& headerName, 
                            std::ostream& str) const
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
         
      for (std::vector<ParserCategory*>::const_iterator i = mParsers.begin(); 
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

         (*i)->encode(str);
      }

      str << Symbols::CRLF;
   }
         
   return str;
}

std::ostream&
ParserContainerBase::encodeEmbedded(const Data& headerName, 
                                    std::ostream& str) const
{
   assert(!headerName.empty());

   if (!mParsers.empty())
   {

      bool first = true;
      for (std::vector<ParserCategory*>::const_iterator i = mParsers.begin(); 
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
            (*i)->encode(s);
         }
         str << Embedded::encode(buf);
      }
   }
   return str;
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
