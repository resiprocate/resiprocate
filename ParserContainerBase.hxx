#ifndef ParserContainerBase_hxx
#define ParserContainerBase_hxx

#include <cassert>

#include <sipstack/ParserCategory.hxx>
#include <iostream>

namespace Vocal2
{

class HeaderFieldValueList;

class ParserContainerBase
{
   public:
      virtual ~ParserContainerBase() {}
      virtual ParserContainerBase* clone() const = 0;
      virtual std::ostream& encode(std::ostream& str) const = 0;
};
 
}

#endif
