#include "sip2/sipstack/MultipartMixedContents.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"
#include "sip2/sipstack/EncodingContext.hxx"
#include "sip2/util/Random.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<MultipartMixedContents> MultipartMixedContents::Factory;

MultipartMixedContents::MultipartMixedContents()
   : Contents(getStaticType()),
     mContents()
{}

MultipartMixedContents::MultipartMixedContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : Contents(hfv, contentsType),
     mContents()
{
}
 
MultipartMixedContents::MultipartMixedContents(const MultipartMixedContents& rhs)
   : Contents(rhs),
     mContents()
{
   for (list<Contents*>::iterator i = mContents.begin(); 
        i != mContents.end(); i++)
   {
      mContents.push_back((*i)->clone());
   }
}

void
MultipartMixedContents::clear()
{
   Contents::clear();
   for (list<Contents*>::iterator i = mContents.begin(); 
        i != mContents.end(); i++)
   {
      delete *i;
   }
}

MultipartMixedContents::~MultipartMixedContents()
{}

MultipartMixedContents&
MultipartMixedContents::operator=(const MultipartMixedContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      for (list<Contents*>::iterator i = mContents.begin(); 
           i != mContents.end(); i++)
      {
         mContents.push_back((*i)->clone());
      }
   }
   return *this;
}

Contents* 
MultipartMixedContents::clone() const
{
   return new MultipartMixedContents(*this);
}

const Mime& 
MultipartMixedContents::getStaticType() const
{
   static Mime type("multipart","mixed");
   return type;
}

std::ostream& 
MultipartMixedContents::encodeParsed(std::ostream& str) const
{
   DebugLog(<< "MultipartMixedContents::encodeParsed ");

   Data boundaryToken = Random::getRandomHex(50);
   Data boundary(boundaryToken.size() + 4, true);
   boundary = Symbols::CRLF;
   boundary += Symbols::DASHDASH;
   boundary += boundaryToken;

   mContentsType.param("boundary") = boundaryToken;

   // !dlb! output headers
   str << Symbols::CRLF;

   for (list<Contents*>::const_iterator i = mContents.begin(); 
        i != mContents.end(); i++)
   {
      str << boundary << Symbols::CRLF;
      (*i)->encode(str);
   }
   str << boundary << Symbols::DASHDASH;
   return str;
}

void 
MultipartMixedContents::parse(ParseBuffer& pb)
{
   // determine the boundary
   const Data& boundaryToken = mContentsType.param("boundary");
   Data boundary(boundaryToken.size() + 2, true);
   boundary = Symbols::DASHDASH;
   boundary += boundaryToken;

   pb.skipToChars(boundary);
   do
   {
      // skip over boudary
      pb.skipN(boundary.size());

      if (*pb.position() == Symbols::DASH[0])
      {
         pb.skipChar();
         if (*pb.position() != Symbols::DASH[0])
         {
            // not really a boundary
            continue;
         }
         // terminating boundary
         pb.skipToEnd();
         return;
      }
      else if (*pb.position() == Symbols::CR[0])
      {
         pb.skipChar();
         if (*pb.position() != Symbols::LF[0])
         {
            // not really a boundary
            continue;
         }
      }

      // parse as MimeWrapper to hold mime headers, dispatches to actual
      // contents parser
      // !dlb! general header parsing
      pb.skipToChars("Content-Type");
      pb.skipToOneOf(Symbols::COLON, ParseBuffer::Whitespace);
      pb.skipChar();

      Mime contentType;
      contentType.parse(pb);

      // !dlb! skipped other headers
      pb.skipToChars(Symbols::CRLFCRLF);
      const char* anchor = pb.skipN(4);
      pb.skipToChars(boundary);
      
      Data tmp;
      pb.data(tmp, anchor);
      mContents.push_back(createContents(contentType, tmp));
   }
   while (!pb.eof());
}
