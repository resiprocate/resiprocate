#if !defined(RESIP_TOKEN_HXX)
#define RESIP_TOKEN_HXX 

#include <iosfwd>
#include "resiprocate/os/Data.hxx"
#include "resiprocate/ParserCategory.hxx"
#include "resiprocate/ParserContainer.hxx"

namespace resip
{


//====================
// Token:
//====================
class Token : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputCommas};

      Token();
      explicit Token(const Data& d);
      Token(HeaderFieldValue* hfv, Headers::Type type);
      Token(const Token&);
      Token& operator=(const Token&);
      bool operator==(const Token& rhs) const;
      bool operator<(const Token& rhs) const;

      Data& value() const;

      virtual void parse(ParseBuffer& pb); // remember to call parseParameters()
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;
   private:
      mutable Data mValue;
};
typedef ParserContainer<Token> Tokens;
 
}

#endif
