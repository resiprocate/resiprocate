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
      
      UnknownParameter(const Data& name);
      
      virtual const Data& getName();
      virtual Parameter* clone() const;
   private:
      Data mName;
};

}

std::ostream& operator<<(std::ostream& stream, Vocal2::UnknownParameter& comp);

#endif
