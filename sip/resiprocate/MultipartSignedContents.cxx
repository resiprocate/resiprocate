#include "resiprocate/sipstack/MultipartSignedContents.hxx"
#include "resiprocate/sipstack/SipMessage.hxx"
#include "resiprocate/util/Logger.hxx"
//#include "resiprocate/sipstack/EncodingContext.hxx"
#include "resiprocate/util/Random.hxx"

using namespace Vocal2;
using namespace std;

#define VOCAL_SUBSYSTEM Subsystem::CONTENTS

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
MultipartSignedContents::getStaticType() 
{
   static Mime type("multipart","signed");
   return type;
}

