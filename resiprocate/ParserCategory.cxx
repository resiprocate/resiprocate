#include <sipstack/ParserCategory.hxx>
#include <sipstack/UnknownSubComponent.hxx>
#include <sipstack/HeaderFieldValue.hxx>

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
