#if !defined(RESIP_PARSERCATEGORIES_HXX)
#define RESIP_PARSERCATEGORIES_HXX 

#include "resiprocate/os/Data.hxx"
#include "resiprocate/ParserCategory.hxx"
#include "resiprocate/ParserContainer.hxx"
#include "resiprocate/HeaderFieldValue.hxx"
#include "resiprocate/MethodTypes.hxx"
#include "resiprocate/Symbols.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/ParameterTypes.hxx"

namespace resip
{

class HeaderFieldValueList;

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

//====================
// Mime:
//====================
class Mime : public ParserCategory
{
   public:
      enum {commaHandling = CommasAllowedOutputCommas};

      Mime();
      Mime(const Data& type, const Data& subType);
      Mime(HeaderFieldValue* hfv, Headers::Type type);
      Mime(const Mime&);
      Mime& operator=(const Mime&);
      bool operator<(const Mime& rhs) const;
      bool operator==(const Mime& rhs) const;
      
      Data& type() const;
      Data& subType() const;
         
      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;
   private:
      mutable Data mType;
      mutable Data mSubType;
};
typedef ParserContainer<Mime> Mimes;

#define defineParam(_enum, _name, _type, _RFC_ref_ignored)  \
      _enum##_Param::DType& param(const _enum##_Param& paramType); \
      const _enum##_Param::DType& param(const _enum##_Param& paramType) const

//====================
// Auth:
//====================
class Auth : public ParserCategory
{
   public:
      enum {commaHandling = NoCommaTokenizing};

      Auth();
      Auth(HeaderFieldValue* hfv, Headers::Type type);
      Auth(const Auth&);
      Auth& operator=(const Auth&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      void parseAuthParameters(ParseBuffer& pb);
      std::ostream& encodeAuthParameters(std::ostream& str) const;

      Data& scheme();
      const Data& scheme() const;

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
      defineParam(username, "username", QuotedDataParameter, "RFC ????");

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
      enum {commaHandling = NoCommaTokenizing};

      IntegerCategory();
      IntegerCategory(HeaderFieldValue* hfv, Headers::Type type);
      IntegerCategory(const IntegerCategory&);
      IntegerCategory& operator=(const IntegerCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      int& value() const;
      Data& comment() const;

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
      enum {commaHandling = NoCommaTokenizing};

      StringCategory();
      explicit StringCategory(const Data& value);
      StringCategory(HeaderFieldValue* hfv, Headers::Type type);
      StringCategory(const StringCategory&);
      StringCategory& operator=(const StringCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      Data& value() const;

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
      enum {commaHandling = NoCommaTokenizing};

      GenericURI() : ParserCategory() {}
      GenericURI(HeaderFieldValue* hfv, Headers::Type type);
      GenericURI(const GenericURI&);
      GenericURI& operator=(const GenericURI&);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

      Data& uri();
      const Data& uri() const;

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
      enum {commaHandling = CommasAllowedOutputMulti};

      NameAddr();
      NameAddr(HeaderFieldValue* hfv, Headers::Type type);
      explicit NameAddr(const Uri&);
      explicit NameAddr(const Data& unparsed);

      NameAddr(const NameAddr&);
      NameAddr& operator=(const NameAddr&);

      virtual ~NameAddr();
      
      Uri& uri();
      const Uri& uri() const;
      Data& displayName();
      const Data& displayName() const;
      bool isAllContacts() const;
      void setAllContacts();
      
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
// CallID:
//====================
class CallID : public ParserCategory
{
   public:
      enum {commaHandling = NoCommaTokenizing};

      CallID();
      CallID(HeaderFieldValue* hfv, Headers::Type type);
      CallID(const CallID&);
      CallID& operator=(const CallID&);
      bool operator==(const CallID&) const;
      
      Data& value();
      const Data& value() const;

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

   private:
      mutable Data mValue;
};
typedef ParserContainer<CallID> CallIDs;
typedef CallID CallId; // code convention compatible

//====================
// CSeqCategory:
//====================
class CSeqCategory : public ParserCategory
{
   public:
      enum {commaHandling = NoCommaTokenizing};
      
      CSeqCategory();
      CSeqCategory(HeaderFieldValue* hfv, Headers::Type type);
      CSeqCategory(const CSeqCategory&);
      CSeqCategory& operator=(const CSeqCategory&);

      MethodTypes& method();
      MethodTypes method() const;
      Data& unknownMethodName();
      const Data& unknownMethodName() const;
      int& sequence();
      int sequence() const;

      bool operator==(const CSeqCategory& rhs) const;

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
      enum {commaHandling = NoCommaTokenizing};

      DateCategory();
      DateCategory(HeaderFieldValue* hfv, Headers::Type type);
      DateCategory(const DateCategory&);
      DateCategory& operator=(const DateCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;

      virtual std::ostream& encodeParsed(std::ostream& str) const;
      
      static DayOfWeek DayOfWeekFromData(const Data&);
      static Month MonthFromData(const Data&);

      const DayOfWeek& dayOfWeek() const;
      int& dayOfMonth();
      int dayOfMonth() const;
      Month& month();
      Month month() const;
      int& year();
      int year() const;
      int& hour();
      int hour() const;
      int& minute();
      int minute() const;
      int& second();
      int second() const;

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
      enum {commaHandling = CommasAllowedOutputCommas};

      WarningCategory();
      WarningCategory(HeaderFieldValue* hfv, Headers::Type type);
      WarningCategory(const WarningCategory&);
      WarningCategory& operator=(const WarningCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual ParserCategory* clone() const;
      virtual std::ostream& encodeParsed(std::ostream& str) const;

      int& code();
      int code() const;
      Data& hostname();
      const Data& hostname() const;
      Data& text();
      const Data& text() const;

   private:
      mutable int mCode;
      mutable Data mHostname;
      mutable Data mText;
};
typedef ParserContainer<WarningCategory> WarningCategories;

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

//====================
// ExpiresCategory:
//====================
class ExpiresCategory : public ParserCategory
{
   public:
      enum {commaHandling = NoCommaTokenizing};

      ExpiresCategory();
      ExpiresCategory(HeaderFieldValue* hfv, Headers::Type type);
      ExpiresCategory(const ExpiresCategory&);
      ExpiresCategory& operator=(const ExpiresCategory&);

      virtual void parse(ParseBuffer& pb);
      virtual std::ostream& encodeParsed(std::ostream& str) const;
      virtual ParserCategory* clone() const;

      int& value();
      int value() const;

   private:
      mutable int mValue;
};
 
//====================
// RequestLine:
//====================
class RequestLine : public ParserCategory
{
   public:
      RequestLine(MethodTypes method, const Data& sipVersion = Symbols::DefaultSipVersion);
      RequestLine(HeaderFieldValue* hfv, Headers::Type type);
      RequestLine(const RequestLine&);
      RequestLine& operator=(const RequestLine&);

      virtual ~RequestLine();

      const Uri& uri() const;
      Uri& uri();

      MethodTypes getMethod() const;
      MethodTypes& method();
      MethodTypes method() const;

      Data& unknownMethodName();
      const Data& unknownMethodName() const;
      const Data& getSipVersion() const;

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
      StatusLine();
      StatusLine(HeaderFieldValue* hfv, Headers::Type type);
      StatusLine(const StatusLine&);
      StatusLine& operator=(const StatusLine&);

      int& responseCode();
      int responseCode() const;
      int& statusCode();
      int statusCode() const;
      const Data& getSipVersion() const;
      Data& reason();
      const Data& reason() const;

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
