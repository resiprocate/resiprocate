#ifndef PARAMETER_HXX
#define PARAMETER_HXX

#include <string>

namespace Vocal2
{


class Parameter
{
   public:

      enum ParamType{ Unknown, TTL, Transport, Maddr, LR, Method , User };
      
      Parameter(ParamType type);
      virtual ~Parameter() {}
      
      ParamType getType();

      virtual const std::string& getName();
      virtual Parameter* clone() const;

      Parameter* next;
      
      

   private:
      static std::string  ParamString[];
      ParamType mType;

};
 
}


#endif
