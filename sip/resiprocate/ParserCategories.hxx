#ifndef ParserCategories_hxx
#define ParserCategories_hxx

#include <sipstack/ParserCategory.hxx>
#include <sipstack/ParserContainer.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/MethodTypes.hxx>

namespace Vocal2
{

class HeaderFieldValueList;

//====================
// Token:
//====================
class Token : public ParserCategory
{
   public:
      Token(HeaderFieldValue& hfv) {}
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
      Mime(HeaderFieldValueList& hfvs) {}
      Mime(HeaderFieldValue& hfv) {}
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
      Auth(HeaderFieldValueList& hfvs) {}
      Auth(HeaderFieldValue& hfv) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// Integer:
//====================
class IntegerComponent : public ParserCategory
{
public:

  IntegerComponent(HeaderFieldValueList& hfvs) {}
  IntegerComponent(HeaderFieldValue& hfv);
  ParserCategory* clone(HeaderFieldValue*) const;
  UnknownSubComponent& operator[](const Data& param)
  {
    checkParsed();
    return *mHeaderField->get(param);
  }

  virtual void parse();
  virtual std::ostream& encode(std::ostream& str) const;

  int& value();
  Data& comment();

private:
  
  int mValue;
  Data mComment;
  bool mHasComment;

};

//====================
// String:
//====================
class StringComponent : public ParserCategory
{
   public:
      StringComponent(HeaderFieldValue& hfv) {}
      virtual ParserCategory* clone(HeaderFieldValue*) const;
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
      GenericURI(HeaderFieldValueList& hfvs) {}
      GenericURI(HeaderFieldValue& hfv) {}
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
      NameAddr(HeaderFieldValueList& hfvs) {}
      NameAddr(HeaderFieldValue& hfv) {}
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
      NameAddrOrAddrSpec(HeaderFieldValueList& hfvs) {}
      NameAddrOrAddrSpec(HeaderFieldValue& hfv) {}
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
      Contact(HeaderFieldValueList& hfvs) {}
      Contact(HeaderFieldValue& hfv) {}
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
      CallId(HeaderFieldValueList& hfvs) {}
      CallId(HeaderFieldValue& hfv) {}
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

  CSeqComponent(HeaderFieldValue& hfv);
  CSeqComponent(HeaderFieldValueList& hfvs) {}
  ParserCategory* clone(HeaderFieldValue*) const;
  //  CSeqComponent(CSeqComponent &copy);

  enum MethodTypes getMethod();
  int& cSeq();

  virtual void parse();
  virtual std::ostream& encode(std::ostream& str) const;

private:
  
  enum MethodTypes mMethod;
  int mCSeq;

};

//====================
// DateComponent:
//====================
class DateComponent : public ParserCategory
{
   public:
      DateComponent(HeaderFieldValueList& hfvs) {}
      DateComponent(HeaderFieldValue& hfv) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// WarningComponent:
//====================
class WarningComponent : public ParserCategory
{
   public:
      WarningComponent(HeaderFieldValueList& hfvs) {}
      WarningComponent(HeaderFieldValue& hfv) {}
      ParserCategory* clone(HeaderFieldValue*) const;
};

//====================
// Via:
//====================
class Via : public ParserCategory
{
   public:
      Via(HeaderFieldValueList& hfvs) {}
      Via(HeaderFieldValue& hfv) {}
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
class RequestLineComponent : public ParserCategory
{
   public:
      RequestLineComponent(HeaderFieldValueList& hfvs) {}
      RequestLineComponent(HeaderFieldValue& hfv) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      MethodTypes getMethod() const;
};

//====================
// StatusLine:
//====================
class StatusLineComponent : public ParserCategory
{
   public:
      StatusLineComponent(HeaderFieldValueList& hfvs) {}
      StatusLineComponent(HeaderFieldValue& hfv) {}
      ParserCategory* clone(HeaderFieldValue*) const;
      int getResponseCode() const;
};

}



#endif
