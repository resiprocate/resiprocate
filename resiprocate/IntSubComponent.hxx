#ifndef IntSubComponent_hxx
#define IntSubComponent_hxx

#include <sip2/sipstack/SubComponent.hxx>
#include <iostream>

namespace Vocal2
{

class IntSubComponent : public SubComponent
{
   public:
      IntSubComponent(Type type,
                      int value);
      int& value();
      virtual SubComponent* clone() const;
   private:
      int mValue;
      std::string mData;
};
 
std::ostream& Vocal2::operator<<(std::ostream& stream, Vocal2::IntSubComponent& comp);

}




#endif
