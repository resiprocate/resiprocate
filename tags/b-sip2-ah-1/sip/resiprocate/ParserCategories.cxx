#include <sipstack/ParserCategories.hxx>

using namespace Vocal2;

ParserCategory* 
StringComponent::clone(HeaderFieldValue*) const
{
   assert(0);
   return 0;
}

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


