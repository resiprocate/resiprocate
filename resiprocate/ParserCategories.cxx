#include <sipstack/ParserCategories.hxx>

using namespace Vocal2;


/*StringComponent::StringComponent(const StringComponent& other)
   : mValue(other.mValue)
{}
*/
void 
StringComponent::parse()
{
   mValue = Data(getHeaderField().mField, getHeaderField().mFieldLength);
}

std::ostream& 
StringComponent::encode(std::ostream& str) const
{
   str << mValue;
   return str;
}


Data& 
StringComponent::value() 
{ 
   checkParsed();
   return mValue; 
}


