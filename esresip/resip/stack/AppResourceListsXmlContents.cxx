#include "resip/stack/AppResourceListsXmlContents.hxx"

namespace resip
{
const Mime AppResourceListsXmlContents::theType("application","resource-lists+xml");
const Data AppResourceListsXmlContents::theErrorContext("AppResourceListsXmlContents");

AppResourceListsXmlContents::AppResourceListsXmlContents(std::vector<Data>& urls) :
   Contents(theType),
   mUrls(urls)
{}

AppResourceListsXmlContents::~AppResourceListsXmlContents()
{}

Contents* 
AppResourceListsXmlContents::clone() const 
{
   return new AppResourceListsXmlContents(*this);
}

const Mime& 
AppResourceListsXmlContents::getStaticType()
{
   return theType;
}

std::ostream& 
AppResourceListsXmlContents::encodeParsed(std::ostream& str) const
{
   static Data prefix("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                       " <resource-lists xmlns=\"urn:ietf:params:xml:ns:resource-lists\">\n"
                       "  <list>\n");
   static Data postFix("  </list>\n"
                        " </resource-lists>");

   str << prefix;

   for(std::vector<Data>::const_iterator i=mUrls.begin(); i!=mUrls.end(); ++i)
   {
      static Data entryPrefix("  <entry uri=\"");
      static Data entryPostfix("\" />");
      str << entryPrefix << *i << entryPostfix;
   }

   return str << postFix;
}

void 
AppResourceListsXmlContents::parse(ParseBuffer& pb)
{
   // !bwc! Write this code.
   assert(0);
}

const Data& 
AppResourceListsXmlContents::errorContext() const
{
   return theErrorContext;
}

} // namespace resip
