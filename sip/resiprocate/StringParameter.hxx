#ifndef STRINGPARAM_HXX
#define STRINGPARAM_HXX

#include <sip2/sipstack/Parameter.hxx>
#include <string>

namespace Vocal2
{

class StringParameter : public Parameter
{

   public:

      StringParameter(ParamType type,
                      const char* startData, uint dataSize);
      
      StringParameter(ParamType type, const std::string& data);
      
      std::string& getData();
      virtual Parameter* clone() const;
   private:

      std::string mData;

};
 
}

#endif
