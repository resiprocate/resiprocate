#ifndef SUBCOMPONENT_HXX
#define SUBCOMPONENT_HXX

#include <sipstack/Data.hxx>
#include <iostream>

namespace Vocal2
{


class SubComponent
{
   public:

      enum Type{ Unknown, TTL, Transport, Maddr, LR, Method, User };
      
      SubComponent(Type type);
      virtual ~SubComponent() {}
      
      Type getType() const;

      virtual const Data& getName() const;

      virtual SubComponent* clone() const;

      SubComponent* next;
   private:
      static Data  ParamString[];
      Type mType;

};
 
std::ostream& operator<<(std::ostream& stream, const SubComponent& comp);

}




#endif
