#include <sipstack/ParserCategory.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/UnknownParameter.hxx>
#include <iostream>

using namespace Vocal2;

ParserCategory::ParserCategory()
   : mHeaderField(new HeaderFieldValue),
     mIsParsed(true)
{}

ParserCategory::ParserCategory(const ParserCategory& rhs)
   : mIsParsed(rhs.mIsParsed)
{
   if (rhs.mHeaderField)
   {
      mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
   }
}

ParserCategory*
ParserCategory::clone(HeaderFieldValue* hfv) const
{
   ParserCategory* ncthis = const_cast<ParserCategory*>(this);
   
   HeaderFieldValue* old = ncthis->mHeaderField;
   // suppress the HeaderFieldValue copy
   ncthis->mHeaderField = 0;
   ParserCategory* pc = clone();
   ncthis->mHeaderField = old;
   // give the clone the 
   pc->mHeaderField = hfv;

   return pc;
}

UnknownParameter&
ParserCategory::operator[](const Data& param) const
{
   checkParsed();
   return *mHeaderField->get(param);
}

void 
ParserCategory::remove(const Data& param)
{
   checkParsed();
   mHeaderField->remove(param);   
}

bool 
ParserCategory::exists(const Data& param) const
{
   checkParsed();
   return mHeaderField->exists(param);
}

void
ParserCategory::parseParameters(const char* pos, unsigned int length)
{
   // nice to be able to not bother parsing past the end
   mHeaderField->parseParameters(pos, length);
}

std::ostream&
Vocal2::operator<<(std::ostream& stream, const ParserCategory& category)
{
   return category.encode(stream);
}

