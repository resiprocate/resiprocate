#ifndef ParserCategories_hxx
#define ParserCategories_hxx

#include <sipstack/ParserCategory.hxx>
#include <sipstack/ParserContainer.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/MethodTypes.hxx>
#include <sipstack/Symbols.hxx>

#include <util/Data.hxx>

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
      Token(const Token&);

      Data& value() const {checkParsed(); return mValue;}

      virtual void parse(); // remember to call parseParameters()
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable Data mValue;
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
      Mime(const Mime&);
      
      Data& type() const { return mType; }
      Data& subType() const { return mSubType; }
         
      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;
   private:
      mutable Data mType;
      mutable Data mSubType;
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
      Auth(const Auth&);

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone() const;
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
      IntegerComponent(const IntegerComponent&);

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      int& value() const {checkParsed(); return mValue;}
      Data& comment() const {checkParsed(); return mComment;}

   private:
      mutable int mValue;
      mutable Data mComment;
};

//====================
// StringComponent
//====================
class StringComponent : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};
      
      StringComponent() : ParserCategory() {}
      StringComponent(const StringComponent&);
      StringComponent(HeaderFieldValue* hfv) : ParserCategory(hfv) {}

      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      Data& value() const {checkParsed(); return mValue;}

   private:
      mutable Data mValue;
};
typedef ParserContainer<StringComponent> StringComponents;

//====================
// GenericUri:
//====================
class GenericURI : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      GenericURI() : ParserCategory() {}
      GenericURI(HeaderFieldValue* hfv) : ParserCategory(hfv) {}
      GenericURI(const GenericURI&);

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
};
typedef ParserContainer<GenericURI> GenericURIs;

//====================
// Url:
//====================
class Url : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      Url() : 
         ParserCategory(),
         mAllContacts(false)
      {}
      Url(HeaderFieldValue* hfv)
         : ParserCategory(hfv), 
           mAllContacts(false)
      {}
      Url(const Url&);

      Data& host() const {checkParsed(); return mHost;}
      Data& user() const {checkParsed(); return mUser;}
      Data& displayName() const {checkParsed(); return mDisplayName;}
      const Data& getAor() const;
      Data& scheme() const {checkParsed(); return mScheme;}
      int& port() const {checkParsed(); return mPort;}
      Data& password() const {checkParsed(); return mPassword;}

      void setAllContacts() { mAllContacts = true;}
      
      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

      bool operator<(Url& other);
      
   protected:
      bool mAllContacts;
      mutable Data mScheme;
      mutable Data mHost;
      mutable Data mUser;
      Data aor;
      mutable int mPort;
      mutable Data mPassword;
      mutable Data mDisplayName;
};

typedef ParserContainer<Url> Urls;

//====================
// CallId:
//====================
class CallId : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      CallId() : ParserCategory() {}
      CallId(HeaderFieldValue* hfv) : ParserCategory(hfv) {}
      CallId(const CallId&);
      
      Data& value() const {checkParsed(); return mValue;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable Data mValue;
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
      CSeqComponent(const CSeqComponent&);

      MethodTypes& method() const {checkParsed(); return mMethod;}
      int& sequence() const {checkParsed(); return mSequence;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable MethodTypes mMethod;
      mutable int mSequence;
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
      DateComponent(const DateComponent&);

      Data& value() const {checkParsed(); return mValue;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable Data mValue;
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
      WarningComponent(const WarningComponent&);

      virtual void parse();
      virtual ParserCategory* clone() const;
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
      Via(const Via&);

      Data& protocolName() const {checkParsed(); return mProtocolName;}
      Data& protocolVersion() const {checkParsed(); return mProtocolVersion;}
      Data& transport() const {checkParsed(); return mTransport;}
      Data& sentHost() const {checkParsed(); return mSentHost;}
      int& sentPort() const {checkParsed(); return mSentPort;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable Data mProtocolName;
      mutable Data mProtocolVersion;
      mutable Data mTransport;
      mutable Data mSentHost;
      mutable int mSentPort;
};
typedef ParserContainer<Via> Vias;
 
//====================
// RequestLine:
//====================
class RequestLine : public Url
{
   public:
      RequestLine(MethodTypes method, const Data& sipVersion = Symbols::DefaultSipVersion)
         : Url(),
           mMethod(method),
           mSipVersion(sipVersion)
      {}
      RequestLine(HeaderFieldValue* hfv) : Url(hfv) {}
      RequestLine(const RequestLine&);

      MethodTypes getMethod() const {checkParsed(); return mMethod;}
      const Data& getSipVersion() const {checkParsed(); return mSipVersion;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      MethodTypes mMethod;
      Data mSipVersion;
};

//====================
// StatusLine:
//====================
class StatusLine : public ParserCategory
{
   public:
      StatusLine() : ParserCategory() {}
      StatusLine(HeaderFieldValue* hfv) : ParserCategory(hfv) {}
      StatusLine(const StatusLine&);

      int& responseCode() const {checkParsed(); return mResponseCode;}
      const Data& getSipVersion() const {checkParsed(); return mSipVersion;}
      Data& reason() const {checkParsed(); return mReason;}

      virtual void parse();
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable int mResponseCode;
      Data mSipVersion;
      mutable Data mReason;
};

}

#endif
