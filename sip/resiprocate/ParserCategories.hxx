#ifndef ParserCategories_hxx
#define ParserCategories_hxx

#include <sipstack/ParserCategory.hxx>
#include <sipstack/ParserContainer.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/MethodTypes.hxx>
#include <sipstack/Data.hxx>

// !dlb! copy constructur, assignment operator
// firgure out the clone mess -- these clones should be blissfully ignorant
namespace Vocal2
{

class HeaderFieldValueList;

//====================
// Token:
//====================
class Token : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      Token(): ParserCategory() {}
      Token(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      Data& value() {checkParsed(); return mValue;}

      virtual void parse(); // remember to call parseParameters()
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      Data mValue;
};
typedef ParserContainer<Token> Tokens;

//====================
// Mime:
//====================
class Mime : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      Mime() : ParserCategory() {}
      Mime(HeaderFieldValue* hfv) : ParserCategory(hfv) {}
      Data& type() { return mType; }
      Data& subType() { return mSubType; }
         
      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;
   private:
      Data mType;
      Data mSubType;
};
typedef ParserContainer<Mime> Mimes;

//====================
// Auth:
//====================
class Auth : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      Auth() : ParserCategory() {}
      Auth(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// Integer:
//====================
class IntegerComponent : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      IntegerComponent() : ParserCategory() {}
      IntegerComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone(HeaderFieldValue*) const;

      int& value() {checkParsed(); return mValue;}
      Data& comment() {checkParsed(); return mComment;}

   private:
      int mValue;
      Data mComment;
};

//====================
// String:
//====================
class StringComponent : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};
      
      StringComponent() : ParserCategory() {}
      StringComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone(HeaderFieldValue*) const;

      Data& value() {checkParsed(); return mValue;}

   private:
      Data mValue;
};
typedef ParserContainer<StringComponent> StringComponents;

//====================
// GenericUri:
//====================
class GenericURI : public ParserCategory
{
   public:
      GenericURI() : ParserCategory() {}
      GenericURI(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
};
typedef ParserContainer<GenericURI> GenericURIs;

class URI : public ParserCategory
{
   public:
      URI() : ParserCategory() {}
      URI(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      Data& host() {checkParsed(); return mHost;}
      Data& user() {checkParsed(); return mUser;}
      const Data& getAor();
      Data& scheme() {checkParsed(); return mScheme;}
      int& port() {checkParsed(); return mPort;}
      Data& password() {checkParsed(); return mPassword;}
      
      virtual ParserCategory* clone(HeaderFieldValue*) const;

   protected:
      Data mScheme;
      Data mHost;
      Data mUser;
      Data aor;
      int mPort;
      Data mPassword;
};

class NameAddrBase : public URI
{
   public:
      NameAddrBase() : URI() {}
      NameAddrBase(HeaderFieldValue* hfv) : URI(hfv) {}

      Data& displayName() {checkParsed(); return mDisplayName;} 
      
      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;

   protected:
      Data mDisplayName;
};

//====================
// NameAddr:
//====================
class NameAddr : public NameAddrBase
{
   public:
      enum {isCommaTokenizing = true};

      NameAddr() : NameAddrBase() {}
      NameAddr(HeaderFieldValue* hfv) : NameAddrBase(hfv) {}

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
};
typedef ParserContainer<NameAddr> NameAddrs;

//====================
// NameAddrOrAddrSpec:
//====================
class NameAddrOrAddrSpec : public NameAddrBase
{
   public:
      enum {isCommaTokenizing = false};

      NameAddrOrAddrSpec() : NameAddrBase() {}
      NameAddrOrAddrSpec(HeaderFieldValue* hfv) : NameAddrBase(hfv) {}

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
};

//====================
// Contact:
//====================
// ?dlb? until shown otherwise, looks the same?
class Contact : public NameAddr
{
   public:
      Contact() : NameAddr() {}
      Contact(HeaderFieldValue* hfv) : NameAddr(hfv) {}
};
typedef ParserContainer<Contact> Contacts;

//====================
// CallId:
//====================
class CallId : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      CallId() : ParserCategory() {}
      CallId(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      Data& value() {checkParsed(); return mValue;}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      Data mValue;
};
typedef ParserContainer<CallId> CallIds;

//====================
// CSeqComponent:
//====================
class CSeqComponent : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};
      
      CSeqComponent() : ParserCategory() {}
      CSeqComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      MethodTypes& method() {checkParsed(); return mMethod;}
      int& sequence() {checkParsed(); return mSequence;}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      MethodTypes mMethod;
      int mSequence;
};

//====================
// DateComponent:
//====================
class DateComponent : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      DateComponent() : ParserCategory() {}
      DateComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      Data& value() {checkParsed(); return mValue;}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      Data mValue;
};

//====================
// WarningComponent:
//====================
class WarningComponent : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      WarningComponent() : ParserCategory() {}
      WarningComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;
};

//====================
// Via:
//====================
class Via : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      Via() : ParserCategory() {}
      Via(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      Data& protocolName() {checkParsed(); return mProtocolName;}
      Data& protocolVersion() {checkParsed(); return mProtocolVersion;}
      Data& transport() {checkParsed(); return mTransport;}
      Data& sentHost() {checkParsed(); return mSentHost;}
      int& sentPort() {checkParsed(); return mSentPort;}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      Data mProtocolName;
      Data mProtocolVersion;
      Data mTransport;
      Data mSentHost;
      int mSentPort;
};
typedef ParserContainer<Via> Vias;
 
//====================
// RequestLine:
//====================
static Data DefaultSipVersion;

class RequestLineComponent : public URI
{
   public:
      RequestLineComponent(MethodTypes method, const Data& sipVersion = DefaultSipVersion) : 
         URI(),
         mMethod(method),
         mSipVersion(sipVersion)
      {}
      RequestLineComponent(HeaderFieldValue* hfv) : URI(hfv) {}

      MethodTypes getMethod() {checkParsed(); return mMethod;}
      const Data& getSipVersion() {checkParsed(); return mSipVersion;}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      MethodTypes mMethod;
      Data mSipVersion;
};

//====================
// StatusLine:
//====================
class StatusLineComponent : public ParserCategory
{
   public:
      StatusLineComponent() : ParserCategory() {}
      StatusLineComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      int responseCode() {checkParsed(); return mResponseCode;}
      const Data& getSipVersion() {checkParsed(); return mSipVersion;}
      Data& reason() {checkParsed(); return mReason;}

      virtual void parse();
      virtual ParserCategory* clone(HeaderFieldValue*) const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      int mResponseCode;
      Data mSipVersion;
      Data mReason;
};

}

#endif
