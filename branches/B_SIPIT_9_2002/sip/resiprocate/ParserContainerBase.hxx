#ifndef ParserContainerBase_hxx
#define ParserContainerBase_hxx

#include <sipstack/ParserCategory.hxx>

namespace Vocal2
{

class HeaderFieldValueList;

class ParserContainerBase : public ParserCategory
{
   public:
      virtual ParserCategory* clone(HeaderFieldValue*) const 
      {
         assert(0);
         return 0;
      }
      
      virtual void parse() {}
      virtual ParserContainerBase* clone(HeaderFieldValueList& hfvs) const = 0;
};
 
}

#endif
