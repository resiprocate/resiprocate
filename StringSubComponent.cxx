#include "StringSubComponent.hxx"

using namespace Vocal2;
using namespace std;


StringSubComponent::StringSubComponent(ParamType type,
                                 const char* startData, uint dataSize)
   : SubComponent(type), 
     mData(startData, dataSize)
{
}


StringSubComponent::StringSubComponent(ParamType type, const string& data)
   : SubComponent(type), 
     mData(data)
{
}


string& 
StringSubComponent::getData()
{
   return mData;
}


SubComponent* 
StringSubComponent::clone() const
{
   return new StringSubComponent(*this);
}

