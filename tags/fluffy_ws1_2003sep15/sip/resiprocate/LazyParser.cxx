#if defined(HAVE_CONFIG_H)
#include "resiprocate/config.hxx"
#endif


#include <cassert>

#include "resiprocate/LazyParser.hxx"
#include "resiprocate/os/ParseBuffer.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/Headers.hxx"

using namespace resip;

LazyParser::LazyParser(HeaderFieldValue* headerFieldValue)
   : mHeaderField(headerFieldValue),
      mIsMine(false),
      mIsParsed(mHeaderField->mField == 0) //although this has a hfv it is
                                           //parsed, as the hfv has no content
{
}
  
LazyParser::LazyParser()
   : mHeaderField(0),
     mIsMine(true),
     mIsParsed(true)
{
}

LazyParser::LazyParser(const LazyParser& rhs)
   : mHeaderField(0),
     mIsMine(true),
     mIsParsed(rhs.mIsParsed)
{
   if (!mIsParsed && rhs.mHeaderField)
   {
      mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
   }
}

LazyParser::~LazyParser()
{
   clear();
}

LazyParser&
LazyParser::operator=(const LazyParser& rhs)
{
   assert( &rhs != 0 );
   
   if (this != &rhs)
   {
      clear();
      mIsParsed = rhs.mIsParsed;
      if (rhs.mIsParsed)
      {
         mHeaderField = 0;
         mIsMine = false;
      }
      else
      {
         mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
         mIsMine = true;
      }
   }
   return *this;
}

void
LazyParser::checkParsed() const
{
   if (!mIsParsed)
   {
      LazyParser* ncThis = const_cast<LazyParser*>(this);
      ncThis->mIsParsed = true;
      ParseBuffer pb(mHeaderField->mField, mHeaderField->mFieldLength, errorContext());
      ncThis->parse(pb);
   }
}

void
LazyParser::clear()
{
   if (mIsMine)
   {
      delete mHeaderField;
      mHeaderField = 0;
   }
}

std::ostream&
LazyParser::encode(std::ostream& str) const
{
   if (isParsed())
   {
      return encodeParsed(str);
   }
   else
   {
      assert(mHeaderField);
      mHeaderField->encode(str);
      return str;
   }
}

std::ostream&
resip::operator<<(std::ostream& s, const LazyParser& lp)
{
   lp.encode(s);
   return s; 
}
