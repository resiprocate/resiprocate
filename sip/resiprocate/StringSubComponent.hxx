#ifndef STRINGSUBCOMPONENT_HXX
#define STRINGSUBCOMPONENT_HXX

#include <sip2/sipstack/SubComponent.hxx>
#include <string>
#include <iostream>

namespace Vocal2
{

class StringSubComponent : public SubComponent
{
   public:
      StringSubComponent(Type type,
                         const char* startData, unsigned int dataSize);
      
      StringSubComponent(Type type, const std::string& data);
      
      std::string& value();
      virtual SubComponent* clone() const;
   private:
      std::string mData;
};
 
}

std::ostream& operator<<(std::ostream& stream, Vocal2::StringSubComponent& comp);

#endif
