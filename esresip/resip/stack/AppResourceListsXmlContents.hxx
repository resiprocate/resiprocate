#ifndef AppResourceListsXmlContents_Include_Guard
#define AppResourceListsXmlContents_Include_Guard

#include "resip/stack/Contents.hxx"
namespace resip
{

class AppResourceListsXmlContents : public Contents
{
   public:
     explicit AppResourceListsXmlContents(std::vector<Data>& urls);
     virtual ~AppResourceListsXmlContents();

/////////////////// Must implement unless abstract ///

      virtual Contents* clone() const ;
      const Mime& getStaticType();

      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual void parse(ParseBuffer& pb);

/////////////////// May override ///

      virtual const Data& errorContext() const;

   protected:
      std::vector<Data> mUrls;

   private:
      static const Mime theType;
      static const Data theErrorContext;
}; // class AppResourceListsXmlContents

} // namespace resip

#endif // include guard
