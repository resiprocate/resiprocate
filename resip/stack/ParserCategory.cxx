#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/ParserCategory.hxx"
#include "rutil/ParseBuffer.hxx"
#include "resip/stack/SipMessage.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/compat.hxx"

#include "resip/stack/UnknownParameter.hxx"
#include "resip/stack/ExtensionParameter.hxx"

#include <iostream>
#include "rutil/ResipAssert.h"

#include "rutil/Logger.hxx"
//#include "rutil/WinLeakCheck.hxx"  // not compatible with placement new used below

#define RESIPROCATE_SUBSYSTEM Subsystem::SIP

using namespace resip;
using namespace std;

const ParserCategory::ParameterTypeSet 
ParserCategory::EmptyParameterTypeSet; 

ParserCategory::ParserCategory(const HeaderFieldValue& headerFieldValue,
                               Headers::Type headerType,
                               PoolBase* pool)
    : LazyParser(headerFieldValue),
      mParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
      mUnknownParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
      mPool(pool),
      mHeaderType(headerType)
{
}

ParserCategory::ParserCategory(const char* buf, 
                                 int length, 
                                 Headers::Type type,
                                 PoolBase* pool):
   LazyParser(buf, length),
   mParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
   mUnknownParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
   mPool(pool),
   mHeaderType(type)
{}

ParserCategory::ParserCategory(PoolBase* pool)
   : LazyParser(),
     mParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
     mUnknownParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
     mPool(pool),
     mHeaderType(Headers::NONE)
{
}

ParserCategory::ParserCategory(const ParserCategory& rhs,
                               PoolBase* pool)
   : LazyParser(rhs),
     mParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
     mUnknownParameters(StlPoolAllocator<Parameter*, PoolBase>(pool)),
     mPool(pool),
     mHeaderType(rhs.mHeaderType)
{
   if (isParsed())
   {
      copyParametersFrom(rhs);
   }
}

ParserCategory&
ParserCategory::operator=(const ParserCategory& rhs)
{
   if (this != &rhs)
   {
      clear();
      mHeaderType = rhs.mHeaderType;
      LazyParser::operator=(rhs);
      if (rhs.isParsed())
      {
         copyParametersFrom(rhs);
      }
   }
   return *this;
}

void
ParserCategory::clear()
{
   //DebugLog(<<"ParserCategory::clear");
   LazyParser::clear();

   while(!mParameters.empty())
   {
      freeParameter(mParameters.back());
      mParameters.pop_back();
   }

   while(!mUnknownParameters.empty())
   {
      freeParameter(mUnknownParameters.back());
      mUnknownParameters.pop_back();
   }
}

void 
ParserCategory::copyParametersFrom(const ParserCategory& other)
{
   mParameters.reserve(other.mParameters.size());
   mUnknownParameters.reserve(other.mUnknownParameters.size());
   
   for (ParameterList::const_iterator it = other.mParameters.begin();
        it != other.mParameters.end(); it++)
   {
      mParameters.push_back((*it)->clone());
   }
   for (ParameterList::const_iterator it = other.mUnknownParameters.begin();
        it != other.mUnknownParameters.end(); it++)
   {
      mUnknownParameters.push_back((*it)->clone());
   }
}

ParserCategory::~ParserCategory()
{
   clear();
}

const Data&
ParserCategory::param(const ExtensionParameter& param) const
{
   checkParsed();
   Parameter* p = getParameterByData(param.getName());
   if (!p)
   {
      InfoLog(<< "Referenced an unknown parameter " << param.getName());
      throw Exception("Missing unknown parameter", __FILE__, __LINE__);
   }
   return static_cast<UnknownParameter*>(p)->value();
}

Data&
ParserCategory::param(const ExtensionParameter& param)
{
   checkParsed();
   Parameter* p = getParameterByData(param.getName());
   if (!p)
   {
      p = new UnknownParameter(param.getName());
      mUnknownParameters.push_back(p);
   } 
   return static_cast<UnknownParameter*>(p)->value();
}

// removing non-present parameter is allowed      
void
ParserCategory::remove(const ParamBase& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

void 
ParserCategory::remove(const ExtensionParameter& param)
{
   checkParsed();
   removeParameterByData(param.getName());
}

bool 
ParserCategory::exists(const ExtensionParameter& param) const
{
   checkParsed();
   return getParameterByData(param.getName()) != NULL;
}

void 
ParserCategory::removeParametersExcept(const ParameterTypeSet& set)
{
   checkParsed();
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end();)
   {
      if (set.find((*it)->getType()) == set.end())
      {
         freeParameter(*it);
         it = mParameters.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

void 
ParserCategory::clearUnknownParameters()
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      freeParameter(*it);
   }   
   mUnknownParameters.clear();
}

void
ParserCategory::parseParameters(ParseBuffer& pb)
{
   while (!pb.eof() )
   {
      const char* start = pb.position();
      pb.skipWhitespace();

      if (  (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0]) )
      {
         // extract the key
         pb.skipChar();
         const char* keyStart = pb.skipWhitespace();
         static std::bitset<256> terminators1=Data::toBitset(" \t\r\n;=?>"); //!dlb! @ here?
         const char* keyEnd = pb.skipToOneOf(terminators1);  

         if((int)(keyEnd-keyStart) != 0)
         {
            ParameterTypes::Type type = ParameterTypes::getType(keyStart, (unsigned int)(keyEnd - keyStart));
            static std::bitset<256> terminators2 = Data::toBitset(" \t\r\n;?>");
            Parameter* p;
            if (type == ParameterTypes::UNKNOWN || 
               !(p=createParam(type, pb, terminators2,getPool())))
            {
               mUnknownParameters.push_back(new (getPool()) UnknownParameter(keyStart, 
                                                                 int((keyEnd - keyStart)), pb, terminators2));
            }
            else
            {
               // invoke the particular factory
               mParameters.push_back(p);
            }
         }
      }
      else
      {
         pb.reset(start);
         return;
      }
   }
}      

Parameter* 
ParserCategory::createParam(ParameterTypes::Type type, ParseBuffer& pb, const std::bitset<256>& terminators, PoolBase* pool)
{
   return 0;
}

static Data up_Msgr("msgr");

EncodeStream&
ParserCategory::encodeParameters(EncodeStream& str) const
{
    
   for (ParameterList::const_iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
#if 0
      // !cj! - may be wrong just hacking 
      // The goal of all this is not to add a tag if the tag is empty 
      ParameterTypes::Type type = (*it)->getType();
      
      if ( type ==  ParameterTypes::tag )
      {
         Parameter* p = (*it);
         DataParameter* d = dynamic_cast<DataParameter*>(p);
         
         Data& data = d->value();
         
         if ( !data.empty() )
         {
            str << Symbols::SEMI_COLON;
            // !ah! this is a TOTAL hack to work around an MSN bug that
            // !ah! requires a SPACE after the SEMI following the MIME type.
            if (it == mParameters.begin() && getParameterByData(up_Msgr))
            {
               str << Symbols::SPACE;
            }

            (*it)->encode(str);
         }
      }
      else
      {
         str << Symbols::SEMI_COLON;
         // !ah! this is a TOTAL hack to work around an MSN bug that
         // !ah! requires a SPACE after the SEMI following the MIME type.
         if (it == mParameters.begin() && getParameterByData(up_Msgr))
         {
            str << Symbols::SPACE;
         }

         (*it)->encode(str);
      }
      
#else
      str << Symbols::SEMI_COLON;
      // !ah! this is a TOTAL hack to work around an MSN bug that
      // !ah! requires a SPACE after the SEMI following the MIME type.
      if (it == mParameters.begin() && getParameterByData(up_Msgr))
      {
         str << Symbols::SPACE;
      }
      
      (*it)->encode(str);
#endif
   }
   for (ParameterList::const_iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      (*it)->encode(str);
   }
   return str;
}

EncodeStream&
resip::operator<<(EncodeStream& stream, const ParserCategory& category)
{
   category.checkParsed();
   return category.encode(stream);
}

Parameter* 
ParserCategory::getParameterByEnum(ParameterTypes::Type type) const
{
   for (ParameterList::const_iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == type)
      {
         return *it;
      }
   }
   return 0;
}

void
ParserCategory::setParameter(const Parameter* parameter)
{
   resip_assert(parameter);

   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == parameter->getType())
      {
         freeParameter(*it);
         mParameters.erase(it);
         mParameters.push_back(parameter->clone());
         return;
      }
   }

   // !dlb! kinda hacky -- what is the correct semantics here?
   // should be quietly add, quietly do nothing, throw?
   mParameters.push_back(parameter->clone());
}

void 
ParserCategory::removeParameterByEnum(ParameterTypes::Type type)
{
   // remove all instances
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end();)
   {
      if ((*it)->getType() == type)
      {
         freeParameter(*it);
         it = mParameters.erase(it);
      }
      else
      {
         ++it;
      }
   }
 }

Parameter* 
ParserCategory::getParameterByData(const Data& data) const
{
   for (ParameterList::const_iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if (isEqualNoCase((*it)->getName(), data))
      {
         return *it;
      }
   }
   return 0;
}

void 
ParserCategory::removeParameterByData(const Data& data)
{
   // remove all instances
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end();)
   {
      if ((*it)->getName() == data)
      {
         freeParameter(*it);
         it = mUnknownParameters.erase(it);
      }
      else
      {
         ++it;
      }
   }
}

Data
ParserCategory::commutativeParameterHash() const
{
   Data buffer;
   Data working;

   for (ParameterList::const_iterator i = mParameters.begin(); i != mParameters.end(); ++i)
   {
      if ((*i)->getType() != ParameterTypes::lr)
      {
         buffer.clear();
         {
            DataStream strm(buffer);
            (*i)->encode(strm);
         }
         working ^= buffer;
      }
   }

   buffer.clear();
   for (ParameterList::const_iterator i = mUnknownParameters.begin(); i != mUnknownParameters.end(); ++i)
   {
      UnknownParameter* p = static_cast<UnknownParameter*>(*i);
      buffer = p->getName();
      buffer += p->value();
      working ^= buffer;
   }
   
   return working;
}

const Data&
ParserCategory::errorContext() const
{
   return Headers::getHeaderName(mHeaderType);
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

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
