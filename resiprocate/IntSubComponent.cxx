#include "IntSubComponent.hxx"

using namespace Vocal2;
using namespace std;

IntSubComponent::IntSubComponent(Type type,
                                int value)
   : SubComponent(type),
     mValue(value)
{}
      
int& IntSubComponent::value()
{
   return mValue;
}

SubComponent* 
IntSubComponent::clone() const
{
   return new IntSubComponent(*this);
}

ostream& operator<<(ostream& stream, IntSubComponent& comp)
{
   return stream << comp.getName() << "=" << comp.value();
}
