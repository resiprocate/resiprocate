#include "resiprocate/MultipartSignedContents.hxx"
#include "resiprocate/SipMessage.hxx"
#include "resiprocate/os/Logger.hxx"
//#include "resiprocate/EncodingContext.hxx"
#include "resiprocate/os/Random.hxx"

using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM Subsystem::CONTENTS

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

