#ifndef Contents_hxx
#define Contents_hxx

#include "resiprocate/sipstack/LazyParser.hxx"
#include "resiprocate/sipstack/ParserCategories.hxx" // .dlb. Mime, Token, StringCategory
#include "resiprocate/sipstack/Headers.hxx"
#include "resiprocate/util/Data.hxx"
#include <map>
#include <iostream>

using namespace std;

namespace Vocal2
{

class Contents;
class HeaderFieldValue;
class ParseBuffer;

class ContentsFactoryBase
{
   public:
      virtual Contents* create(HeaderFieldValue* hfv, 
                               const Mime& contentType) const = 0;
      virtual Contents* convert(Contents* c) const = 0;
};

// MIME header types
class MIME_Header
{
};

class MIME_Id_Header : public MIME_Header
{
   public:
      typedef Token Type;
};
extern MIME_Id_Header h_Id;

class MIME_Description_Header : public MIME_Header
{
   public:
      typedef StringCategory Type;
};
extern MIME_Description_Header h_Description;
      
// Common type for all body contents
class Contents : public LazyParser
{
   public:
      // pass Mime instance for parameters

      Contents(HeaderFieldValue* headerFieldValue, const Mime& contentsType);
      Contents(const Mime& contentsType);
      Contents(const Contents& rhs);
      virtual ~Contents();
      Contents& operator=(const Contents& rhs);

      void preParseHeaders(ParseBuffer& pb);
      std::ostream& encodeHeaders(std::ostream& str) const;

      virtual Contents* getContents() {return this;}
      Contents* getContents(const Mime&);

      virtual Data getBodyData() const;
      
      virtual Contents* clone() const = 0;
      const Mime& getType() const {return mType;}

      static Contents* createContents(const Mime& contentType, 
                                      const Data& contents);

      bool exists(const HeaderBase& headerType) const;
      void remove(const HeaderBase& headerType);

      bool exists(const MIME_Header& headerType) const;
      void remove(const MIME_Header& headerType);

      // shared header types
      ContentType_Header::Type& header(const ContentType_Header& headerType) const;
      ContentDisposition_Header::Type& header(const ContentDisposition_Header& headerType) const;
      ContentTransferEncoding_Header::Type& header(const ContentTransferEncoding_Header& headerType) const;
      ParserContainer<ContentLanguage_MultiHeader::Type>& header(const ContentLanguage_MultiHeader& headerType) const;

      // MIME specific header types
      MIME_Id_Header::Type& header(const MIME_Id_Header& headerType) const;
      MIME_Description_Header::Type& header(const MIME_Description_Header& headerType) const;

      int& verion() {return mVersion;}
      int& minorVersion() {return mMinorVersion;}

      static std::map<Mime, ContentsFactoryBase*>& getFactoryMap();

   protected:
      void clear();
      virtual const Data& errorContext() const;

      // MIME version? version & minorVersion as ints?
      mutable Mime mType;
      mutable ContentDisposition_Header::Type *mDisposition;
      mutable StringCategory *mTransferEncoding;      
      mutable ParserContainer<ContentLanguage_MultiHeader::Type> *mLanguages;
      mutable Token *mId;
      mutable StringCategory *mDescription;

      mutable int mVersion;
      mutable int mMinorVersion;

   private:
      static std::map<Mime, ContentsFactoryBase*>* FactoryMap;
};

template<class T>
class ContentsFactory : public ContentsFactoryBase
{
   public:
      ContentsFactory()
      {
         HeaderFieldValue hfv;
         //T tmp;
		 Contents::getFactoryMap()[T::getStaticType()] = this;
      }
      
      // pass Mime instance for parameters
      virtual Contents* create(HeaderFieldValue* hfv, const Mime& contentType) const
      {
         return new T(hfv, contentType);
      }

      virtual Contents* convert(Contents* c) const
      {
         return dynamic_cast<T*>(c);
      }
};

}

#endif

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
 *    notice, this std::list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this std::list of conditions and the following disclaimer in
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
