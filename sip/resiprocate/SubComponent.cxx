#include <sip2/sipstack/SubComponent.hxx>


using namespace Vocal2;
using namespace std;


string SubComponent::ParamString[] = { "unknown", "ttl", "transport", "maddr", "lr", "method", "user" };

SubComponent::SubComponent(ParamType type)
   : next(0),
     mType(type)
{}


SubComponent::ParamType 
SubComponent::getType()
{
   return mType;
}


const string& 
SubComponent::getName()
{
   return SubComponent::ParamString[mType];
}

SubComponent* 
SubComponent::clone() const
{
   return new SubComponent(*this);
}

