HeaderFieldValue::HeaderFieldValue(const char* field, uint fieldLength)
  : mField(field),
    mFieldLength(fieldLength),
    mParserCategory(0)
{}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv)
  : mField(0),
    mFieldLength(0),
    mSubComponentList(hfv.mSubComponentList),
    mUnkownSubComponentList(hfv.UnknownSubComponentList),
    mParserCategory(hfv.mParserCategory->clone(this))
{

  // if this isn't parsed, chunk and copy the block of memory
  // the copy for the param lists will end up with empty lists
  if (!(isParsed()))
    {
      mFieldLength = hfv.mFieldLength;
      memcpy(mField, hfv.mField, mFieldLength);
    }
  
  // if it is, the above will end up with null unparsed fields and valid 
  // param lists
  
}

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
  return mComponent != 0;
}


bool HeaderFieldValue::exists(const std::string& subcomponent)
  throw (ParseException&)
{
  
  int exists = mUnknownSubComponentList.find(subcomponent);
  if (!exists)
    {
      exists = mSubComponentList.find(subcomponent);
      if (exists)
	{
	  ParseException except("Asked for a known subcomponent by string",
				__FILE__, __LINE__);
	  throw except;
	}
    }
  return exists;
  
}


bool HeaderFieldValue::exists(const SubComponent::Type type)
{
  
  return mSubComponentList.find(type);

}



SubComponent* HeaderFieldValue::get(const std::string& type) const
{

  return mUnknownSubComponentList.get(type);

}


ostream& operator<<(ostream& stream, HeaderFieldValueList& hList)
{
  if (!isParsed())
    {
      stream << mSubComponentList << " : " << mUnknownSubComponentList;
    }
  else
    {
      stream << string(mField, mFieldLength);
    }
}



