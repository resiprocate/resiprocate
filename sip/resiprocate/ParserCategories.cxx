#include <sipstack/ParserCategories.hxx>

using namespace Vocal2;


void 
StringComponent::parse()
{
   mValue = Data(getHeaderField().mField, getHeaderField().mFieldLength);
}

std::ostream& 
StringComponent::encode(std::ostream& str) const
{
   return str;
}


Data& 
StringComponent::value() 
{ 
   checkParsed();
   return mValue; 
}
