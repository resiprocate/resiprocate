#ifndef UnknownSubComponent_hxx
#define UnknownSubComponent_hxx

#include <sipstack/StringSubComponent.hxx>
#include <sipstack/Data.hxx>

namespace Vocal2
{

class UnknownSubComponent : public StringSubComponent
{
   public:
      UnknownSubComponent(const char* startName, unsigned int nameSize,
                       const char* startData, unsigned int dataSize);
      
      UnknownSubComponent(const Data& name, const Data& data);
      
      virtual const Data& getName();
      virtual SubComponent* clone() const;
   private:
      Data mName;
};

}

std::ostream& operator<<(std::ostream& stream, Vocal2::UnknownSubComponent& comp);

#endif
