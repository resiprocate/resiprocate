#include <iostream>

#include <sipstack/UnknownSubComponent.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/ParserCategory.hxx>

using namespace std;
using namespace Vocal2;

HeaderFieldValue::HeaderFieldValue()
   : next(0),
     mParserCategory(0),
     mField(0),
     mFieldLength(0),
     mMine(false)
{}

HeaderFieldValue::HeaderFieldValue(const char* field, uint fieldLength)
  : next(0),
    mParserCategory(0),
    mField(field),
    mFieldLength(fieldLength),
    mMine(false)
{}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv)
  : next(hfv.next),
    mParserCategory(hfv.mParserCategory->clone(this)),
    mField(0),
    mFieldLength(hfv.mFieldLength),
    mSubComponentList(hfv.mSubComponentList),
    mUnknownSubComponentList(hfv.mUnknownSubComponentList),
    mMine(true)
{

  // if this isn't parsed, chunk and copy the block of memory
  // the copy for the param lists will end up with empty lists
  if (!(isParsed()))
    {
      const_cast<char*>(mField) = new char[mFieldLength];
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

HeaderFieldValue* 
HeaderFieldValue::clone() const
{
  return new HeaderFieldValue(*this);
}

SubComponentList& 
HeaderFieldValue::getSubComponents()
{
  return mSubComponentList;
}

SubComponentList& 
HeaderFieldValue::getUnknownSubComponents()
{
  return mSubComponentList;
}

bool 
HeaderFieldValue::isParsed() const
{
  return mParserCategory != 0;
}


bool 
HeaderFieldValue::exists(const Data& subcomponent)
{
  
  SubComponent* exists = mUnknownSubComponentList.find(subcomponent);
  if (!exists)
    {
      exists = mSubComponentList.find(subcomponent);
      if (exists)
	{
	  ParseException except;
	  throw except;
	}
    }
  return exists;
}


bool 
HeaderFieldValue::exists(const SubComponent::Type type)
{
  
  return mSubComponentList.find(type);
  
}

UnknownSubComponent* 
HeaderFieldValue::get(const Data& type)
{
  return dynamic_cast<UnknownSubComponent*>(mUnknownSubComponentList.get(type));
}

ostream& Vocal2::operator<<(ostream& stream, HeaderFieldValue& hfv)
{
  if (hfv.isParsed())
    {
      stream << hfv.mSubComponentList << " : " << hfv.mUnknownSubComponentList;
    }
  else
    {
      stream << Data(hfv.mField, hfv.mFieldLength);
    }
  return stream;
}



