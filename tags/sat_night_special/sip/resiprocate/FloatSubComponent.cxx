#include "FloatSubComponent.hxx"

using namespace Vocal2;
using namespace std;

FloatSubComponent::FloatSubComponent(Type type,
                                     float value)
   : SubComponent(type),
     mValue(value)
{}

float& FloatSubComponent::value()
{
   return mValue;
}

SubComponent* 
FloatSubComponent::clone() const
{
   return new FloatSubComponent(*this);
}

ostream& Vocal2::operator<<(ostream& stream, FloatSubComponent& comp)
{
   return stream << comp.getName() << "=" << comp.value();
}
