#include "sip2/sipstack/MultipartMixedContents.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"
//#include "sip2/sipstack/EncodingContext.hxx"
#include "sip2/util/Random.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<MultipartMixedContents> MultipartMixedContents::Factory;

MultipartMixedContents::MultipartMixedContents()
   : Contents(getStaticType()),
     mContents()
{
   setBoundary();
}

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
MultipartMixedContents::setBoundary()
{
   Data boundaryToken = Random::getRandomHex(50);
   mType.param("boundary") = boundaryToken;
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
   encodeHeaders(str);

   const Data& boundaryToken = mType.param("boundary");
   Data boundary(boundaryToken.size() + 6, true);
   boundary = Symbols::CRLF;
   boundary += "_=";
   boundary += Symbols::DASHDASH;
   boundary += boundaryToken;

   for (list<Contents*>::const_iterator i = mContents.begin(); 
        i != mContents.end(); i++)
   {
      str << boundary << Symbols::CRLF;
      (*i)->encode(str);
   }
   str << boundary << Symbols::DASHDASH;
   return str;
}

// The boundary delimiter MUST occur at the beginning of a line, i.e., following
// a CRLF, and the initial CRLF is considered to be attached to the boundary
// delimiter line rather than part of the preceding part.
void 
MultipartMixedContents::parse(ParseBuffer& pb)
{
   DebugLog(<< "MultipartMixedContents::parse");

   parseHeaders(pb);
   // back up two characters to include the CRLF in the boundary, sigh
   pb.reset(pb.position()-2);

   // determine the boundary
   const Data& boundaryToken = mType.param("boundary");
   Data boundary(boundaryToken.size() + 4, true);
   boundary = Symbols::CRLF;
   boundary += Symbols::DASHDASH;
   boundary += boundaryToken;

   pb.skipToChars(boundary);
   do
   {
      DebugLog(<< "MultipartMixedContents::parse <" << pb.position() << ">");
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
         break;
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

      const char* headerStart = pb.skipWhitespace();

      // pull out contents type only
      pb.skipToChars("Content-Type");
      pb.skipToOneOf(Symbols::COLON, ParseBuffer::Whitespace);
      pb.skipWhitespace();
      const char* typeStart = pb.skipChar(Symbols::COLON[0]);

      // determine contents-type header buffer
      pb.skipToTermCRLF();

      ParseBuffer subPb(typeStart, pb.position() - typeStart);
      Mime contentType;
      contentType.parse(subPb);

      // determine contents buffer
      pb.skipToChars(boundary);
      pb.assertNotEof();
      Data tmp;
      pb.data(tmp, headerStart);
      mContents.push_back(createContents(contentType, tmp));
   }
   while (!pb.eof());

   // replace the boundary
   setBoundary();
}
