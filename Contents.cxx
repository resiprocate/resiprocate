#include "sip2/sipstack/Contents.hxx"

using namespace Vocal2;

std::map<Mime, ContentsFactoryBase*>* Contents::FactoryMap = 0;

std::map<Mime, ContentsFactoryBase*>& 
Contents::getFactoryMap()
{
   if (Contents::FactoryMap == 0)
   {
      Contents::FactoryMap = new std::map<Mime, ContentsFactoryBase*>();
   }
   return *Contents::FactoryMap;
}
