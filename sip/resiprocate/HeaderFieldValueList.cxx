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

HeaderFieldValueList* 
HeaderFieldValueList::clone() const
{
   HeaderFieldValueList* nhfvs = new HeaderFieldValueList;
   
   for (const_iterator i = begin(); i != end(); i++)
   {
      nhfvs->push_back((*i)->clone());
   }

   if (mParserContainer != 0)
   {
      nhfvs->mParserContainer = mParserContainer->clone();
   }
   return nhfvs;
}
