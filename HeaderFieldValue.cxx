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

#ifndef WIN32
// !dlb! when does this get called?
HeaderFieldValue::HeaderFieldValue(ParserCategory* parser)
   : mParserCategory(parser)
{}
#endif

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

// ;lr;ttl=7;user=phone
// pass as ParseBuffer& (who makes it?)
// extract key, induce parse generically
void 
HeaderFieldValue::parseParameters(ParseBuffer& pb)
{
   while (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0])
   {
      cerr << "HeaderFieldValue::parseParameters" << endl;
      cerr << pb.position() << endl;
      // extract the key
      const char* keyStart = pb.skipChar();
      const char* keyEnd = pb.skipToOneOf(";=?");  //!dlb! @ here?
      cerr.write(keyStart, keyEnd - keyStart);
      cerr << endl;
      
      ParameterTypes::Type type = ParameterTypes::getType(keyStart, (keyEnd - keyStart));

      if (type == ParameterTypes::UNKNOWN)
      {
         cerr << "HeaderFieldValue::parseParameters:UNKNOWN" << endl;
         mUnknownParameterList.insert(new UnknownParameter(keyStart, (keyEnd - keyStart), pb));
      }
      else
      {
         cerr << "HeaderFieldValue::parserParameters:invokingFactory" << endl;
         // invoke the particular factory
         mParameterList.insert(ParameterTypes::ParameterFactories[type](type, pb));
         cerr << "HeaderFieldValue::parserParameters:insert: " << mParameterList << endl;
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



