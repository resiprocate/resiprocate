#include "StringSubComponent.hxx"

using namespace Vocal2;
using namespace std;


StringSubComponent::StringSubComponent(Type type,
                                 const char* startData, unsigned int dataSize)
   : SubComponent(type), 
     mData(startData, dataSize)
{}


StringSubComponent::StringSubComponent(Type type, const Data& data)
   : SubComponent(type), 
     mData(data)
{}

Data& 
StringSubComponent::value()
{
   return mData;
}

SubComponent* 
StringSubComponent::clone() const
{
   return new StringSubComponent(*this);
}

ostream& Vocal2::operator<<(ostream& stream, StringSubComponent& comp)
{
   return stream << comp.getName() << "=" << comp.value();
}
