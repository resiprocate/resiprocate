#include "sip2/sipstack/MultipartMixedContents.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"

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

   // !dlb! needs to come from context
   const Data boundary = "--boundary";

   mContentsType.param("boundary") = boundary;
   
   for (list<Contents*>::const_iterator i = mContents.begin(); 
        i != mContents.end(); i++)
   {
      str << Symbols::CRLFCRLF;
      str << boundary << Symbols::CRLF;
      (*i)->encode(str);
      str << boundary << Symbols::CRLF;
   }

   return str;
}

void 
MultipartMixedContents::parse(ParseBuffer& pb)
{
   DebugLog(<< "MultipartMixedContents::parse: " << pb.position());
   
   // determine the boundary
   const Data& boundary = mContentsType.param("boundary");

   pb.skipToChars(boundary);
   do
   {
      // !dlb! case sensitive?
      pb.skipToChars("Content-Type");
      pb.skipToOneOf(Symbols::COLON, ParseBuffer::Whitespace);
      
      Mime contentType;
      contentType.parse(pb);

      pb.skipToChars(Symbols::CRLFCRLF);
      const char* anchor = pb.skipN(4);
      pb.skipToChars(boundary);

      mContents.push_back(createContents(contentType, anchor, pb));
   }
   while (!pb.eof());
}
