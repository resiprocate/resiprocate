#include <sip2/sipstack/UnknownSubComponent.hxx>

using namespace Vocal2;
using namespace std;

UnknownSubComponent::UnknownSubComponent(const char* startName, unsigned int nameSize,
                                   const char* startData, unsigned int dataSize)
   : StringSubComponent(SubComponent::Unknown, startData, dataSize),
     mName(startName, nameSize)
{
}


UnknownSubComponent::UnknownSubComponent(const string& name, const string& data)
   : StringSubComponent(SubComponent::Unknown, data),
     mName(name)
{
}


const string& 
UnknownSubComponent::getName()
{
   return mName;
}

SubComponent* 
UnknownSubComponent::clone() const
{
   return new UnknownSubComponent(*this);
}

ostream& operator<<(ostream& stream, const UnknownSubComponent& comp)
{
   return ostream << comp.getName() << "=" << comp.getString();
}
