#ifndef ParserCategories_hxx
#define ParserCategories_hxx

#include "sip2/util/Data.hxx"
#include "sip2/sipstack/ParserCategory.hxx"
#include "sip2/sipstack/ParserContainer.hxx"
#include "sip2/sipstack/HeaderFieldValue.hxx"
#include "sip2/sipstack/MethodTypes.hxx"
#include "sip2/sipstack/Symbols.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/ParameterTypes.hxx"

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

      Token() : ParserCategory(), mValue() {}
      Token(HeaderFieldValue* hfv, Headers::Type type) : ParserCategory(hfv, type), mValue() {}
      Token(const Token&);
      Token& operator=(const Token&);
      bool operator<(const Token& rhs) const;

      Data& value() const {checkParsed(); return mValue;}

      virtual void parse(ParseBuffer& pb); // remember to call parseParameters()
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;
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

      Mime() : ParserCategory(), mType(), mSubType() {};
      Mime(const Data& type, const Data& subType);

      Mime(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type),
           mType(), 
           mSubType()
      {}
      Mime(const Mime&);
      Mime& operator=(const Mime&);
      bool operator<(const Mime& rhs) const;
      
      Data& type() const {checkParsed(); return mType;}
      Data& subType() const {checkParsed(); return mSubType;}
         
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;
   private:
      mutable Data mType;
      mutable Data mSubType;
};
typedef ParserContainer<Mime> Mimes;

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)  \
      _enum##_Param::DType& param(const _enum##_Param& paramType) const

//====================
// Auth:
//====================
class Auth : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      Auth() : ParserCategory() {}
      Auth(HeaderFieldValue* hfv, Headers::Type type) : ParserCategory(hfv, type) {}
      Auth(const Auth&);
      Auth& operator=(const Auth&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      void parseAuthParameters(ParseBuffer& pb);
      std::ostream& encodeAuthParameters(std::ostream& str) const;

      Data& scheme() const {checkParsed(); return mScheme;}

      defineParam(algorithm, "algorithm", DataParameter, "RFC ????");
      defineParam(cnonce, "cnonce", QuotedDataParameter, "RFC ????");
      defineParam(nonce, "nonce", QuotedDataParameter, "RFC ????");
      defineParam(domain, "domain", QuotedDataParameter, "RFC ????");
      defineParam(nc, "nc", DataParameter, "RFC ????");
      defineParam(opaque, "opaque", QuotedDataParameter, "RFC ????");
      defineParam(qop, "qop", <SPECIAL-CASE>, "RFC ????");
      defineParam(realm, "realm", QuotedDataParameter, "RFC ????");
      defineParam(response, "response", QuotedDataParameter, "RFC ????");
      defineParam(stale, "stale", DataParameter, "RFC ????");
      defineParam(uri, "uri", QuotedDataParameter, "RFC ????");
      defineParam(username, "username", DataParameter, "RFC ????");

      Qop_Options_Param::DType& param(const Qop_Options_Param& paramType) const;
   private:
      mutable Data mScheme;
};

#undef defineParam

//====================
// Integer:
//====================
class IntegerCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      IntegerCategory() : ParserCategory(), mValue(0), mComment() {}
      IntegerCategory(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type), 
           mValue(0), 
           mComment() 
      {}
      IntegerCategory(const IntegerCategory&);
      IntegerCategory& operator=(const IntegerCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
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
      StringCategory(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type),
           mValue()
      {}
      StringCategory(const StringCategory&);
      StringCategory& operator=(const StringCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
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
      GenericURI(HeaderFieldValue* hfv, Headers::Type type) : ParserCategory(hfv, type) {}
      GenericURI(const GenericURI&);
      GenericURI& operator=(const GenericURI&);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

      Data& uri();

   private:
      mutable Data mUri;
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
         mDisplayName()
      {}

      NameAddr(HeaderFieldValue* hfv,
               Headers::Type type)
         : ParserCategory(hfv, type), 
           mAllContacts(false),
           mDisplayName()
      {}

      explicit NameAddr(const Data& unparsed);
      explicit NameAddr(const Uri&);

      NameAddr(const NameAddr&);
      NameAddr& operator=(const NameAddr&);

      virtual ~NameAddr();
      
      Uri& uri() const;
      Data& displayName() const {checkParsed(); return mDisplayName;}
      bool isAllContacts() const {checkParsed(); return mAllContacts; }
      void setAllContacts() { mAllContacts = true;}
      
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

      bool operator<(const NameAddr& other) const;

   protected:
      bool mAllContacts;
      mutable Uri mUri;
      mutable Data mDisplayName;

   private:
#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
      //disallow the following parameters from being accessed in NameAddr
      //this works on gcc 3.2 so far. definitely does not work on gcc 2.95 on qnx
      using ParserCategory::param;
      transport_Param::DType& param(const transport_Param& paramType) const;
      method_Param::DType& param(const method_Param& paramType) const;
      ttl_Param::DType& param(const ttl_Param& paramType) const;
      maddr_Param::DType& param(const maddr_Param& paramType) const;
      lr_Param::DType& param(const lr_Param& paramType) const;
      comp_Param::DType& param(const comp_Param& paramType) const;
#endif
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
      CallId(HeaderFieldValue* hfv, 
             Headers::Type type) 
         : ParserCategory(hfv, type), mValue()
      {}
      CallId(const CallId&);
      CallId& operator=(const CallId&);
      bool operator==(const CallId&) const;
      
      Data& value() const {checkParsed(); return mValue;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

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
      
      CSeqCategory();
      CSeqCategory(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type), mMethod(UNKNOWN), mSequence(-1) 
      {}
      CSeqCategory(const CSeqCategory&);
      CSeqCategory& operator=(const CSeqCategory&);

      MethodTypes& method() const {checkParsed(); return mMethod;}
      Data& unknownMethodName() const {checkParsed(); return mUnknownMethodName;}
      int& sequence() const {checkParsed(); return mSequence;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

   private:
      mutable MethodTypes mMethod;
      mutable Data mUnknownMethodName;
      mutable int mSequence;
};

//====================
// DateCategory:
//====================

enum DayOfWeek { 
   Sun = 0,
   Mon,
   Tue,
   Wed,
   Thu,
   Fri,
   Sat
};

extern Data DayOfWeekData[];
extern Data MonthData[];

enum Month {
   Jan = 0,
   Feb,
   Mar,
   Apr,
   May,
   Jun,
   Jul,
   Aug,
   Sep,
   Oct,
   Nov,
   Dec
};

class DateCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      DateCategory();

      DateCategory(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type),
           mDayOfWeek(Sun),
           mDayOfMonth(),
           mMonth(Jan),
           mYear(0),
           mHour(0),
           mMin(0),
           mSec(0)
      {}

      DateCategory(const DateCategory&);
      DateCategory& operator=(const DateCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;

      virtual std::ostream& encodeParsed(std::ostream& str) const;
      
      static DayOfWeek DayOfWeekFromData(const Data&);
      static Month MonthFromData(const Data&);

      DayOfWeek& dayOfWeek() {checkParsed(); return mDayOfWeek;}
      int& dayOfMonth() {checkParsed(); return mDayOfMonth;}
      Month month() {checkParsed(); return mMonth;}
      int& year() {checkParsed(); return mYear;}
      int& hour() {checkParsed(); return mHour;}
      int& minute() {checkParsed(); return mMin;}
      int& second() {checkParsed(); return mSec;}

   private:
      mutable enum DayOfWeek mDayOfWeek;
      mutable int mDayOfMonth;
      mutable enum Month mMonth;
      mutable int mYear;
      mutable int mHour;
      mutable int mMin;
      mutable int mSec;
};

//====================
// WarningCategory:
//====================
class WarningCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      WarningCategory() : ParserCategory() {}
      WarningCategory(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type) 
      {}
      WarningCategory(const WarningCategory&);
      WarningCategory& operator=(const WarningCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

      int& code();
      Data& hostname();
      Data& text();

   private:
      mutable int mCode;
      mutable Data mHostname;
      mutable Data mText;
};

//====================
// Via:
//====================
class Via : public ParserCategory
{
   public:
      enum {isCommaTokenizing = true};

      Via() 
         : ParserCategory(), 
           mProtocolName(Symbols::ProtocolName),
           mProtocolVersion(Symbols::ProtocolVersion),
           mTransport(Symbols::UDP),
           mSentHost(),
           mSentPort(0) 
      {
         // insert a branch in all Vias (default constructor)
         this->param(p_branch);
      }

      Via(HeaderFieldValue* hfv, Headers::Type type) 
         : ParserCategory(hfv, type),
           mProtocolName(Symbols::ProtocolName),
           mProtocolVersion(Symbols::ProtocolVersion),
           mTransport(Symbols::UDP), // !jf! 
           mSentHost(),
           mSentPort(-1) {}
      Via(const Via&);
      Via& operator=(const Via&);

      Data& protocolName() const {checkParsed(); return mProtocolName;}
      Data& protocolVersion() const {checkParsed(); return mProtocolVersion;}
      Data& transport() const {checkParsed(); return mTransport;}
      Data& sentHost() const {checkParsed(); return mSentHost;}
      int& sentPort() const {checkParsed(); return mSentPort;}

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

//====================
// ExpiresCategory:
//====================
class ExpiresCategory : public ParserCategory
{
   public:
      enum {isCommaTokenizing = false};

      ExpiresCategory() : ParserCategory(), mValue(0) {}
      ExpiresCategory(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type), mValue(0)
      {}
      ExpiresCategory(const ExpiresCategory&);
      ExpiresCategory& operator=(const ExpiresCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      int& value() const {checkParsed(); return mValue;}

   private:
      mutable int mValue;
};
 
//====================
// RequestLine:
//====================
class RequestLine : public ParserCategory
{
   public:
      RequestLine(MethodTypes method, const Data& sipVersion = Symbols::DefaultSipVersion)
         : mMethod(method),
           mUnknownMethodName(),
           mSipVersion(sipVersion)
      {}

      RequestLine(HeaderFieldValue* hfv, Headers::Type type) 
         : ParserCategory(hfv, type),
           mMethod(UNKNOWN),
           mUnknownMethodName(MethodNames[UNKNOWN]),
           mSipVersion(Symbols::DefaultSipVersion)
      {}
      
      RequestLine(const RequestLine&);
      RequestLine& operator=(const RequestLine&);

      virtual ~RequestLine();

      Uri& uri() const;
      
      MethodTypes getMethod() const {checkParsed(); return mMethod;}
      Data& unknownMethodName() const {checkParsed(); return mUnknownMethodName;}
      const Data& getSipVersion() const {checkParsed(); return mSipVersion;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

   private:
      mutable Uri mUri;
      mutable MethodTypes mMethod;
      mutable Data mUnknownMethodName;
      mutable Data mSipVersion;
};

//====================
// StatusLine:
//====================
class StatusLine : public ParserCategory
{
   public:
      StatusLine() : 
         ParserCategory(), 
         mResponseCode(-1),
         mSipVersion(Symbols::DefaultSipVersion), 
         mReason()
      {}
      StatusLine(HeaderFieldValue* hfv, Headers::Type type)
         : ParserCategory(hfv, type), 
           mResponseCode(-1), 
           mSipVersion(Symbols::DefaultSipVersion), 
           mReason() 
      {}

      StatusLine(const StatusLine&);
      StatusLine& operator=(const StatusLine&);

      int& responseCode() const {checkParsed(); return mResponseCode;}
      const Data& getSipVersion() const {checkParsed(); return mSipVersion;}
      Data& reason() const {checkParsed(); return mReason;}

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

   private:
      mutable int mResponseCode;
      Data mSipVersion;
      mutable Data mReason;
};

}

#endif


/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */

/* Local Variables: */
/* c-file-style: "ellemtel" */
/* End: */
