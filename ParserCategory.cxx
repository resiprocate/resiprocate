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

// !dlb! need to convert existing parameter by enum to UnknownParameter for backward compatibility
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
ParserCategory::getParameterByEnum(int type) const
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
ParserCategory::removeParameterByEnum(int type)
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

Transport_Param::Type::Type& 
ParserCategory::param(const Transport_Param& paramType) const
{
   checkParsed();
   Transport_Param::Type* p = dynamic_cast<Transport_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Transport_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

User_Param::Type::Type& 
ParserCategory::param(const User_Param& paramType) const
{
   checkParsed();
   User_Param::Type* p = dynamic_cast<User_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new User_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Method_Param::Type::Type& 
ParserCategory::param(const Method_Param& paramType) const
{
   checkParsed();
   Method_Param::Type* p = dynamic_cast<Method_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Method_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Ttl_Param::Type::Type& 
ParserCategory::param(const Ttl_Param& paramType) const
{
   checkParsed();
   Ttl_Param::Type* p = dynamic_cast<Ttl_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Ttl_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Maddr_Param::Type::Type& 
ParserCategory::param(const Maddr_Param& paramType) const
{
   checkParsed();
   Maddr_Param::Type* p = dynamic_cast<Maddr_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Maddr_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Lr_Param::Type::Type& 
ParserCategory::param(const Lr_Param& paramType) const
{
   checkParsed();
   Lr_Param::Type* p = dynamic_cast<Lr_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Lr_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Q_Param::Type::Type& 
ParserCategory::param(const Q_Param& paramType) const
{
   checkParsed();
   Q_Param::Type* p = dynamic_cast<Q_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Q_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Purpose_Param::Type::Type& 
ParserCategory::param(const Purpose_Param& paramType) const
{
   checkParsed();
   Purpose_Param::Type* p = dynamic_cast<Purpose_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Purpose_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Expires_Param::Type::Type& 
ParserCategory::param(const Expires_Param& paramType) const
{
   checkParsed();
   Expires_Param::Type* p = dynamic_cast<Expires_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Expires_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Handling_Param::Type::Type& 
ParserCategory::param(const Handling_Param& paramType) const
{
   checkParsed();
   Handling_Param::Type* p = dynamic_cast<Handling_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Handling_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Tag_Param::Type::Type& 
ParserCategory::param(const Tag_Param& paramType) const
{
   checkParsed();
   Tag_Param::Type* p = dynamic_cast<Tag_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Tag_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

ToTag_Param::Type::Type& 
ParserCategory::param(const ToTag_Param& paramType) const
{
   checkParsed();
   ToTag_Param::Type* p = dynamic_cast<ToTag_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new ToTag_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

FromTag_Param::Type::Type& 
ParserCategory::param(const FromTag_Param& paramType) const
{
   checkParsed();
   FromTag_Param::Type* p = dynamic_cast<FromTag_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new FromTag_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Duration_Param::Type::Type& 
ParserCategory::param(const Duration_Param& paramType) const
{
   checkParsed();
   Duration_Param::Type* p = dynamic_cast<Duration_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Duration_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Branch_Param::Type::Type& 
ParserCategory::param(const Branch_Param& paramType) const
{
   checkParsed();
   Branch_Param::Type* p = dynamic_cast<Branch_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Branch_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Received_Param::Type::Type& 
ParserCategory::param(const Received_Param& paramType) const
{
   checkParsed();
   Received_Param::Type* p = dynamic_cast<Received_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Received_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Comp_Param::Type::Type& 
ParserCategory::param(const Comp_Param& paramType) const
{
   checkParsed();
   Comp_Param::Type* p = dynamic_cast<Comp_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Comp_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}

Rport_Param::Type::Type& 
ParserCategory::param(const Rport_Param& paramType) const
{
   checkParsed();
   Rport_Param::Type* p = dynamic_cast<Rport_Param::Type*>(getParameterByEnum(paramType.getTypeNum()));
   if (!p)
   {
      p = new Rport_Param::Type(paramType.getTypeNum());
      mParameters.push_back(p);
   }
   return p->value();
}
