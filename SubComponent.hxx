#ifndef SUBCOMPONENT_HXX
#define SUBCOMPONENT_HXX

#include <string>

namespace Vocal2
{


class SubComponent
{
   public:

      enum ParamType{ Unknown, TTL, Transport, Maddr, LR, Method , User };
      
      SubComponent(ParamType type);
      virtual ~SubComponent() {}
      
      ParamType getType();

      virtual const std::string& getName();
      virtual SubComponent* clone() const;

      SubComponent* next;
      
      

   private:
      static std::string  ParamString[];
      ParamType mType;

};
 
}


#endif
