#include "resiprocate/MultipartMixedContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"
//#include "resiprocate/EncodingContext.hxx"
#include "resiprocate/os/Random.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

ContentsFactory<MultipartMixedContents> MultipartMixedContents::Factory;

MultipartMixedContents::MultipartMixedContents()
   : Contents(getStaticType()),
     mContents()
{
   setBoundary();
}

MultipartMixedContents::MultipartMixedContents(const Mime& contentsType)
   : Contents(contentsType),
     mContents()
{
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
   mType.param(p_boundary) = boundaryToken;
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
{
   clear();
}

MultipartMixedContents&
MultipartMixedContents::operator=(const MultipartMixedContents& rhs)
{
   if (this != &rhs)
   {
      Contents::operator=(rhs);
      clear();
      
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
MultipartMixedContents::getStaticType() 
{
   static Mime type("multipart","mixed");
   return type;
}

std::ostream& 
MultipartMixedContents::encodeParsed(std::ostream& str) const
{
   const Data& boundaryToken = mType.param(p_boundary);
   Data boundary(boundaryToken.size() + 2, true);
   boundary = Symbols::DASHDASH;
   boundary += boundaryToken;

   assert( mContents.size() > 0 );
   
   bool first = true;
   for (list<Contents*>::const_iterator i = mContents.begin(); 
        i != mContents.end(); i++)
   {
      if (!first)
      {
         str << Symbols::CRLF;
      }
      else
      {
         first = false;
      }
      str << boundary << Symbols::CRLF;
      (*i)->encodeHeaders(str);
      (*i)->encode(str);
   }

   str << Symbols::CRLF << boundary << Symbols::DASHDASH;
   return str;
}

// The boundary delimiter MUST occur at the beginning of a line, i.e., following
// a CRLF, and the initial CRLF is considered to be attached to the boundary
// delimiter line rather than part of the preceding part.
void 
MultipartMixedContents::parse(ParseBuffer& pb)
{
   const Data& boundaryToken = mType.param(p_boundary);
   
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
 
      assert( !pb.eof() && *pb.position() == Symbols::CR[0] );
      pb.skipChar();
      assert( !pb.eof() && *pb.position() == Symbols::LF[0] );
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
      
      pb.assertNotEof();

      // determine body start
      pb.reset(typeStart);
      const char* bodyStart = pb.skipToChars(Symbols::CRLFCRLF);
      pb.assertNotEof();
      bodyStart += 4;

      // determine contents body buffer
      pb.skipToChars(boundary);
      pb.assertNotEof();
      Data tmp;
      pb.data(tmp, bodyStart);
      // create contents against body
      mContents.push_back(createContents(contentType, tmp));
      // pre-parse headers
      ParseBuffer headersPb(headerStart, bodyStart-4-headerStart);
      mContents.back()->preParseHeaders(headersPb);

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

// ---------------------------------------------------
ContentsFactory<MultipartRelatedContents> MultipartRelatedContents::Factory;

MultipartRelatedContents::MultipartRelatedContents()
   : MultipartMixedContents(getStaticType())
{}

MultipartRelatedContents::MultipartRelatedContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : MultipartMixedContents(hfv, contentsType)
{}

MultipartRelatedContents::MultipartRelatedContents(const MultipartRelatedContents& rhs)
   : MultipartMixedContents(rhs)
{}

MultipartRelatedContents&
MultipartRelatedContents::operator=(const MultipartRelatedContents& rhs)
{
   MultipartMixedContents::operator=(rhs);
   return *this;
}

Contents* 
MultipartRelatedContents::clone() const
{
   return new MultipartRelatedContents(*this);
}

const Mime& 
MultipartRelatedContents::getStaticType() 
{
   static Mime type("multipart","related");
   return type;
}
