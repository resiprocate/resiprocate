#include "sip2/sipstack/Contents.hxx"

using namespace Vocal2;

std::map<Mime, ContentsFactoryBase*>* Contents::FactoryMap = 0;

Contents::Contents() 
   : LazyParser()
{}

Contents::Contents(HeaderFieldValue* headerFieldValue) 
   : LazyParser(headerFieldValue) 
{}

Contents::Contents(const Contents& rhs) 
   : LazyParser(rhs) 
{}
  
Contents::~Contents()
{}

Contents& 
Contents::operator=(const Contents& rhs) 
{
   LazyParser::operator=(rhs); return *this;
}

std::map<Mime, ContentsFactoryBase*>& 
Contents::getFactoryMap()
{
   if (Contents::FactoryMap == 0)
   {
      Contents::FactoryMap = new std::map<Mime, ContentsFactoryBase*>();
   }
   return *Contents::FactoryMap;
}

Contents*
Contents::getContents(const Mime& m)
{
   assert(Contents::getFactoryMap().find(m) != Contents::getFactoryMap().end());
   return Contents::getFactoryMap()[m]->convert(getContents());
}
