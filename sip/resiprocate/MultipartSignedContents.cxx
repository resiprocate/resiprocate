#include "sip2/sipstack/MultipartSignedContents.hxx"
#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/util/Logger.hxx"
//#include "sip2/sipstack/EncodingContext.hxx"
#include "sip2/util/Random.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::SIP

ContentsFactory<MultipartSignedContents> MultipartSignedContents::Factory;


MultipartSignedContents::MultipartSignedContents()
   : MultipartMixedContents()
{
}


MultipartSignedContents::MultipartSignedContents(HeaderFieldValue* hfv, const Mime& contentsType)
   : MultipartMixedContents(hfv, contentsType)
{
}
 

MultipartSignedContents::MultipartSignedContents(const MultipartSignedContents& rhs)
   : MultipartMixedContents(rhs)
{
}


MultipartSignedContents::~MultipartSignedContents()
{
}


MultipartSignedContents&
MultipartSignedContents::operator=(const MultipartSignedContents& rhs)
{
   if (this != &rhs)
   {
      MultipartMixedContents::operator=( rhs);
   }
   return *this;
}


Contents* 
MultipartSignedContents::clone() const
{
   return new MultipartSignedContents(*this);
}


const Mime& 
MultipartSignedContents::getStaticType() const
{
   static Mime type("multipart","signed");
   return type;
}

