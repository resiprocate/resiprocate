#include "sip2/sipstack/Contents.hxx"
#include "sip2/util/ParseBuffer.hxx"

using namespace Vocal2;

std::map<Mime, ContentsFactoryBase*>* Contents::FactoryMap = 0;

Contents::Contents(HeaderFieldValue* headerFieldValue,
                   const Mime& contentType) 
   : LazyParser(headerFieldValue),
     mContentsType(contentType),
     mDisposition(0),
     mEncoding(0),
     mLanguages(0)
{}

Contents::Contents(const Mime& contentType) 
   : mContentsType(contentType),
     mDisposition(0),
     mEncoding(0),
     mLanguages(0)
{}

Contents::Contents(const Contents& rhs) 
    : LazyParser(rhs),
      mContentsType(rhs.mContentsType),
      mDisposition(0),
      mEncoding(0),
      mLanguages(0)
{
   if (rhs.mDisposition)
   {
      mDisposition = new Content_Disposition_Header::Type;
      *mDisposition = *rhs.mDisposition;
   }

   if (rhs.mEncoding)
   {
      mEncoding = new Content_Encoding_Header::Type;
      *mEncoding = *rhs.mEncoding;
   }

   if (rhs.mLanguages)
   {
      mLanguages = new ParserContainer<Content_Language_MultiHeader::Type>;
      *mLanguages = *rhs.mLanguages;
   }
}
  
Contents::~Contents()
{
   delete mDisposition;
   delete mEncoding;
   delete mLanguages;
}

Contents& 
Contents::operator=(const Contents& rhs) 
{
   if (this != &rhs)
   {
      LazyParser::operator=(rhs); 
      mContentsType = rhs.mContentsType;
      
      delete mDisposition;
      delete mEncoding;
      delete mLanguages;
      
      mDisposition = new Content_Disposition_Header::Type(*rhs.mDisposition);
      mEncoding = new Content_Encoding_Header::Type(*rhs.mEncoding);
      mLanguages = new ParserContainer<Content_Language_MultiHeader::Type>(*rhs.mLanguages);
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
   assert(Contents::getFactoryMap().find(m) != Contents::getFactoryMap().end());
   return Contents::getFactoryMap()[m]->convert(getContents());
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
   switch (headerType.getTypeNum())
   {
      case Headers::Content_Type :
      {
         return true;
      }
      case Headers::Content_Disposition :
      {
         return mDisposition != 0;
      }
      case Headers::Content_Encoding :
      {
         return mEncoding != 0;
      }
      case Headers::Content_Language :
      {
         return mLanguages != 0;
      }
      default : return false;
   }
}

void
Contents::remove(const HeaderBase& headerType)
{
   switch (headerType.getTypeNum())
   {
      case Headers::Content_Disposition :
      {
         delete mDisposition;
         mDisposition = 0;
         break;
      }
      case Headers::Content_Encoding :
      {
         delete mEncoding;
         mEncoding = 0;
         break;
      }
      case Headers::Content_Language :
      {
         delete mLanguages;
         mLanguages = 0;
         break;
      }
      default :
         ;
   }
}

Content_Type_Header::Type&
Contents::header(const Content_Type_Header& headerType) const
{
   return mContentsType;
}

Content_Disposition_Header::Type&
Contents::header(const Content_Disposition_Header& headerType) const
{
   if (mDisposition == 0)
   {
      mDisposition = new Content_Disposition_Header::Type;
   }
   return *mDisposition;
}

Content_Encoding_Header::Type&
Contents::header(const Content_Encoding_Header& headerType) const
{
   if (mEncoding == 0)
   {
      mEncoding = new Content_Encoding_Header::Type;
   }
   return *mEncoding;
}

ParserContainer<Content_Language_MultiHeader::Type>&
Contents::header(const Content_Language_MultiHeader& headerType) const 
{
   if (mLanguages == 0)
   {
      mLanguages = new ParserContainer<Content_Language_MultiHeader::Type>;
   }
   return *mLanguages;
}

void
Contents::parseHeaders(ParseBuffer& pb)
{
   // check if headers already set
   if (exists(h_ContentDisposition) ||
       exists(h_ContentEncoding) ||
       exists(h_ContentLanguages))
   {
      return;
   }

   Data headerName;
   while (!pb.eof() &&
          !(*pb.position() == Symbols::CR[0] &&
            *(pb.position()+1) == Symbols::LF[0]))
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
         case Headers::Content_Disposition :
         {
            mDisposition = new Content_Disposition_Header::Type;
            mDisposition->parse(subPb);
            break;
         }
         case Headers::Content_Encoding :
         {
            mEncoding = new Content_Encoding_Header::Type;
            mEncoding->parse(subPb);
            break;
         }
         case Headers::Content_Language :
         {
            if (mLanguages == 0)
            {
               mLanguages = new ParserContainer<Content_Language_MultiHeader::Type>;
            }

            subPb.skipWhitespace();
            while (*subPb.position() != Symbols::COMMA[0])
            {
               Content_Language_MultiHeader::Type tmp;
               header(h_ContentLanguages).push_back(tmp);
               header(h_ContentLanguages).back().parse(subPb);
               subPb.skipLWS();
            }
         }
         default :
         {
            // add to application headers someday
            assert(false);
         }
      }
      pb.skipChars(Symbols::CRLF);
   }
}

ostream&
Contents::encodeHeaders(ostream& str) const
{
   if (exists(h_ContentDisposition))
   {
      header(h_ContentDisposition).encode(str);
      str << Symbols::CRLF;
   }

   if (exists(h_ContentEncoding))
   {
      header(h_ContentEncoding).encode(str);
      str << Symbols::CRLF;
   }

   if (exists(h_ContentLanguages))
   {
      for (ParserContainer<Content_Language_MultiHeader::Type>::iterator i = header(h_ContentLanguages).begin();
           i != header(h_ContentLanguages).end(); i++)
      {
         i->encode(str);
         str << Symbols::CRLF;
      }
   }

   str << Symbols::CRLF;
   return str;
}
