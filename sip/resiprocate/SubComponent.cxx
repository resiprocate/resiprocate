#include <sipstack/SubComponent.hxx>


using namespace Vocal2;
using namespace std;


Data SubComponent::ParamString[] = { "unknown", "ttl", "transport", "maddr", "lr", "method", "user" };

SubComponent::SubComponent(Type type)
   : next(0),
     mType(type)
{}


SubComponent::Type 
SubComponent::getType() const
{
   return mType;
}


const Data& 
SubComponent::getName() const
{
   return SubComponent::ParamString[mType];
}

SubComponent* 
SubComponent::clone() const
{
   return new SubComponent(*this);
}

ostream& Vocal2::operator<<(ostream& stream, const SubComponent& comp)
{
   return stream << comp.getName();
}
