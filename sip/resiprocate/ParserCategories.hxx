#ifndef ParserCategories_hxx
#define ParserCategories_hxx

#include <sipstack/ParserCategory.hxx>
#include <sipstack/ParserContainer.hxx>
#include <sipstack/HeaderFieldValue.hxx>

namespace Vocal2
{

class HeaderFieldValueList;

//====================
// Token:
//====================
class Token : public ParserCategory
{
   public:
      Token(HeaderFieldValueList& hvfs) {}
      Token(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<Token> Tokens;

//====================
// Mime:
//====================
class Mime : public ParserCategory
{
   public:
      Mime(HeaderFieldValueList& hvfs) {}
      Mime(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<Mime> Mimes;

//====================
// Auth:
//====================
class Auth : public ParserCategory
{
   public:
      Auth(HeaderFieldValueList& hvfs) {}
      Auth(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// Integer:
//====================
class IntegerComponent : public ParserCategory
{
   public:
      IntegerComponent(HeaderFieldValueList& hvfs) {}
      IntegerComponent(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};

//====================
// String:
//====================
class StringComponent : public ParserCategory
{
   public:
      StringComponent(HeaderFieldValueList& hvfs) {}
      StringComponent(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      virtual void parse();
      virtual std::ostream& encode(std::ostream& str) const;
      Data& value();
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
      GenericURI(HeaderFieldValueList& hvfs) {}
      GenericURI(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<GenericURI> GenericURIs;

//====================
// NameAddrs:
//====================
class NameAddr : public ParserCategory
{
   public:
      NameAddr(HeaderFieldValueList& hvfs) {}
      NameAddr(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<NameAddr> NameAddrs;

//====================
// NameAddrOrAddrSpec:
//====================
class NameAddrOrAddrSpec : public ParserCategory
{
   public:
      NameAddrOrAddrSpec(HeaderFieldValueList& hvfs) {}
      NameAddrOrAddrSpec(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
      virtual void parse() {assert(0);}
};

//====================
// Contact:
//====================
class Contact : public ParserCategory
{
   public:
      Contact(HeaderFieldValueList& hvfs) {}
      Contact(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<Contact> Contacts;

//====================
// CallId:
//====================
class CallId : public ParserCategory
{
   public:
      CallId(HeaderFieldValueList& hvfs) {}
      CallId(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<CallId> CallIds;

//====================
// CSeqComponent:
//====================
class CSeqComponent : public ParserCategory
{
   public:
      CSeqComponent(HeaderFieldValue& hvf) {}
      CSeqComponent(HeaderFieldValueList& hvfs) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// DateComponent:
//====================
class DateComponent : public ParserCategory
{
   public:
      DateComponent(HeaderFieldValueList& hvfs) {}
      DateComponent(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// WarningComponent:
//====================
class WarningComponent : public ParserCategory
{
   public:
      WarningComponent(HeaderFieldValueList& hvfs) {}
      WarningComponent(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// Via:
//====================
class Via : public ParserCategory
{
   public:
      Via(HeaderFieldValueList& hvfs) {}
      Via(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const Data& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<Via> Vias;
 
//====================
// RequestLine:
//====================
class RequestLine : public ParserCategory
{
   public:
      RequestLine(HeaderFieldValueList& hvfs) {}
      RequestLine(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
//    CommandType getMethod() const;
};

//====================
// StatusLine:
//====================
class StatusLine : public ParserCategory
{
   public:
      StatusLine(HeaderFieldValueList& hvfs) {}
      StatusLine(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      int getResponseCode() const;
};

}



#endif
