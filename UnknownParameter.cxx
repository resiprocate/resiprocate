#include <sipstack/UnknownParameter.hxx>

using namespace Vocal2;
using namespace std;

UnknownParameter::UnknownParameter(const char* startName, unsigned int nameSize,
                                   const char* startData, unsigned int dataSize)
   : DataParameter(ParameterTypes::UNKNOWN, startData, dataSize),
     mName(startName, nameSize)
{
}


UnknownParameter::UnknownParameter(const Data& name, const Data& data)
   : DataParameter(ParameterTypes::UNKNOWN, data),
     mName(name)
{
}

const Data& 
UnknownParameter::getName()
{
   return mName;
}

Parameter* 
UnknownParameter::clone() const
{
   return new UnknownParameter(*this);
}

ostream& operator<<(ostream& stream, UnknownParameter& comp)
{
   return stream << comp.getName() << "=" << comp.value();
}
