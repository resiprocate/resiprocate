#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif

#include "resiprocate/Contents.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/GenericContents.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

MIME_Id_Header resip::h_Id;
MIME_Description_Header resip::h_Description;

std::map<Mime, ContentsFactoryBase*>* Contents::FactoryMap = 0;

Contents::Contents(HeaderFieldValue* headerFieldValue,
                   const Mime& contentType) 
   : LazyParser(headerFieldValue),
     mType(contentType),
     mDisposition(0),
     mTransferEncoding(0),
     mLanguages(0),
     mId(0),
     mDescription(0),
     mVersion(1),
     mMinorVersion(0)
{}

Contents::Contents(const Mime& contentType) 
   : mType(contentType),
     mDisposition(0),
     mTransferEncoding(0),
     mLanguages(0),
     mId(0),
     mDescription(0),
     mVersion(1),
     mMinorVersion(0)
{}

Contents::Contents(const Contents& rhs) 
    : LazyParser(rhs),
      mType(rhs.mType),
      mDisposition(0),
      mTransferEncoding(0),
      mLanguages(0),
      mId(0),
      mDescription(0),
      mVersion(1),
      mMinorVersion(0)
{
   *this = rhs;
}

Contents::~Contents()
{
   clear();
}

void
Contents::clear()
{
   delete mDisposition;
   delete mTransferEncoding;
   delete mLanguages;
   delete mId;
   delete mDescription;

   mDisposition = 0;
   mTransferEncoding = 0;
   mLanguages = 0;
   mId = 0;
   mDescription = 0;
}

const Data&
Contents::errorContext() const
{
   static const Data context("Contents");
   return context;
}

Contents& 
Contents::operator=(const Contents& rhs) 
{
   if (this != &rhs)
   {
      LazyParser::operator=(rhs); 
      mType = rhs.mType;
      if (rhs.mDisposition)
      {
         mDisposition = new ContentDisposition_Header::Type(*rhs.mDisposition);
      }
      if (rhs.mTransferEncoding)
      {
         mTransferEncoding = new StringCategory(*rhs.mTransferEncoding);
      }
      if (rhs.mLanguages)
      {
         mLanguages = new ParserContainer<ContentLanguage_MultiHeader::Type>(*rhs.mLanguages);
      }
      if (rhs.mId)
      {
         mId = new Token(*rhs.mId);
      }
      if (rhs.mDescription)
      {
         mDescription = new StringCategory(*rhs.mDescription);
      }

      mVersion = rhs.mVersion;
      mMinorVersion = rhs.mMinorVersion;
   }

   return *this;
}

std::map<Mime, ContentsFactoryBase*>& 
Contents::getFactoryMap()
{
   if (Contents::FactoryMap == 0)
   {
      Contents::FactoryMap = new std::map<Mime, ContentsFactoryBase*>();
   }
   return *Contents::FactoryMap;
}

Contents*
Contents::getContents(const Mime& m)
{
   if (Contents::getFactoryMap().find(m) != Contents::getFactoryMap().end())
   {
      return Contents::getFactoryMap()[m]->convert(getContents());
   }
   else
   {
      // return a generic contents and hope for the best
      return new GenericContents(mHeaderField, m);
   }
}

Contents*
Contents::createContents(const Mime& contentType, 
                         const Data& contents)
{
   assert(!contents.mMine);
   HeaderFieldValue *hfv = new HeaderFieldValue(contents.data(), contents.size());
   assert(Contents::getFactoryMap().find(contentType) != Contents::getFactoryMap().end());
   Contents* c = Contents::getFactoryMap()[contentType]->create(hfv, contentType);
   c->mIsMine = true;
   return c;
}

bool
Contents::exists(const HeaderBase& headerType) const
{
   checkParsed();
   switch (headerType.getTypeNum())
   {
      case Headers::ContentType :
      {
         return true;
      }
      case Headers::ContentDisposition :
      {
         return mDisposition != 0;
      }
      case Headers::ContentTransferEncoding :
      {
         return mTransferEncoding != 0;
      }
      case Headers::ContentLanguage :
      {
         return mLanguages != 0;
      }
      default : return false;
   }
}

bool
Contents::exists(const MIME_Header& type) const
{
   if (&type == &h_Id)
   {
      return mId != 0;
   }
   
   if (&type == &h_Description)
   {
      return mDescription != 0;
   }

   assert(false);
   return false;
}

void
Contents::remove(const HeaderBase& headerType)
{
   switch (headerType.getTypeNum())
   {
      case Headers::ContentDisposition :
      {
         delete mDisposition;
         mDisposition = 0;
         break;
      }
      case Headers::ContentLanguage :
      {
         delete mLanguages;
         mLanguages = 0;
         break;
      }
      case Headers::ContentTransferEncoding :
      {
         delete mTransferEncoding;
         mTransferEncoding = 0;
         break;
      }
      default :
         ;
   }
}

void
Contents::remove(const MIME_Header& type)
{
   if (&type == &h_Id)
   {
      delete mId;
      mId = 0;
      return;
   }
    
   if (&type == &h_Description)
   {
      delete mDescription;
      mDescription = 0;
      return;
   }

   assert(false);
}

ContentType_Header::Type&
Contents::header(const ContentType_Header& headerType) const
{
   return mType;
}

ContentDisposition_Header::Type&
Contents::header(const ContentDisposition_Header& headerType) const
{
   checkParsed();
   if (mDisposition == 0)
   {
      mDisposition = new ContentDisposition_Header::Type;
   }
   return *mDisposition;
}

ContentTransferEncoding_Header::Type&
Contents::header(const ContentTransferEncoding_Header& headerType) const
{
   checkParsed();
   if (mTransferEncoding == 0)
   {
      mTransferEncoding = new ContentTransferEncoding_Header::Type;
   }
   return *mTransferEncoding;
}

ParserContainer<ContentLanguage_MultiHeader::Type>&
Contents::header(const ContentLanguage_MultiHeader& headerType) const 
{
   checkParsed();
   if (mLanguages == 0)
   {
      mLanguages = new ParserContainer<ContentLanguage_MultiHeader::Type>;
   }
   return *mLanguages;
}

MIME_Description_Header::Type&
Contents::header(const MIME_Description_Header& headerType) const
{
   checkParsed();
   if (mDescription == 0)
   {
      mDescription = new MIME_Description_Header::Type;
   }
   return *mDescription;
}

MIME_Id_Header::Type&
Contents::header(const MIME_Id_Header& headerType) const
{
   checkParsed();
   if (mId == 0)
   {
      mId = new MIME_Id_Header::Type;
   }
   return *mId;
}

// !dlb! headers except Content-Disposition may contain (comments)
void
Contents::preParseHeaders(ParseBuffer& pb)
{
   const char* start = pb.position();
   Data all( start, pb.end()-start);

   Data headerName;

   while (!pb.eof())
   {
      const char* anchor = pb.skipWhitespace();
      pb.skipToOneOf(Symbols::COLON, ParseBuffer::Whitespace);
      pb.data(headerName, anchor);

      pb.skipWhitespace();
      pb.skipChar(Symbols::COLON[0]);
      anchor = pb.skipWhitespace();
      pb.skipToTermCRLF();

      Headers::Type type = Headers::getType(headerName.data(), headerName.size());
      ParseBuffer subPb(anchor, pb.position() - anchor);

      switch (type)
      {
         case Headers::ContentType :
         {
            // already set
            break;
         }
         case Headers::ContentDisposition :
         {
            mDisposition = new ContentDisposition_Header::Type;
            mDisposition->parse(subPb);
            break;
         }
         case Headers::ContentTransferEncoding :
         {
            mTransferEncoding = new ContentTransferEncoding_Header::Type;
            mTransferEncoding->parse(subPb);
            break;
         }
         // !dlb! not sure this ever happens?
         case Headers::ContentLanguage :
         {
            if (mLanguages == 0)
            {
               mLanguages = new ParserContainer<ContentLanguage_MultiHeader::Type>;
            }

            subPb.skipWhitespace();
            while (!subPb.eof() && *subPb.position() != Symbols::COMMA[0])
            {
               ContentLanguage_MultiHeader::Type tmp;
               header(h_ContentLanguages).push_back(tmp);
               header(h_ContentLanguages).back().parse(subPb);
               subPb.skipLWS();
            }
         }
         default :
         {
            if (isEqualNoCase(headerName, "Content-Transfer-Encoding"))
            {
               mTransferEncoding = new StringCategory();
               mTransferEncoding->parse(subPb);
            }
            else if (isEqualNoCase(headerName, "Content-Description"))
            {
               mDescription = new StringCategory();
               mDescription->parse(subPb);
            }
            else if (isEqualNoCase(headerName, "Content-Id"))
            {
               mId = new Token();
               mId->parse(subPb);
            }
            else if (isEqualNoCase(headerName, "MIME-Version"))
            {
               subPb.skipWhitespace();
               if (!subPb.eof() && *subPb.position() == Symbols::LPAREN[0])
               {
                  subPb.skipToEndQuote(Symbols::RPAREN[0]);
                  subPb.skipChar(Symbols::RPAREN[0]);
               }
               mVersion = subPb.integer();

               if (!subPb.eof() && *subPb.position() == Symbols::LPAREN[0])
               {
                  subPb.skipToEndQuote(Symbols::RPAREN[0]);
                  subPb.skipChar(Symbols::RPAREN[0]);
               }
               subPb.skipChar(Symbols::PERIOD[0]);
               
               if (!subPb.eof() && *subPb.position() == Symbols::LPAREN[0])
               {
                  subPb.skipToEndQuote(Symbols::RPAREN[0]);
                  subPb.skipChar(Symbols::RPAREN[0]);
               }
               
               mMinorVersion = subPb.integer();
            }
            else
            {
               // add to application headers someday
               cerr << "Unknown MIME Content- header: " << headerName << endl;
               ErrLog(<< "Unknown MIME Content- header: " << headerName);
               assert(false);
            }
         }
      }
   }
}

ostream&
Contents::encodeHeaders(ostream& str) const
{
   if (mVersion != 1 || mMinorVersion != 0)
   {
      str << "MIME-Version" << Symbols::COLON[0] << Symbols::SPACE[0]
          << mVersion << Symbols::PERIOD[0] << mMinorVersion 
          << Symbols::CRLF;
   }

   str << "Content-Type" << Symbols::COLON[0] << Symbols::SPACE[0]
       << mType 
       << Symbols::CRLF;

   if (exists(h_ContentDisposition))
   {
      str <<  "Content-Disposition" << Symbols::COLON[0] << Symbols::SPACE[0];

      header(h_ContentDisposition).encode(str);
      str << Symbols::CRLF;
   }

   if (exists(h_ContentLanguages))
   {
      str <<  "Content-Languages" << Symbols::COLON[0] << Symbols::SPACE[0];
      
      for (ParserContainer<ContentLanguage_MultiHeader::Type>::iterator 
              i = header(h_ContentLanguages).begin();
           i != header(h_ContentLanguages).end(); i++)
      {
         i->encode(str);
         str << Symbols::CRLF;
      }
   }

#if 0 // !cj! this look redudneatn with next thing but I don't know 
   if (exists(h_ContentTransferEncoding))
   {
      str <<  "Content-Transfer-Encoding" << Symbols::COLON[0] << Symbols::SPACE[0];

      header(h_ContentTransferEncoding).encode(str);
      str << Symbols::CRLF;
   }
#endif 

   if (mTransferEncoding)
   {
      str << "Content-Transfer-Encoding" << Symbols::COLON[0] << Symbols::SPACE[0]
          << *mTransferEncoding
          << Symbols::CRLF;
   }

   if (mId)
   {
      str << "Content-Id" << Symbols::COLON[0] << Symbols::SPACE[0]
          << *mId
          << Symbols::CRLF;
   }

   if (mDescription)
   {
      str << "Content-Description" << Symbols::COLON[0] << Symbols::SPACE[0]
          << *mDescription
          << Symbols::CRLF;
   }

   str << Symbols::CRLF;
   return str;
}


Data
Contents::getBodyData() const 
{
   ErrLog( << "Need to implement getBodyData function for " << getType() );
   assert(0);
   return Data::Empty;
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
