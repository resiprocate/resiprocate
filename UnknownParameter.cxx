#include <sip2/sipstack/UnknownParameter.hxx>

using namespace Vocal2;
using namespace std;

UnknownParameter::UnknownParameter(const char* startName, uint nameSize,
                                   const char* startData, uint dataSize)
   : StringParameter(Parameter::Unknown, startData, dataSize),
     mName(startName, nameSize)
{
}


UnknownParameter::UnknownParameter(const string& name, const string& data)
   : StringParameter(Parameter::Unknown, data),
     mName(name)
{
}


const string& 
UnknownParameter::getName()
{
   return mName;
}
