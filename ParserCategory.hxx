#ifndef ParserCategory_hxx
#define ParserCategory_hxx

#include <string>
#include <sip2/sipstack/ParserContainer.hxx>

namespace Vocal2
{
class HeaderFieldValueList;
class UnknownSubComponent;

class ParserCategory
{
   public:
      virtual ~ParserCategory() {}
      virtual ParserCategory* clone(HeaderFieldValue*) const = 0;

   protected:
      // call before every access 
      void checkParsed()
      {
         if (!mIsParsed)
         {
            mIsParsed = true;
            parse();
         }
      }
      virtual void parse() = 0;

      HeaderFieldValue* mHeaderField;
   private:
      bool mIsParsed;
};

//====================
// Token:
//====================
class Token : public ParserCategory
{
   public:
      Token() {};
      Token(HeaderFieldValueList& hvfs) {}
      Token(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      Mime() {};
      Mime(HeaderFieldValueList& hvfs) {}
      Mime(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      Auth() {};
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
      IntegerComponent() {};
      IntegerComponent(HeaderFieldValueList& hvfs) {}
      IntegerComponent(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      StringComponent() {};
      StringComponent(HeaderFieldValueList& hvfs) {}
      StringComponent(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// GenericUri:
//====================
class GenericURI : public ParserCategory
{
   public:
      GenericURI() {};
      GenericURI(HeaderFieldValueList& hvfs) {}
      GenericURI(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      NameAddr() {};
      NameAddr(HeaderFieldValueList& hvfs) {}
      NameAddr(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      NameAddrOrAddrSpec() {};
      NameAddrOrAddrSpec(HeaderFieldValueList& hvfs) {}
      NameAddrOrAddrSpec(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};

//====================
// Contact:
//====================
class Contact : public ParserCategory
{
   public:
      Contact() {};
      Contact(HeaderFieldValueList& hvfs) {}
      Contact(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      CallId() {};
      CallId(HeaderFieldValueList& hvfs) {}
      CallId(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
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
      CSeqComponent() {};
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
      DateComponent() {};
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
      WarningComponent() {};
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
      Via() {};
      Via(HeaderFieldValueList& hvfs) {}
      Via(HeaderFieldValue& hvf) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      UnknownSubComponent& operator[](const std::string& param)
      {
         checkParsed();
         return *mHeaderField->get(param);
      }
};
typedef ParserContainer<Via> Vias;

#endif
