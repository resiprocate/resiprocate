#include "StringParameter.hxx"

using namespace Vocal2;
using namespace std;


StringParameter::StringParameter(ParamType type,
                                 const char* startData, uint dataSize)
   : Parameter(type), 
     mData(startData, dataSize)
{
}


StringParameter::StringParameter(ParamType type, const string& data)
   : Parameter(type), 
     mData(data)
{
}


string& 
StringParameter::getData()
{
   return mData;
}


Parameter* 
StringParameter::clone() const
{
   return new StringParameter(*this);
}

