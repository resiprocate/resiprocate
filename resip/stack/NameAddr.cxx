#if defined(HAVE_CONFIG_H)
#include "resip/stack/config.hxx"
#endif

#include "resip/stack/NameAddr.hxx"
#include "rutil/ParseException.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// NameAddr:
//====================
NameAddr::NameAddr() : 
   ParserCategory(),
   mAllContacts(false),
   mDisplayName()
{}

NameAddr::NameAddr(HeaderFieldValue* hfv,
                   Headers::Type type)
   : ParserCategory(hfv, type), 
     mAllContacts(false),
     mDisplayName()
{}

NameAddr::NameAddr(const NameAddr& rhs)
   : ParserCategory(rhs),
     mAllContacts(rhs.mAllContacts),
     mUri(rhs.mUri),
     mDisplayName(rhs.mDisplayName)
{}

static const Data parseContext("NameAddr constructor");
NameAddr::NameAddr(const Data& unparsed)
   : ParserCategory(),
     mAllContacts(false),
     mDisplayName()
{
   // must copy because parse creates overlays
   NameAddr tmp;
   ParseBuffer pb(unparsed, parseContext);
   tmp.parse(pb);
   *this = tmp;
}

NameAddr::NameAddr(const Uri& uri)
   : ParserCategory(),
     mAllContacts(false),
     mUri(uri),
     mDisplayName()
{}

NameAddr::~NameAddr()
{}

NameAddr&
NameAddr::operator=(const NameAddr& rhs)
{
   if (this != &rhs)
   {
      assert( &rhs != 0 );
      
      ParserCategory::operator=(rhs);
      mAllContacts = rhs.mAllContacts;
      mDisplayName = rhs.mDisplayName;
      mUri = rhs.mUri;
   }
   return *this;
}

bool 
NameAddr::operator==(const NameAddr& other) const
{
    return uri() == other.uri() && displayName() == other.displayName();
}

bool
NameAddr::operator<(const NameAddr& rhs) const
{
   return uri() < rhs.uri();
}

ParserCategory *
NameAddr::clone() const
{
   return new NameAddr(*this);
}

const Uri&
NameAddr::uri() const 
{
   checkParsed(); 
   return mUri;
}

Uri&
NameAddr::uri()
{
   checkParsed(); 
   return mUri;
}

Data& 
NameAddr::displayName()
{
   checkParsed(); 
   return mDisplayName;
}

const Data& 
NameAddr::displayName() const 
{
   checkParsed(); 
   return mDisplayName;
}

bool 
NameAddr::isAllContacts() const 
{
   checkParsed(); 
   return mAllContacts;
}

void 
NameAddr::setAllContacts()
{
   mAllContacts = true;
}

void
NameAddr::parse(ParseBuffer& pb)
{
   const char* start;
   start = pb.skipWhitespace();
   bool laQuote = false;

   if (!pb.eof() && *pb.position() == Symbols::STAR[0])
   {
      mAllContacts = true;
      pb.skipChar(Symbols::STAR[0]);
      pb.skipWhitespace();
      // now fall through to parse header parameters
   }
   else
   {
      if (!pb.eof() && *pb.position() == Symbols::DOUBLE_QUOTE[0])
      {
         start = pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
         pb.skipToEndQuote();
         pb.data(mDisplayName, start);
         pb.skipChar(Symbols::DOUBLE_QUOTE[0]);
         laQuote = true;
         pb.skipToChar(Symbols::LA_QUOTE[0]);
         if (pb.eof())
         {
            throw ParseException("Expected '<'", 
                                 "NameAddr", 
                                 __FILE__, 
                                 __LINE__);
         }
         else
         {
            pb.skipChar(Symbols::LA_QUOTE[0]);
         }
      }
      else if (!pb.eof() && *pb.position() == Symbols::LA_QUOTE[0])
      {
         pb.skipChar(Symbols::LA_QUOTE[0]);
         laQuote = true;
      }
      else
      {
         start = pb.position();
         pb.skipToChar(Symbols::LA_QUOTE[0]);
         if (pb.eof())
         {
            pb.reset(start);
         }
         else
         {
            laQuote = true;
            pb.skipBackWhitespace();
            pb.data(mDisplayName, start);
            pb.skipToChar(Symbols::LA_QUOTE[0]);
            pb.skipChar(Symbols::LA_QUOTE[0]);
         }
      }
      pb.skipWhitespace();
      mUri.parse(pb);
      if (laQuote)
      {
         pb.skipChar(Symbols::RA_QUOTE[0]);
         pb.skipWhitespace();
         // now fall through to parse header parameters
      }
      else
      {
         // deal with Uri/NameAddr parameter ambiguity
         // heuristically assign Uri parameters to the Uri
         swap(mParameters, mUri.mParameters);
         swap(mUnknownParameters, mUri.mUnknownParameters);
         for (ParameterList::iterator it = mParameters.begin(); 
              it != mParameters.end();)
         {
            switch ((*it)->getType())
            {
               case ParameterTypes::comp:             
               case ParameterTypes::lr:
               case ParameterTypes::maddr:
               case ParameterTypes::method: 
               case ParameterTypes::transport:
               case ParameterTypes::ttl:
               case ParameterTypes::user:
               {
                  mUri.mParameters.push_back(*it);
                  it = mParameters.erase(it);
                  break;
               }
               default:
               {
                  it++;
               }
            }
            // fall through to parse any parameters left which are not Uri parameters
         }
      }
   }
   parseParameters(pb);
}

ostream&
NameAddr::encodeParsed(ostream& str) const
{
   //bool displayName = !mDisplayName.empty();
  if (mAllContacts)
  {
     str << Symbols::STAR;
  }
  else
  {
     if (!mDisplayName.empty())
     {
#ifndef HANDLE_EMBEDDED_QUOTES_DNAME
        // .dlb. doesn't deal with embedded quotes
        str << Symbols::DOUBLE_QUOTE << mDisplayName << Symbols::DOUBLE_QUOTE;
#else
        // does nothing if display name is properly quoted
        if (mustQuoteDisplayName())
        {
           str << Symbols::DOUBLE_QUOTE;
           for (unsigned int i=0; i < mDisplayName.size(); i++)
           {
              char c = mDisplayName[i];
              switch(c)
              {
                 case '"':
                 case '\\':
                    str << '\\' << c;
                    break;
                 default:
                    str << c;
              }
           }
           str << Symbols::DOUBLE_QUOTE;
        }
        else
        {
           str << mDisplayName;           
        }
#endif
     }     
     str << Symbols::LA_QUOTE;
     mUri.encodeParsed(str);
     str << Symbols::RA_QUOTE;
  }
  
  encodeParameters(str);
  return str;
}


bool 
NameAddr::mustQuoteDisplayName() const
{
   if (mDisplayName.empty())
   {
      return false;
   }
   ParseBuffer pb(mDisplayName.data(), mDisplayName.size());   
   
   //shouldn't really be any leading whitespace
   pb.skipWhitespace();
   if (pb.eof())
   {
      return false;
   }
   if ((*pb.position() == '"'))
   {
      bool escaped = false;
      while(!pb.eof())
      {
         pb.skipChar();
         if (escaped)
         {
            escaped = false;
         }
         else if (*pb.position() == '\\')
         {
            escaped = true;
         }
         else if (*pb.position() == '"')
         {
            break;
         }
      }
      if (*pb.position() == '"')
      {
         //should only have whitespace left, and really non of that
         pb.skipChar();
         if (pb.eof())
         {
            return false;
         }
         pb.skipWhitespace();
         if (pb.eof())
         {
            return false; //properly quoted
         }
         else
         {
            return true; 
         }
      }
      else
      {
         return true; //imbalanced quotes
      }
   }
   else
   {
      while (!pb.eof())
      {
         const char* start;
         start = pb.skipWhitespace();
         pb.skipNonWhitespace();
		 const char* end = pb.position();
         for (const char* c = start; c < end; c++)
         {
            if ( (*c >= 'a' && *c <= 'z') ||
                 (*c >= 'A' && *c <= 'Z') ||
                 (*c >= '0' && *c <= '9'))
            {
               continue;
            }
            switch(*c)
            {
               case '-':
               case '.':
               case '!':
               case '%':
               case '*':
               case '_':
               case '+':
               case '`':
               case '\'':
               case '~':
                  break;
               default:
                  return true;
            }
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
