#include <iostream>

#include <sipstack/UnknownParameter.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/ParserCategory.hxx>
#include <sipstack/Symbols.hxx>

#include <util/Logger.hxx>

using namespace std;
using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

HeaderFieldValue::HeaderFieldValue()
   : next(0),
     mParserCategory(0),
     mField(0),
     mFieldLength(0),
     mMine(false)
{}

HeaderFieldValue::HeaderFieldValue(const char* field, unsigned int fieldLength)
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
    mParameterList(hfv.mParameterList),
    mUnknownParameterList(hfv.mUnknownParameterList),
    mMine(true)
{
  // if this isn't parsed, chunk and copy the block of memory
  // the copy for the param lists will end up with empty lists
  if (!(isParsed()))
    {
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

HeaderFieldValue* 
HeaderFieldValue::clone() const
{
  return new HeaderFieldValue(*this);
}
      
void 
HeaderFieldValue::parseParameters(const char* startPos, unsigned int length)
{
   Data data(startPos, length);

   Data key;
   Data value;

   bool done = false;
   while(!done)
   {
      bool isQuoted = false;
      char matchedChar=0;
      
      key = data.matchChar(Symbols::SEMI_OR_EQUAL, &matchedChar);
      if(matchedChar == Symbols::EQUALS[0])
      {
         // this is a separator, so stuff before this is a thing
         value = data.matchChar(Symbols::SEMI_COLON, &matchedChar);
         if(matchedChar != Symbols::SEMI_COLON[0])
         {
            value = data;
            done = true;
         }

         int eaten = value.eatWhiteSpace();
         if (value.size() && value[0] == Symbols::DOUBLE_QUOTE[0])
         {
            isQuoted = true;
            value = value.substr(1, value.size()-2);
         }
         else if (value.empty())
         {
            DebugLog (<< "Found no value after param " << key);
            throw ParseException("error parsing param", __FILE__,__LINE__);
         }
         else if (eaten)
         {
            DebugLog (<< "Found whitespace before non-quoted value in param: " << key);
            throw ParseException("error parsing param", __FILE__,__LINE__);
         }

         ParameterTypes::Type type = ParameterTypes::getType(key.data(), key.size());
         if (type == ParameterTypes::UNKNOWN)
         {
            UnknownParameter* p = new UnknownParameter(key.data(), key.size(), value.data(), value.size());
            p->setQuoted(isQuoted);
            mParameterList.insert(p);
         }
         else
         {
            //Parameter* p = ParameterTypes::make(type);
         }
      }
      else if(matchedChar == Symbols::SEMI_COLON[0])
      {
         // no value here, so just stuff nothing into the value
         assert (key != "");
         //operator[](key) = "";
      }
      else
      {
         // nothing left, so done
         // done
         if(data.size() != 0)
         {
            assert (data != "");
            //operator[](data) = "";
         }
         done = true;
      }
   }
}


ParameterList& 
HeaderFieldValue::getParameters()
{
  return mParameterList;
}

ParameterList& 
HeaderFieldValue::getUnknownParameters()
{
  return mParameterList;
}

bool 
HeaderFieldValue::isParsed() const
{
  return mParserCategory != 0;
}


bool 
HeaderFieldValue::exists(const Data& subcomponent)
{
  
  Parameter* exists = mUnknownParameterList.find(subcomponent);
  if (!exists)
  {
     exists = mParameterList.find(subcomponent);
     if (exists)
     {
        throw ParseException("???", __FILE__, __LINE__); // !jf!
     }
  }
  return exists;
}


bool 
HeaderFieldValue::exists(const ParameterTypes::Type type)
{
  
  return mParameterList.find(type);
  
}
void 
HeaderFieldValue::remove(const ParameterTypes::Type type)
{
   mParameterList.erase(type);
}

void 
HeaderFieldValue::remove(const Data& parameter)
{
   mUnknownParameterList.erase(parameter);
}

UnknownParameter* 
HeaderFieldValue::get(const Data& type)
{
  return dynamic_cast<UnknownParameter*>(mUnknownParameterList.get(type));
}

ostream& 
HeaderFieldValue::encode(ostream& str) const
{
   if (mParserCategory != 0 && mParserCategory->isParsed())
   {
      return mParserCategory->encode(str);
   }
   str.write(mField, mFieldLength);
   str << endl;
   return str;
}

ostream& Vocal2::operator<<(ostream& stream, HeaderFieldValue& hfv)
{
   if (hfv.isParsed())
   {
      hfv.mParameterList.encode(stream);
      stream << Symbols::SPACE << Symbols::COLON << Symbols::SPACE;
      hfv.mUnknownParameterList.encode(stream);
   }
   else
   {
      stream << Data(hfv.mField, hfv.mFieldLength);
   }
   return stream;
}



