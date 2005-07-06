#if !defined(RESIP_VIA_HXX)
#define RESIP_VIA_HXX 

#include <iosfwd>
#include "resiprocate/os/Data.hxx"
#include "resiprocate/ParserCategory.hxx"
#include "resiprocate/ParserContainer.hxx"

namespace resip
{

//====================
// Via:
//====================
class Via : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputMulti};

      Via();
      Via(HeaderFieldValue* hfv, Headers::Type type);
      Via(const Via&);
      Via& operator=(const Via&);

      Data& protocolName();
      const Data& protocolName() const;
      Data& protocolVersion();
      const Data& protocolVersion() const;
      Data& transport();
      const Data& transport() const;
      Data& sentHost();
      const Data& sentHost() const;
      int& sentPort();
      int sentPort() const;

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

   private:
      mutable Data mProtocolName;
      mutable Data mProtocolVersion;
      mutable Data mTransport;
      mutable Data mSentHost;
      mutable int mSentPort;
};
typedef ParserContainer<Via> Vias;

 
}


#endif
