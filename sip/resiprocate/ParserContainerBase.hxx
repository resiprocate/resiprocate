#ifndef ParserContainerBase_hxx
#define ParserContainerBase_hxx

#include <cassert>

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
      
      virtual void parse() {assert(0);}
      
      virtual ParserContainerBase* clone(HeaderFieldValueList* hfvs) const = 0;
      virtual std::ostream& encode(std::ostream& str) const { assert(0); return str;}
};
 
}

#endif
