#ifndef UNKONWNPARAM_HXX
#define UNKNONWPARAM_HXX

#include <sip2/sipstack/StringSubComponent.hxx>
#include <string>

namespace Vocal2
{

class UnknownSubComponent : public StringSubComponent
{
   public:
      UnknownSubComponent(const char* startName, unsigned int nameSize,
                       const char* startData, unsigned int dataSize);
      
      UnknownSubComponent(const std::string& name, const std::string& data);
      
      virtual const std::string& getName();
      virtual SubComponent* clone() const;
   private:
      std::string mName;
};

}

std::ostream& operator<<(std::ostream& stream, const UnknownSubComponent& comp);

#endif
