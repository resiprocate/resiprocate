#include <sipstack/HeaderFieldValueList.hxx>
#include <sipstack/ParserContainerBase.hxx>
#include <sipstack/HeaderFieldValue.hxx>

using namespace Vocal2;

HeaderFieldValueList::~HeaderFieldValueList()
{
   for (iterator i = begin(); i != end(); i++)
   {
      delete *i;
   }
   delete mParserContainer;
}

HeaderFieldValueList::HeaderFieldValueList(const HeaderFieldValueList& rhs)
{
   if (mParserContainer != 0)
   {
      mParserContainer = rhs.mParserContainer->clone();
   }
   else
   {
      for (const_iterator i = rhs.begin(); i != rhs.end(); i++)
      {
         push_back(new HeaderFieldValue(**i, 0));
      }
   }
}
