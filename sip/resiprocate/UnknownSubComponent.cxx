#include <sipstack/UnknownSubComponent.hxx>

using namespace Vocal2;
using namespace std;

UnknownSubComponent::UnknownSubComponent(const char* startName, unsigned int nameSize,
                                   const char* startData, unsigned int dataSize)
   : StringSubComponent(SubComponent::Unknown, startData, dataSize),
     mName(startName, nameSize)
{
}


UnknownSubComponent::UnknownSubComponent(const Data& name, const Data& data)
   : StringSubComponent(SubComponent::Unknown, data),
     mName(name)
{
}


const Data& 
UnknownSubComponent::getName()
{
   return mName;
}

SubComponent* 
UnknownSubComponent::clone() const
{
   return new UnknownSubComponent(*this);
}

ostream& operator<<(ostream& stream, UnknownSubComponent& comp)
{
   return stream << comp.getName() << "=" << comp.value();
}
