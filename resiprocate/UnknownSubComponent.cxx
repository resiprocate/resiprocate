#include <sip2/sipstack/UnknownSubComponent.hxx>

using namespace Vocal2;
using namespace std;

UnknownSubComponent::UnknownSubComponent(const char* startName, uint nameSize,
                                   const char* startData, uint dataSize)
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

