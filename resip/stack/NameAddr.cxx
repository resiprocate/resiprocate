#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/NameAddr.hxx"
#include "rutil/ParseException.hxx"
#include "resip/stack/UnknownParameter.hxx"
#include "rutil/Data.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

//====================
// NameAddr:
//====================
NameAddr::NameAddr() : 
   ParserCategory(),
   mAllContacts(false),
   mDisplayName(),
   mUnknownUriParametersBuffer(0)
{}

NameAddr::NameAddr(const HeaderFieldValue& hfv,
                   Headers::Type type,
                   PoolBase* pool)
   : ParserCategory(hfv, type, pool), 
     mAllContacts(false),
     mUri(pool),
     mDisplayName(),
     mUnknownUriParametersBuffer(0)
{}

NameAddr::NameAddr(const NameAddr& rhs,
                   PoolBase* pool)
   : ParserCategory(rhs, pool),
     mAllContacts(rhs.mAllContacts),
     mUri(rhs.mUri, pool),
     mDisplayName(rhs.mDisplayName),
     mUnknownUriParametersBuffer(0)
{}

static const Data parseContext("NameAddr constructor");
NameAddr::NameAddr(const Data& unparsed, bool preCacheAor)
   : ParserCategory(),
     mAllContacts(false),
     mDisplayName(),
     mUnknownUriParametersBuffer(0)
{
   HeaderFieldValue hfv(unparsed.data(), unparsed.size());
   // must copy because parse creates overlays
   NameAddr tmp(hfv, Headers::UNKNOWN);
   tmp.checkParsed();
   *this = tmp;
   if(preCacheAor)
   {
      mUri.getAor();
   }
}

NameAddr::NameAddr(const Uri& uri)
   : ParserCategory(),
     mAllContacts(false),
     mUri(uri),
     mDisplayName(),
     mUnknownUriParametersBuffer(0)
{}

NameAddr::~NameAddr()
{
   if(mUnknownUriParametersBuffer) 
   {          
      delete mUnknownUriParametersBuffer;
   }
}

NameAddr&
NameAddr::operator=(const NameAddr& rhs)
{
   if (this != &rhs)
   {
      resip_assert( &rhs != 0 );
      
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

ParserCategory *
NameAddr::clone(void* location) const
{
   return new (location) NameAddr(*this);
}

ParserCategory* 
NameAddr::clone(PoolBase* pool) const
{
   return new (pool) NameAddr(*this, pool);
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
   pb.assertNotEof();
   bool laQuote = false;
   bool starContact = false;
   
   if (*pb.position() == Symbols::STAR[0])
   {
      pb.skipChar(Symbols::STAR[0]);
      pb.skipWhitespace();
      if (pb.eof() || *pb.position() == Symbols::SEMI_COLON[0])
      {
         starContact = true;
      }
   }

   if (starContact)
   {
      mAllContacts = true;
      // now fall through to parse header parameters
   }
   else
   {
      pb.reset(start);
      if (*pb.position() == Symbols::DOUBLE_QUOTE[0])
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
      else if (*pb.position() == Symbols::LA_QUOTE[0])
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
         if(mUri.mUnknownParameters.size() > 0)
         {
            resip_assert(!mUnknownUriParametersBuffer);
            mUnknownUriParametersBuffer = new Data;
            {  // Scope stream
               oDataStream str(*mUnknownUriParametersBuffer);
               // deal with Uri/NameAddr parameter ambiguity
               // heuristically assign Uri parameters to the Uri
               for (ParameterList::iterator it = mUri.mUnknownParameters.begin(); 
                  it != mUri.mUnknownParameters.end(); ++it)
               {
                  // We're just going to assume all unknown (to Uri) params really
                  // belong on the header. This is not necessarily the case.
                  str << ";";
                  (*it)->encode(str);
               }
            }
            mUri.clearUnknownParameters();
            ParseBuffer pb2(*mUnknownUriParametersBuffer);
            parseParameters(pb2);
         }
      }
   }
   parseParameters(pb);
}

EncodeStream&
NameAddr::encodeParsed(EncodeStream& str) const
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

ParameterTypes::Factory NameAddr::ParameterFactories[ParameterTypes::MAX_PARAMETER]={0};

Parameter* 
NameAddr::createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool)
{
   if(type > ParameterTypes::UNKNOWN && type < ParameterTypes::MAX_PARAMETER && ParameterFactories[type])
   {
      return ParameterFactories[type](type, pb, terminators, pool);
   }
   return 0;
}

bool 
NameAddr::exists(const Param<NameAddr>& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

void 
NameAddr::remove(const Param<NameAddr>& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)                                                      \
_enum##_Param::DType&                                                                                           \
NameAddr::param(const _enum##_Param& paramType)                                                           \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p =                                                                                     \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));                            \
   if (!p)                                                                                                      \
   {                                                                                                            \
      p = new _enum##_Param::Type(paramType.getTypeNum());                                                      \
      mParameters.push_back(p);                                                                                 \
   }                                                                                                            \
   return p->value();                                                                                           \
}                                                                                                               \
                                                                                                                \
const _enum##_Param::DType&                                                                                     \
NameAddr::param(const _enum##_Param& paramType) const                                                     \
{                                                                                                               \
   checkParsed();                                                                                               \
   _enum##_Param::Type* p =                                                                                     \
      static_cast<_enum##_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));                            \
   if (!p)                                                                                                      \
   {                                                                                                            \
      InfoLog(<< "Missing parameter " _name " " << ParameterTypes::ParameterNames[paramType.getTypeNum()]);     \
      DebugLog(<< *this);                                                                                       \
      throw Exception("Missing parameter " _name, __FILE__, __LINE__);                                          \
   }                                                                                                            \
   return p->value();                                                                                           \
}

defineParam(data, "data", ExistsParameter, "RFC 3840");
defineParam(control, "control", ExistsParameter, "RFC 3840");
defineParam(mobility, "mobility", QuotedDataParameter, "RFC 3840"); // mobile|fixed
defineParam(description, "description", QuotedDataParameter, "RFC 3840"); // <> quoted
defineParam(events, "events", QuotedDataParameter, "RFC 3840"); // list
defineParam(priority, "priority", QuotedDataParameter, "RFC 3840"); // non-urgent|normal|urgent|emergency
defineParam(methods, "methods", QuotedDataParameter, "RFC 3840"); // list
defineParam(schemes, "schemes", QuotedDataParameter, "RFC 3840"); // list
defineParam(application, "application", ExistsParameter, "RFC 3840");
defineParam(video, "video", ExistsParameter, "RFC 3840");
defineParam(language, "language", QuotedDataParameter, "RFC 3840"); // list
defineParam(type, "type", QuotedDataParameter, "RFC 3840"); // list
defineParam(isFocus, "isfocus", ExistsParameter, "RFC 3840");
defineParam(actor, "actor", QuotedDataParameter, "RFC 3840"); // principal|msg-taker|attendant|information
defineParam(text, "text", ExistsOrDataParameter, "RFC 3840");
defineParam(extensions, "extensions", QuotedDataParameter, "RFC 3840"); //list
defineParam(Instance, "+sip.instance", QuotedDataParameter, "RFC 5626");  // <> quoted
defineParam(regid, "reg-id", UInt32Parameter, "RFC 5626");
defineParam(pubGruu, "pub-gruu", QuotedDataParameter, "RFC 5627");
defineParam(tempGruu, "temp-gruu", QuotedDataParameter, "RFC 5627");
defineParam(expires, "expires", UInt32Parameter, "RFC 3261");
defineParam(q, "q", QValueParameter, "RFC 3261");
defineParam(tag, "tag", DataParameter, "RFC 3261");
defineParam(index, "index", DataParameter, "RFC 4244");
defineParam(rc, "rc", DataParameter, "RFC 4244-bis");
defineParam(mp, "mp", DataParameter, "RFC 4244-bis");
defineParam(np, "np", DataParameter, "RFC 4244-bis");

#undef defineParam

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
