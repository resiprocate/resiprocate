#ifndef STRINGSUBCOMPONENT_HXX
#define STRINGSUBCOMPONENT_HXX

#include <iostream>

#include <sipstack/SubComponent.hxx>
#include <sipstack/Data.hxx>


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

#ifndef WIN32
// CJ TODO FIX 
std::ostream& Vocal2::operator<<(std::ostream& stream, Vocal2::StringSubComponent& comp);
#endif
 
}

#endif
