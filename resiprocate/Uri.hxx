#ifndef Uri_hxx
#define Uri_hxx

#include <sipstack/SipMessage.hxx>
#include <sipstack/ParserCategory.hxx>

namespace Vocal2
{

class Uri : public ParserCategory, public SipMessage
{
   public:
      Uri() : 
         ParserCategory(),
         SipMessage()
      {}

      Uri(const Uri&);
      
      Data& host() const {checkParsed(); return mHost;}
      Data& user() const {checkParsed(); return mUser;}
      const Data& getAor() const;
      Data& scheme() const {checkParsed(); return mScheme;}
      int& port() const {checkParsed(); return mPort;}
      Data& password() const {checkParsed(); return mPassword;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

      // parse the headers into this as SipMessage -- called from parse
      void parseEmbeddedHeaders();

      bool operator<(const Uri& other) const;
      
   protected:
      mutable Data mScheme;
      mutable Data mHost;
      mutable Data mUser;
      mutable Data mAor;
      mutable int mPort;
      mutable Data mPassword;
};
 
}

#endif
