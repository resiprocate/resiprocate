#include <vector>

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#include "resip/stack/Contents.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/Logger.hxx"
#include "resip/stack/OctetContents.hxx"
#include "rutil/MD5Stream.hxx"
#include "rutil/WinLeakCheck.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

H_ContentID resip::h_ContentID;
H_ContentDescription resip::h_ContentDescription;

Contents::Contents(const HeaderFieldValue& headerFieldValue,
                   const Mime& contentType) 
   : LazyParser(headerFieldValue),
      mType(contentType)
{
   init();
}

Contents::Contents(const Mime& contentType) 
   : mType(contentType)
{
   init();
}

Contents::Contents(const Contents& rhs) 
    : LazyParser(rhs)
{
   init(rhs);
}

Contents::Contents(const Contents& rhs,HeaderFieldValue::CopyPaddingEnum e) 
    : LazyParser(rhs,e)
{
   init(rhs);
}

Contents::Contents(const HeaderFieldValue& headerFieldValue,
                     HeaderFieldValue::CopyPaddingEnum e,
                     const Mime& contentsType)
    : LazyParser(headerFieldValue,e),
    mType(contentsType)
{
   init();
}


Contents::~Contents()
{
   freeMem();
}

static const Data errorContextData("Contents");
const Data&
Contents::errorContext() const
{
   return errorContextData;
}

Contents& 
Contents::operator=(const Contents& rhs) 
{
   if (this != &rhs)
   {
      freeMem();
      LazyParser::operator=(rhs);
      init(rhs);
   }

   return *this;
}

void
Contents::init(const Contents& orig)
{
   mBufferList.clear();
   mType = orig.mType;
   if (orig.mDisposition)
   {
       mDisposition = new H_ContentDisposition::Type(*orig.mDisposition);
   }
   else
   {
      mDisposition = 0;
   }
   
   if (orig.mTransferEncoding)
   {
       mTransferEncoding = new H_ContentTransferEncoding::Type(*orig.mTransferEncoding);
   }
   else
   {
      mTransferEncoding = 0;
   }
   
   if (orig.mLanguages)
   {
       mLanguages = new H_ContentLanguages::Type(*orig.mLanguages);
   }
   else
   {
      mLanguages = 0;
   }
   
   if (orig.mId)
   {
       mId = new Token(*orig.mId);
   }
   else
   {
      mId = 0;
   }
   
   if (orig.mDescription)
   {
       mDescription = new StringCategory(*orig.mDescription);
   }
   else
   {
      mDescription = 0;
   }
   
   if(orig.mLength)
   {
      mLength = new StringCategory(*orig.mLength);
   }
   else
   {
      mLength = 0;
   }

   mVersion = orig.mVersion;
   mMinorVersion = orig.mMinorVersion;

}

Contents*
Contents::createContents(const Mime& contentType, 
                         const Data& contents)
{
  // !ass! why are we asserting that the Data doesn't own the buffer?
  // .dlb. because this method is to be called only within a multipart
  // !ass! HFV is an overlay -- then setting c->mIsMine to true ?? dlb Q
  // .dlb. we are telling the content that it owns its HFV, not the data that it
  // .dlb. owns its memory
  // .bwc. So, we are _violating_ _encapsulation_, to make an assertion that the 
  // Data is an intermediate instead of owning the buffer itself? What do we 
  // care how this buffer is owned? All we require is that the buffer doesn't 
  // get dealloced/modified while we're still around. The assertion might mean 
  // that the Data will not do either of these things, but it affords no 
  // protection from the actual owner doing so. We are no more protected with 
  // the assertion, so I am removing it.
//   assert(!contents.mMine);

   HeaderFieldValue hfv(contents.data(), (unsigned int)contents.size());

// !bwc! This padding stuff is now the responsibility of the Contents class.
//   if(contentType.subType()=="sipfrag"||contentType.subType()=="external-body")
//   {
//      // .bwc. The parser for sipfrag requires padding at the end of the hfv.
//      HeaderFieldValue* temp = hfv;
//      hfv = new HeaderFieldValue(*temp,HeaderFieldValue::CopyPadding);
//      delete temp;
//   }
   
   Contents* c;
   if (ContentsFactoryBase::getFactoryMap().find(contentType) != ContentsFactoryBase::getFactoryMap().end())
   {
      c = ContentsFactoryBase::getFactoryMap()[contentType]->create(hfv, contentType);
   }
   else
   {
      c = new OctetContents(hfv, contentType);
   }
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
   if (&type == &h_ContentID)
   {
      return mId != 0;
   }
   
   if (&type == &h_ContentDescription)
   {
      return mDescription != 0;
   }

   resip_assert(false);
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
   if (&type == &h_ContentID)
   {
      delete mId;
      mId = 0;
      return;
   }
    
   if (&type == &h_ContentDescription)
   {
      delete mDescription;
      mDescription = 0;
      return;
   }

   resip_assert(false);
}

const H_ContentType::Type&
Contents::header(const H_ContentType& headerType) const
{
   return mType;
}

H_ContentType::Type&
Contents::header(const H_ContentType& headerType)
{
   return mType;
}

const H_ContentDisposition::Type&
Contents::header(const H_ContentDisposition& headerType) const
{
   checkParsed();
   if (mDisposition == 0)
   {
      ErrLog(<< "You called "
            "Contents::header(const H_ContentDisposition& headerType) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      Contents* ncthis = const_cast<Contents*>(this);
      ncthis->mDisposition = new H_ContentDisposition::Type;
   }
   return *mDisposition;
}

H_ContentDisposition::Type&
Contents::header(const H_ContentDisposition& headerType)
{
   checkParsed();
   if (mDisposition == 0)
   {
      mDisposition = new H_ContentDisposition::Type;
   }
   return *mDisposition;
}

const H_ContentTransferEncoding::Type&
Contents::header(const H_ContentTransferEncoding& headerType) const
{
   checkParsed();
   if (mTransferEncoding == 0)
   {
      ErrLog(<< "You called "
            "Contents::header(const H_ContentTransferEncoding& headerType) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      Contents* ncthis = const_cast<Contents*>(this);
      ncthis->mTransferEncoding = new H_ContentTransferEncoding::Type;
   }
   return *mTransferEncoding;
}

H_ContentTransferEncoding::Type&
Contents::header(const H_ContentTransferEncoding& headerType)
{
   checkParsed();
   if (mTransferEncoding == 0)
   {
      mTransferEncoding = new H_ContentTransferEncoding::Type;
   }
   return *mTransferEncoding;
}

const H_ContentLanguages::Type&
Contents::header(const H_ContentLanguages& headerType) const 
{
   checkParsed();
   if (mLanguages == 0)
   {
      ErrLog(<< "You called "
            "Contents::header(const H_ContentLanguages& headerType) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      Contents* ncthis = const_cast<Contents*>(this);
      ncthis->mLanguages = new H_ContentLanguages::Type;
   }
   return *mLanguages;
}

H_ContentLanguages::Type&
Contents::header(const H_ContentLanguages& headerType)
{
   checkParsed();
   if (mLanguages == 0)
   {
      mLanguages = new H_ContentLanguages::Type;
   }
   return *mLanguages;
}

const H_ContentDescription::Type&
Contents::header(const H_ContentDescription& headerType) const
{
   checkParsed();
   if (mDescription == 0)
   {
      ErrLog(<< "You called "
            "Contents::header(const H_ContentDescription& headerType) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      Contents* ncthis = const_cast<Contents*>(this);
      ncthis->mDescription = new H_ContentDescription::Type;
   }
   return *mDescription;
}

H_ContentDescription::Type&
Contents::header(const H_ContentDescription& headerType)
{
   checkParsed();
   if (mDescription == 0)
   {
      mDescription = new H_ContentDescription::Type;
   }
   return *mDescription;
}

const H_ContentID::Type&
Contents::header(const H_ContentID& headerType) const
{
   checkParsed();
   if (mId == 0)
   {
      ErrLog(<< "You called "
            "Contents::header(const H_ContentID& headerType) _const_ "
            "without first calling exists(), and the header does not exist. Our"
            " behavior in this scenario is to implicitly create the header(using const_cast!); "
            "this is probably not what you want, but it is either this or "
            "assert/throw an exception. Since this has been the behavior for "
            "so long, we are not throwing here, _yet_. You need to fix your "
            "code, before we _do_ start throwing. This is why const-correctness"
            " should never be made a TODO item </rant>");
      Contents* ncthis = const_cast<Contents*>(this);
      ncthis->mId = new H_ContentID::Type;
   }
   return *mId;
}

H_ContentID::Type&
Contents::header(const H_ContentID& headerType)
{
   checkParsed();
   if (mId == 0)
   {
      mId = new H_ContentID::Type;
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

   try
   {
      
   while (!pb.eof())
   {
      const char* anchor = pb.skipWhitespace();
      pb.skipToOneOf(Symbols::COLON, ParseBuffer::Whitespace);
      pb.data(headerName, anchor);

      pb.skipWhitespace();
      pb.skipChar(Symbols::COLON[0]);
      anchor = pb.skipWhitespace();
      pb.skipToTermCRLF();

      Headers::Type type = Headers::getType(headerName.data(), (int)headerName.size());
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
            mDisposition = new H_ContentDisposition::Type;
            mDisposition->parse(subPb);
            break;
         }
         case Headers::ContentTransferEncoding :
         {
            mTransferEncoding = new H_ContentTransferEncoding::Type;
            mTransferEncoding->parse(subPb);
            break;
         }
         // !dlb! not sure this ever happens?
         case Headers::ContentLanguage :
         {
            if (mLanguages == 0)
            {
               mLanguages = new H_ContentLanguages::Type;
            }

            subPb.skipWhitespace();
            while (!subPb.eof() && *subPb.position() != Symbols::COMMA[0])
            {
               H_ContentLanguages::Type::value_type tmp;
               header(h_ContentLanguages).push_back(tmp);
               header(h_ContentLanguages).back().parse(subPb);
               subPb.skipLWS();
            }
            break;	// .kw. added -- this is needed, right?
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
            // Some people put this in ...
            else if (isEqualNoCase(headerName, "Content-Length"))
            {
               mLength = new StringCategory();
               mLength->parse(subPb);
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
               std::cerr << "Unknown MIME Content- header: " << headerName << std::endl;
               ErrLog(<< "Unknown MIME Content- header: " << headerName);
               resip_assert(false);
            }
         }
      }
   }
   }
   catch (ParseException &  e )
   {
      ErrLog( << "Some problem parsing contents: " << e );
      throw e;
   }
}

EncodeStream&
Contents::encodeHeaders(EncodeStream& str) const
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

      size_t count = 0;
      size_t size = header(h_ContentLanguages).size();

      for (H_ContentLanguages::Type::const_iterator 
              i = header(h_ContentLanguages).begin();
           i != header(h_ContentLanguages).end(); ++i)
      {
         i->encode(str);

         if (++count < size)
             str << Symbols::COMMA << Symbols::SPACE;
      }
      str << Symbols::CRLF;
   }

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

   if (mLength)
   {
      str << "Content-Length" << Symbols::COLON[0] << Symbols::SPACE[0]
          <<  *mLength 
          << Symbols::CRLF;
   }
   
   str << Symbols::CRLF;
   return str;
}

Data
Contents::getBodyData() const 
{
   checkParsed();
   return Data::from(*this);
}

void
Contents::addBuffer(char* buf)
{
   mBufferList.push_back(buf);
}

bool
resip::operator==(const Contents& lhs, const Contents& rhs)
{
   MD5Stream lhsStream;
   lhsStream << lhs;
   MD5Stream rhsStream;
   rhsStream << rhs;
   return lhsStream.getHex() == rhsStream.getHex();
}

bool
resip::operator!=(const Contents& lhs, const Contents& rhs)
{
   return !operator==(lhs,rhs);
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005
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
