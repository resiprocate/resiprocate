#include <sipstack/ParserCategories.hxx>

using namespace Vocal2;

ParserCategory* 
Unknown::clone(HeaderFieldValue* hfv) const
{
   return new Unknown(*hfv);
}

void
Unknown::parse()
{
   
}

