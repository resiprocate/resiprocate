#ifndef UNKONWNPARAM_HXX
#define UNKNONWPARAM_HXX

#include <sip2/sipstack/StringSubComponent.hxx>
#include <string>

namespace Vocal2
{

class UnknownSubComponent : public StringSubComponent
{
   public:
      UnknownSubComponent(const char* startName, uint nameSize,
                       const char* startData, uint dataSize);
      
      UnknownSubComponent(const std::string& name, const std::string& data);
      
      virtual const std::string& getName();
      virtual SubComponent* clone() const;
   private:
      std::string mName;
};
 
}


#endif
