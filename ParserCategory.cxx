#include "sip2/sipstack/ParserCategory.hxx"
#include "sip2/sipstack/HeaderFieldValue.hxx"
#include "sip2/sipstack/UnknownParameter.hxx"
#include "sip2/util/compat.hxx"
#include "sip2/util/ParseBuffer.hxx"
#include <iostream>
#include <cassert>

// !ah! just for debugging
#include "sip2/util/Logger.hxx"
#define VOCAL_SUBSYSTEM Subsystem::SIP

using namespace Vocal2;
using namespace std;

ParserCategory::ParserCategory(HeaderFieldValue* headerFieldValue)
    : mHeaderField(headerFieldValue),
      mParameters(),
      mUnknownParameters(),
      mMine(false),
      mIsParsed(headerFieldValue->mField == 0)
{
}

ParserCategory::ParserCategory()
   : mHeaderField(0),
     mMine(true),
     mIsParsed(true)
{
}

ParserCategory::ParserCategory(const ParserCategory& rhs)
   : mHeaderField(0),
     mMine(true),
     mIsParsed(rhs.mIsParsed)
{
   if (mIsParsed)
   {
      copyParametersFrom(rhs);
   }
   else if (rhs.mHeaderField)
   {
      mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
   }
}

ParserCategory&
ParserCategory::operator=(const ParserCategory& rhs)
{
   if (this != &rhs)
   {
      clear();
      mIsParsed = rhs.mIsParsed;
      if (rhs.mIsParsed)
      {
         copyParametersFrom(rhs);
         mHeaderField = 0;
         mMine = false;
      }
      else
      {
         mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
         mMine = true;
      }
   }
   return *this;
}

void
ParserCategory::clear()
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      delete *it;
   }
   mParameters.clear();
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      delete *it;
   }   
   mUnknownParameters.clear();
   if (mMine)
   {
      delete mHeaderField;
      mHeaderField = 0;
   }
}

void 
ParserCategory::copyParametersFrom(const ParserCategory& other)
{
   for (ParameterList::iterator it = other.mParameters.begin();
        it != other.mParameters.end(); it++)
   {
      mParameters.push_back((*it)->clone());
   }
   for (ParameterList::iterator it = other.mUnknownParameters.begin();
        it != other.mUnknownParameters.end(); it++)
   {
      mUnknownParameters.push_back((*it)->clone());
   }
}

ParserCategory::~ParserCategory()
{
   clear();
}

void
ParserCategory::checkParsed() const
{
   if (!mIsParsed)
   {
      ParserCategory* ncThis = const_cast<ParserCategory*>(this);
      ncThis->mIsParsed = true;
      ParseBuffer pb(mHeaderField->mField, mHeaderField->mFieldLength);
      ncThis->parse(pb);
   }
}

ostream&
ParserCategory::encodeFromHeaderFieldValue(ostream& str) const
{
   assert(mHeaderField);
   mHeaderField->encode(str);
   return str;
}

// !dlb! need to convert existing parameter by enum to UnknownParameter for backward compatibility
Data&
ParserCategory::param(const Data& param) const
{
   checkParsed();
   Parameter* p = getParameterByData(param);
   if(!p)
   {
      p = new UnknownParameter(param);
      mUnknownParameters.push_back(p);
   }
   return dynamic_cast<UnknownParameter*>(p)->value();
}

bool
ParserCategory::exists(const ParamBase& paramType) const
{
    checkParsed();
    bool ret = getParameterByEnum(paramType.getTypeNum()) != NULL;
    return ret;
}

// removing non-present parameter is allowed      
void
ParserCategory::remove(const ParamBase& paramType)
{
    checkParsed();
    removeParameterByEnum(paramType.getTypeNum());
}

void 
ParserCategory::remove(const Data& param)
{
   checkParsed();
   removeParameterByData(param);   
}

bool 
ParserCategory::exists(const Data& param) const
{
   checkParsed();
   bool ret = ( getParameterByData(param) != NULL );
   return ret;
}

void
ParserCategory::parseParameters(ParseBuffer& pb)
{
   pb.skipWhitespace();
   while (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0])
   {
      // extract the key
      pb.skipChar();
      const char* keyStart = pb.skipWhitespace();
      const char* keyEnd = pb.skipToOneOf(" \t\r\n;=?>");  //!dlb! @ here?
      ParameterTypes::Type type = ParameterTypes::getType(keyStart, (keyEnd - keyStart));
      if (type == ParameterTypes::UNKNOWN)
      {
         mUnknownParameters.push_back(new UnknownParameter(keyStart, int((keyEnd - keyStart)), pb));
      }
      else
      {
         // invoke the particular factory
         mParameters.push_back(ParameterTypes::ParameterFactories[type](type, pb));
      }
      pb.skipWhitespace();
   }
}      

ostream&
ParserCategory::encodeParameters(ostream& str) const
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      (*it)->encode(str);
   }
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      (*it)->encode(str);
   }
   return str;
}

ostream&
Vocal2::operator<<(ostream& stream, const ParserCategory& category)
{
   category.checkParsed();
   return category.encode(stream);
}

Parameter* 
ParserCategory::getParameterByEnum(int type) const
{
   for (ParameterList::iterator it = mParameters.begin();
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
ParserCategory::removeParameterByEnum(int type)
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == type)
      {
         delete *it;
         mParameters.erase(it);
         return;
      }
   }
}

Parameter* 
ParserCategory::getParameterByData(const Data& data) const
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
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
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if ((*it)->getName() == data)
      {
         delete *it;
         mParameters.erase(it);
         return;
      }
   }
}

Transport_Param::DType& 
ParserCategory::param(const Transport_Param& paramType) const
{
   checkParsed();
   Transport_Param::Type* p = dynamic_cast<Transport_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Transport_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

User_Param::DType& 
ParserCategory::param(const User_Param& paramType) const
{
   checkParsed();
   User_Param::Type* p = dynamic_cast<User_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new User_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Method_Param::DType& 
ParserCategory::param(const Method_Param& paramType) const
{
   checkParsed();
   Method_Param::Type* p = dynamic_cast<Method_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Method_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Ttl_Param::DType& 
ParserCategory::param(const Ttl_Param& paramType) const
{
   checkParsed();
   Ttl_Param::Type* p = dynamic_cast<Ttl_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Ttl_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Maddr_Param::DType& 
ParserCategory::param(const Maddr_Param& paramType) const
{
   checkParsed();
   Maddr_Param::Type* p = dynamic_cast<Maddr_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Maddr_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Lr_Param::DType& 
ParserCategory::param(const Lr_Param& paramType) const
{
   checkParsed();
   Lr_Param::Type* p = dynamic_cast<Lr_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Lr_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Q_Param::DType& 
ParserCategory::param(const Q_Param& paramType) const
{
   checkParsed();
   Q_Param::Type* p = dynamic_cast<Q_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Q_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Purpose_Param::DType& 
ParserCategory::param(const Purpose_Param& paramType) const
{
   checkParsed();
   Purpose_Param::Type* p = dynamic_cast<Purpose_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Purpose_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Expires_Param::DType& 
ParserCategory::param(const Expires_Param& paramType) const
{
   checkParsed();
   Expires_Param::Type* p = dynamic_cast<Expires_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Expires_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Handling_Param::DType& 
ParserCategory::param(const Handling_Param& paramType) const
{
   checkParsed();
   Handling_Param::Type* p = dynamic_cast<Handling_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Handling_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Tag_Param::DType& 
ParserCategory::param(const Tag_Param& paramType) const
{
   checkParsed();
   Tag_Param::Type* p = dynamic_cast<Tag_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Tag_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

ToTag_Param::DType& 
ParserCategory::param(const ToTag_Param& paramType) const
{
   checkParsed();
   ToTag_Param::Type* p = dynamic_cast<ToTag_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ToTag_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

FromTag_Param::DType& 
ParserCategory::param(const FromTag_Param& paramType) const
{
   checkParsed();
   FromTag_Param::Type* p = dynamic_cast<FromTag_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new FromTag_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Duration_Param::DType& 
ParserCategory::param(const Duration_Param& paramType) const
{
   checkParsed();
   Duration_Param::Type* p = dynamic_cast<Duration_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Duration_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Branch_Param::DType& 
ParserCategory::param(const Branch_Param& paramType) const
{
   checkParsed();
   Branch_Param::Type* p = dynamic_cast<Branch_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Branch_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return *p;
}

Received_Param::DType& 
ParserCategory::param(const Received_Param& paramType) const
{
   checkParsed();
   Received_Param::Type* p = dynamic_cast<Received_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Received_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Mobility_Param::DType& 
ParserCategory::param(const Mobility_Param& paramType) const
{
   checkParsed();
   Mobility_Param::Type* p = dynamic_cast<Mobility_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Mobility_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Comp_Param::DType& 
ParserCategory::param(const Comp_Param& paramType) const
{
   checkParsed();
   Comp_Param::Type* p = dynamic_cast<Comp_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Comp_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Rport_Param::DType& 
ParserCategory::param(const Rport_Param& paramType) const
{
   checkParsed();
   Rport_Param::Type* p = dynamic_cast<Rport_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Rport_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
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
