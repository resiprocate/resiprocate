#ifndef UNKONWNPARAM_HXX
#define UNKNONWPARAM_HXX

#include <sip2/sipstack/StringParameter.hxx>
#include <string>

namespace Vocal2
{

class UnknownParameter : public StringParameter
{
   public:
      UnknownParameter(const char* startName, uint nameSize,
                       const char* startData, uint dataSize);
      
      UnknownParameter(const std::string& name, const std::string& data);
      
      virtual const std::string& getName();
      virtual Parameter* clone() const;
   private:
      std::string mName;
};
 
}


#endif
