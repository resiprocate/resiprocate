#include <sipstack/ParserCategory.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/UnknownParameter.hxx>
#include <util/ParseBuffer.hxx>
#include <iostream>

using namespace Vocal2;
using namespace std;

ParserCategory::ParserCategory()
   : mHeaderField(0),
     mIsParsed(true)
{}

ParserCategory::ParserCategory(const ParserCategory& rhs)
   : mIsParsed(rhs.mIsParsed)
{
   if (rhs.mHeaderField)
   {
      mHeaderField = new HeaderFieldValue(*rhs.mHeaderField);
   }
}


ParserCategory::~ParserCategory()
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      delete *it;
   }
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      delete *it;
   }   
}

ParserCategory*
ParserCategory::clone(HeaderFieldValue* hfv) const
{
   ParserCategory* ncthis = const_cast<ParserCategory*>(this);
   
   HeaderFieldValue* old = ncthis->mHeaderField;
   // suppress the HeaderFieldValue copy
   ncthis->mHeaderField = 0;
   ParserCategory* pc = clone();
   ncthis->mHeaderField = old;
   // give the clone the 
   pc->mHeaderField = hfv;

   return pc;
}

UnknownParameter&
ParserCategory::param(const Data& param) const
{
   checkParsed();
   Parameter* p = getParameterByData(param);
   if(!p)
   {
      p = new UnknownParameter(param);
      mUnknownParameters.push_back(p);
   }
   return *dynamic_cast<UnknownParameter*>(p);
}

void 
ParserCategory::remove(const Data& param)
{
   checkParsed();
   removeParameterByData(param);   
}

bool 
ParserCategory::exists(const Data& param) const
{
   checkParsed();
   return getParameterByData(param);
}

void
ParserCategory::parseParameters(ParseBuffer& pb)
{
   while (!pb.eof() && *pb.position() == Symbols::SEMI_COLON[0])
   {
      // extract the key
      const char* keyStart = pb.skipChar();
      const char* keyEnd = pb.skipToOneOf(" \t\r\n;=?>");  //!dlb! @ here?
      
      ParameterTypes::Type type = ParameterTypes::getType(keyStart, (keyEnd - keyStart));

      if (type == ParameterTypes::UNKNOWN)
      {
         mParameters.push_back(new UnknownParameter(keyStart, (keyEnd - keyStart), pb));
      }
      else
      {
         // invoke the particular factory
         mParameters.push_back(ParameterTypes::ParameterFactories[type](type, pb));
      }
      pb.skipToOneOf(" \t\r\n;?>");      
   }
}      

void 
ParserCategory::encodeParameters(std::ostream& str) const
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      (*it)->encode(str);
   }
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      str << Symbols::SEMI_COLON;
      (*it)->encode(str);
   }
}

std::ostream&
Vocal2::operator<<(std::ostream& stream, const ParserCategory& category)
{
   category.checkParsed();
   return category.encode(stream);
}

Parameter* 
ParserCategory::getParameterByEnum(ParameterTypes::Type type) const
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == type)
      {
         return *it;
      }
   }
   return 0;
}

void 
ParserCategory::removeParameterByEnum(ParameterTypes::Type type)
{
   for (ParameterList::iterator it = mParameters.begin();
        it != mParameters.end(); it++)
   {
      if ((*it)->getType() == type)
      {
         delete *it;
         mParameters.erase(it);
         return;
      }
   }
}

Parameter* 
ParserCategory::getParameterByData(const Data& data) const
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if (isEqualNoCase((*it)->getName(), data))
      {
         return *it;
      }
   }
   return 0;
}

void 
ParserCategory::removeParameterByData(const Data& data)
{
   for (ParameterList::iterator it = mUnknownParameters.begin();
        it != mUnknownParameters.end(); it++)
   {
      if ((*it)->getName() == data)
      {
         delete *it;
         mParameters.erase(it);
         return;
      }
   }
}
