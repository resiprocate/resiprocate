#ifndef FloatSubComponent_hxx
#define FloatSubComponent_hxx

#include <sip2/sipstack/SubComponent.hxx>
#include <iostream>

namespace Vocal2
{

class FloatSubComponent : public SubComponent
{
   public:
      FloatSubComponent(Type type,
                      float value);
      float& value();
      virtual SubComponent* clone() const;
   private:
      float mValue;
      Data mData;
};
 
std::ostream& operator<<(std::ostream& stream, FloatSubComponent& comp);

}


#endif
