#ifndef UnknownParameter_hxx
#define UnknownParameter_hxx

#include <sipstack/DataParameter.hxx>

namespace Vocal2
{

class UnknownParameter : public DataParameter
{
   public:
      UnknownParameter(const char* startName, unsigned int nameSize,
                       const char* startData, unsigned int dataSize);
      
      // for making a new unknown parameter 
      // msg->header(foo)["mynewparam"] = "bar";
      UnknownParameter(const Data& name);
      
      virtual const Data& getName();
      virtual Parameter* clone() const;
   private:
      Data mName;
};

}

std::ostream& operator<<(std::ostream& stream, Vocal2::UnknownParameter& comp);

#endif
