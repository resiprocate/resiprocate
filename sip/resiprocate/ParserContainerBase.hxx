#ifndef ParserContainerBase_hxx
#define ParserContainerBase_hxx

#include <sipstack/ParserCategory.hxx>
#include <iostream>
#include <sipstack/HeaderTypes.hxx>

namespace Vocal2
{

class HeaderFieldValueList;

class ParserContainerBase
{
   public:
      ParserContainerBase(Headers::Type type = Headers::UNKNOWN)
         : mType(type)
      {}
      ParserContainerBase(const ParserContainerBase&)
         : mType(Headers::UNKNOWN)
      {}
      virtual ~ParserContainerBase() {}
      virtual ParserContainerBase* clone() const = 0;
      virtual std::ostream& encode(std::ostream& str) const = 0;

   protected:
      const Headers::Type mType;

};
 
}

#endif
