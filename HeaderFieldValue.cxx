HeaderFieldValue::HeaderFieldValue(const char* field, uint fieldLength)
  : mField(field),
    mFieldLength(fieldLength),
    mComponent(0)
{}

HeaderFieldValue::HeaderFieldValue(const HeaderFieldValue& hfv)
  : mField(0),
    mFieldLength(0),
    mParamList(hfv.mParamList),
    mUnkownParamList(hfv.UnknownParamList),
    mComponent(hfv.mComponent->clone(this))
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

ParameterList& HeaderFieldValue::getParameters()
{
  return mParamList;
}

ParameterList& HeaderFieldValue::getUnknownParameters()
{
  return mParamList;
}

bool HeaderFieldValue::isParsed() const
{
  return mComponent != 0;
}

