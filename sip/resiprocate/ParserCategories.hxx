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
class Uri;

//====================
// Token:
//====================
class Token : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      Token(): ParserCategory(), mValue() {}
      Token(HeaderFieldValue* hfv) : ParserCategory(hfv) {}
      Token(const Token&);

      Data& value() const {checkParsed(); return mValue;}

      virtual void parse(ParseBuffer& pb); // remember to call parseParameters()
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

      Mime() : ParserCategory(), mType(), mSubType() {}
      Mime(HeaderFieldValue* hfv) : ParserCategory(hfv), mType(), mSubType() {}
      Mime(const Mime&);
      
      Data& type() const { return mType; }
      Data& subType() const { return mSubType; }
         
      virtual void parse(ParseBuffer& pb);
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

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone() const;
};

//====================
// Integer:
//====================
class IntegerCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      IntegerCategory() : ParserCategory(), mValue(0), mComment() {}
      IntegerCategory(HeaderFieldValue* hfv) : ParserCategory(hfv), mValue(0), mComment() {}
      IntegerCategory(const IntegerCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      int& value() const {checkParsed(); return mValue;}
      Data& comment() const {checkParsed(); return mComment;}

   private:
      mutable int mValue;
      mutable Data mComment;
};

//====================
// StringCategory
//====================
class StringCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};
      
      StringCategory() : ParserCategory(), mValue() {}
      StringCategory(const StringCategory&);
      StringCategory(HeaderFieldValue* hfv) : ParserCategory(hfv), mValue() {}

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encode(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      Data& value() const {checkParsed(); return mValue;}

   private:
      mutable Data mValue;
};
typedef ParserContainer<StringCategory> StringCategories;

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

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
};
typedef ParserContainer<GenericURI> GenericURIs;

//====================
// NameAddr:
//====================
class NameAddr : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      NameAddr() : 
         ParserCategory(),
         mAllContacts(false),
         mUri(0),
         mDisplayName()
      {}

      NameAddr(HeaderFieldValue* hfv)
         : ParserCategory(hfv), 
           mAllContacts(false),
           mUri(0),
           mDisplayName()
      {}

      NameAddr(const NameAddr&);
      NameAddr(const Uri&);

      virtual ~NameAddr();
      
      Uri& uri() const {checkParsed(); return *mUri;}
      Data& displayName() const {checkParsed(); return mDisplayName;}
      void setAllContacts() { mAllContacts = true;}
      
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

      bool operator<(const NameAddr& other) const;
      
   protected:
      bool mAllContacts;
      mutable Uri* mUri;
      mutable Data mDisplayName;
};
typedef ParserContainer<NameAddr> NameAddrs;

//====================
// CallId:
//====================
class CallId : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      CallId() : ParserCategory(), mValue() {}
      CallId(HeaderFieldValue* hfv) : ParserCategory(hfv), mValue() {}
      CallId(const CallId&);
      
      Data& value() const {checkParsed(); return mValue;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable Data mValue;
};
typedef ParserContainer<CallId> CallIds;

//====================
// CSeqCategory:
//====================
class CSeqCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};
      
      CSeqCategory() : ParserCategory(), mMethod(UNKNOWN), mSequence(-1) {}
      CSeqCategory(HeaderFieldValue* hfv) : ParserCategory(hfv), mMethod(UNKNOWN), mSequence(-1) {}
      CSeqCategory(const CSeqCategory&);

      MethodTypes& method() const {checkParsed(); return mMethod;}
      int& sequence() const {checkParsed(); return mSequence;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable MethodTypes mMethod;
      mutable int mSequence;
};

//====================
// DateCategory:
//====================
class DateCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      DateCategory() : ParserCategory(), mValue() {}
      DateCategory(HeaderFieldValue* hfv) : ParserCategory(hfv), mValue() {}
      DateCategory(const DateCategory&);

      Data& value() const {checkParsed(); return mValue;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable Data mValue;
};

//====================
// WarningCategory:
//====================
class WarningCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      WarningCategory() : ParserCategory() {}
      WarningCategory(HeaderFieldValue* hfv) : ParserCategory(hfv) {}
      WarningCategory(const WarningCategory&);

      virtual void parse(ParseBuffer& pb);
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

      Via() : ParserCategory(), mProtocolName(),mProtocolVersion(),mTransport(),mSentHost(),mSentPort(-1) {}
      Via(HeaderFieldValue* hfv) : ParserCategory(hfv),mProtocolName(),mProtocolVersion(),mTransport(),mSentHost(),mSentPort(-1) {}
      Via(const Via&);

      Data& protocolName() const {checkParsed(); return mProtocolName;}
      Data& protocolVersion() const {checkParsed(); return mProtocolVersion;}
      Data& transport() const {checkParsed(); return mTransport;}
      Data& sentHost() const {checkParsed(); return mSentHost;}
      int& sentPort() const {checkParsed(); return mSentPort;}

      virtual void parse(ParseBuffer& pb);
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
class RequestLine : public ParserCategory
{
   public:
      RequestLine(MethodTypes method, const Data& sipVersion = Symbols::DefaultSipVersion)
         : mUri(0),
           mMethod(method),
           mSipVersion(sipVersion)
      {}
      RequestLine(HeaderFieldValue* hfv) : ParserCategory(hfv), mUri(0), mMethod(UNKNOWN), mSipVersion() {}
      RequestLine(const RequestLine&);

      virtual ~RequestLine();

      Uri& uri() const {checkParsed(); return *mUri;}
      MethodTypes getMethod() const {checkParsed(); return mMethod;}
      const Data& getSipVersion() const {checkParsed(); return mSipVersion;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      Uri* mUri;
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
      StatusLine(HeaderFieldValue* hfv) : ParserCategory(hfv), mResponseCode(-1), mSipVersion(), mReason() {}
      StatusLine(const StatusLine&);

      int& responseCode() const {checkParsed(); return mResponseCode;}
      const Data& getSipVersion() const {checkParsed(); return mSipVersion;}
      Data& reason() const {checkParsed(); return mReason;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encode(std::ostream& str) const;

   private:
      mutable int mResponseCode;
      Data mSipVersion;
      mutable Data mReason;
};

}

#endif
