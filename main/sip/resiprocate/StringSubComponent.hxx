#ifndef STRINGPARAM_HXX
#define STRINGPARAM_HXX

#include <sip2/sipstack/SubComponent.hxx>
#include <string>

namespace Vocal2
{

class StringSubComponent : public SubComponent
{

   public:

      StringSubComponent(ParamType type,
                      const char* startData, uint dataSize);
      
      StringSubComponent(ParamType type, const std::string& data);
      
      std::string& getData();
      virtual SubComponent* clone() const;
   private:

      std::string mData;

};
 
}

#endif
