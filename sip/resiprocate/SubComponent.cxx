#include <sip2/sipstack/SubComponent.hxx>


using namespace Vocal2;
using namespace std;


string SubComponent::ParamString[] = { "unknown", "ttl", "transport", "maddr", "lr", "method", "user" };

SubComponent::SubComponent(Type type)
   : next(0),
     mType(type)
{}


SubComponent::Type 
SubComponent::getType() const
{
   return mType;
}


const string& 
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
