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
   list<Contents*>::const_iterator j;
   const list<Contents*>& list = rhs.parts();
   
   for ( j = list.begin(); 
         j != list.end(); j++)
   {
      assert( *j );
      mContents.push_back( (*j)->clone() );
   }
}


void
MultipartMixedContents::setBoundary()
{
   Data boundaryToken = Random::getRandomHex(8);
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
         mContents.push_back( (*i)->clone() );
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
   Data boundary(boundaryToken.size() + 4, true);
   boundary = Symbols::CRLF;
   boundary += Symbols::DASHDASH;
   boundary += boundaryToken;

   assert( mContents.size() > 0 );
   
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
   parseHeaders(pb);

   const Data& boundaryToken = mType.param("boundary");
   
   Data boundary(boundaryToken.size() + 4, true);
   boundary += Symbols::CRLF;
   boundary += Symbols::DASHDASH;
   boundary += boundaryToken;

   Data boundaryNoCRLF(boundaryToken.size() + 2, true);
   boundaryNoCRLF += Symbols::DASHDASH;
   boundaryNoCRLF += boundaryToken;

   pb.skipToChars(boundaryNoCRLF);
   pb.skipN(boundaryNoCRLF.size());
   pb.assertNotEof();

   do
   {
      // skip over boudary
 
      assert( *pb.position() == Symbols::CR[0] );
      pb.skipChar();
      assert( *pb.position() == Symbols::LF[0] );
      pb.skipChar();
      
      pb.assertNotEof();

      const char* headerStart = pb.position();

      // pull out contents type only
      pb.skipToChars("Content-Type");
      pb.assertNotEof();

      pb.skipToChar(Symbols::COLON[0]);
      pb.skipChar();
      pb.assertNotEof();
      
      pb.skipWhitespace();
      const char* typeStart = pb.position();
      pb.assertNotEof();
      
      // determine contents-type header buffer
      pb.skipToTermCRLF();
      pb.assertNotEof();

      ParseBuffer subPb(typeStart, pb.position() - typeStart);
      Mime contentType;
      contentType.parse(subPb);
      //DebugLog( <<"got type " << contentType );
      
      pb.assertNotEof();

      // determine contents buffer
      pb.skipToChars(boundary);
      pb.assertNotEof();
      Data tmp;
      pb.data(tmp, headerStart);
      mContents.push_back(createContents(contentType, tmp));

      pb.skipN(boundary.size());

      const char* loc = pb.position();
      pb.skipChar();
      pb.skipChar();
      Data next;
      pb.data(next, loc);

      if ( next == Symbols::DASHDASH )
      {
         break;
      }
      pb.reset( loc );
   }
   while ( !pb.eof() );

   // replace the boundary
   setBoundary();
}
