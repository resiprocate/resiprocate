#include <sipstack/ParserCategory.hxx>
#include <sipstack/UnknownSubComponent.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <iostream>

UnknownSubComponent&
ParseCategory::operator[](const Data& param)
{
   checkParsed();
   return *mHeaderField->get(param);
}

void
ParseCategory::parseParameters(const char* start)
{
   mHeaderField->parseParameters(start);
}

ostream&
Vocal2::operator<<(ostream&, const ParserCategory& category)
{
   return category.encode(stream);
}

