#include <sipstack/ParseCategory.hxx>
#include <sipstack/UnknownSubComponent.hxx>

UnknownSubComponent&
ParseCategory::operator[](const Data& param)
{
   checkParsed();
   return *mHeaderField->get(param);
}
