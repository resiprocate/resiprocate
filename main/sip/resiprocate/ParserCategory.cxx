#include <sipstack/ParserCategory.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/UnknownParameter.hxx>
#include <iostream>

using namespace Vocal2;

UnknownParameter&
ParserCategory::operator[](const Data& param)
{
   checkParsed();
   return *mHeaderField->get(param);
}

void
ParserCategory::parseParameters(const char* start)
{
   mHeaderField->parseParameters(start);
}

std::ostream&
Vocal2::operator<<(std::ostream& stream, const ParserCategory& category)
{
   return category.encode(stream);
}

