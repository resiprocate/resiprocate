#include "sip2/sipstack/Contents.hxx"
#include "sip2/util/ParseBuffer.hxx"

using namespace Vocal2;

std::map<Mime, ContentsFactoryBase*>* Contents::FactoryMap = 0;

Contents::Contents() 
   : LazyParser()
{}

Contents::Contents(HeaderFieldValue* headerFieldValue,
                   const Mime& contentType) 
   : LazyParser(headerFieldValue),
     mContentsType(contentType)
{}

Contents::Contents( const Mime& contentType) 
   : mContentsType(contentType)
{}

Contents::Contents(const Contents& rhs) 
   : LazyParser(rhs) 
{}
  
Contents::~Contents()
{}

Contents& 
Contents::operator=(const Contents& rhs) 
{
   LazyParser::operator=(rhs); 
   mContentsType = rhs.mContentsType;
   return *this;
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

Contents*
Contents::createContents(const Mime& contentType, 
                         const char* anchor, 
                         ParseBuffer& pb)
{
   HeaderFieldValue *hfv = new HeaderFieldValue(anchor, pb.position() - anchor);
   assert(Contents::getFactoryMap().find(contentType) != Contents::getFactoryMap().end());
   Contents* c = Contents::getFactoryMap()[contentType]->create(hfv, contentType);
   c->mIsMine = true;
   return c;
}
