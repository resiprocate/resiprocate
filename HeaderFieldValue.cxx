#include <iostream>

#include <sip2/sipstack/HeaderFieldValue.hxx>
#include <sip2/sipstack/ParserCategory.hxx>
#include <sip2/sipstack/UnknownSubComponent.hxx>

using namespace std;
using namespace Vocal2;

HeaderFieldValue::HeaderFieldValue(const char* field, uint fieldLength)
  : mField(field),
    mFieldLength(fieldLength),
    mParserCategory(0),
    mMine(false)
{}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv)
  : mField(0),
    mFieldLength(0),
    mSubComponentList(hfv.mSubComponentList),
    mUnknownSubComponentList(hfv.mUnknownSubComponentList),
    mParserCategory(hfv.mParserCategory->clone(this)),
    mMine(true)
{

  // if this isn't parsed, chunk and copy the block of memory
  // the copy for the param lists will end up with empty lists
   if (!(isParsed()))
   {
      const_cast<unsigned int&>(mFieldLength) = hfv.mFieldLength;
      const_cast<char*&>(mField) = new char[mFieldLength];
      memcpy(const_cast<char*>(mField), hfv.mField, mFieldLength);
   }
  
  // if it is, the above will end up with null unparsed fields and valid 
  // param lists
  
}

HeaderFieldValue::~HeaderFieldValue()
{
   if (mMine)
   {
      HeaderFieldValue* ncThis = const_cast<HeaderFieldValue*>(this);      
      delete [] ncThis->mField;
   }
}

// make destructor

HeaderFieldValue* HeaderFieldValue::clone() const
{
  return new HeaderFieldValue(*this);
}

SubComponentList& HeaderFieldValue::getSubComponents()
{
  return mSubComponentList;
}

SubComponentList& HeaderFieldValue::getUnknownSubComponents()
{
  return mSubComponentList;
}

bool HeaderFieldValue::isParsed() const
{
  return mParserCategory != 0;
}


bool HeaderFieldValue::exists(const std::string& subcomponent)
{

  SubComponent* exists = mUnknownSubComponentList.find(subcomponent);
  if (!exists)
  {
     exists = mSubComponentList.find(subcomponent);
     if (exists)
     {
        ParseException except();
        throw except;
     }
  }
  return exists;
}


bool HeaderFieldValue::exists(const SubComponent::Type type)
{
  
  return mSubComponentList.find(type);

}

UnknownSubComponent* 
HeaderFieldValue::get(const std::string& type) const
{
   return dynamic_cast<UnknownSubComponent*>(mUnknownSubComponentList.get(type));
}

ostream& operator<<(ostream& stream, HeaderFieldValue& hfv)
{
  if (!hfv.isParsed())
  {
     stream << mSubComponentList() << " : " << mUnknownSubComponentList;
  }
  else
  {
     stream << string(mField, mFieldLength);
  }
}



