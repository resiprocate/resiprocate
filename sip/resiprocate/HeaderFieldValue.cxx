#include <iostream>

#include <sipstack/UnknownParameter.hxx>
#include <sipstack/ExistsParameter.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/ParserCategory.hxx>
#include <sipstack/Symbols.hxx>
#include <util/ParseBuffer.hxx>
#include <util/Logger.hxx>

using namespace std;
using namespace Vocal2;


#define VOCAL_SUBSYSTEM Subsystem::SIP

HeaderFieldValue::HeaderFieldValue(const char* field, unsigned int fieldLength)
   : mParserCategory(0),
     mField(field),
     mFieldLength(fieldLength),
     mMine(false)
{}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv,
                                   ParserCategory* assignParser)
   : mParserCategory(assignParser),
     mField(0),
     mFieldLength(hfv.mFieldLength),
     mMine(true)
{
   // if this isn't parsed, chunk and copy the block of memory
   if (!isParsed())
   {
      const_cast<char*&>(mField) = new char[mFieldLength];
      memcpy(const_cast<char*>(mField), hfv.mField, mFieldLength);
   }
}

HeaderFieldValue::~HeaderFieldValue()
{
  if (mMine)
  {
     HeaderFieldValue* ncThis = const_cast<HeaderFieldValue*>(this);      
     delete [] ncThis->mField;
  }
}

bool 
HeaderFieldValue::isParsed() const
{
   return (mParserCategory != 0 && mParserCategory->isParsed());
}

ostream& 
HeaderFieldValue::encode(ostream& str) const
{
   if (mParserCategory != 0 && mParserCategory->isParsed())
   {
      return mParserCategory->encode(str);
   }
   str.write(mField, mFieldLength);
   return str;
}

ostream& Vocal2::operator<<(ostream& stream, HeaderFieldValue& hfv)
{
   if (hfv.isParsed())
   {
      hfv.mParserCategory->encode(stream);
   }
   else
   {
      stream << Data(hfv.mField, hfv.mFieldLength);
   }
   return stream;
}




