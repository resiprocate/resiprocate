

#include <cassert>


#include <sipstack/DataParameter.hxx>
#include <sipstack/Symbols.hxx>

using namespace Vocal2;
using namespace std;

DataParameter::DataParameter(ParameterTypes::Type type,
                             const char* startData, unsigned int dataSize)
   : Parameter(type), 
     mData(startData, dataSize),
     mQuoted(false)
{
   assert(0);
}

DataParameter::DataParameter(ParameterTypes::Type type)
   : Parameter(type),
     mQuoted(false)
{
}

Data& 
DataParameter::value()
{
   return mData;
}


Parameter* 
DataParameter::clone() const
{
   return new DataParameter(*this);
}

ostream& 
DataParameter::encode(ostream& stream) const
{
   if (mQuoted)
   {
      return stream << getName() << Symbols::EQUALS << Symbols::DOUBLE_QUOTE << mData << Symbols::DOUBLE_QUOTE;
   }
   else
   {
      return stream << getName() << Symbols::EQUALS << mData;
   }
}
