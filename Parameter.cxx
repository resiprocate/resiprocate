#include <sip2/sipstack/Parameter.hxx>


using namespace Vocal2;
using namespace std;


string Parameter::ParamString[] = { "unknown", "ttl", "transport", "maddr", "lr", "method", "user" };

Parameter::Parameter(ParamType type)
   : next(0),
     mType(type)
{}


Parameter::ParamType 
Parameter::getType()
{
   return mType;
}


const string& 
Parameter::getName()
{
   return Parameter::ParamString[mType];
}

Parameter* 
Parameter::clone() const
{
   return new Parameter(*this);
}

