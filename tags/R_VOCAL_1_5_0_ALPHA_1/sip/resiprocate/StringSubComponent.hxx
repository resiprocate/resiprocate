#ifndef STRINGSUBCOMPONENT_HXX
#define STRINGSUBCOMPONENT_HXX

#include <sipstack/SubComponent.hxx>
#include <sipstack/Data.hxx>
#include <iostream>

namespace Vocal2
{

class StringSubComponent : public SubComponent
{
   public:
      StringSubComponent(Type type,
                         const char* startData, unsigned int dataSize);
      
      StringSubComponent(Type type, const Data& data);
      
      Data& value();
      virtual SubComponent* clone() const;
   private:
      Data mData;
};

std::ostream& Vocal2::operator<<(std::ostream& stream, StringSubComponent& comp);
 
}

#endif
