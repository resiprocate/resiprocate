#include <iostream>
#include <memory>

#include "resip/stack/Contents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/ParserCategories.hxx"
#include "resip/stack/ParameterTypes.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Logger.hxx"
#include "tassert.h"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

void
wsinv()
{
/*
   This short, relatively human-readable message contains:

   o  line folding all over.

   o  escaped characters within quotes.

   o  an empty subject.

   o  LWS between colons, semicolons, header field values, and other
      fields.

   o  both comma separated and separately listed header field values.

   o  a mix of short and long form for the same header field name.

   o  unknown Request-URI parameter.

   o  unknown header fields.

   o  an unknown header field with a value that would be syntactically
      invalid if it were defined in terms of generic-param.

   o  unusual header field ordering.

   o  unusual header field name character case.

   o  unknown parameters of a known header field.

   o  a uri parameter with no value.

   o  a header parameter with no value.

   o  integer fields (Max-Forwards and CSeq) with leading zeros.

   All elements should treat this as a well-formed request.

   The UnknownHeaderWithUnusualValue header field deserves special
   attention.  If this header field were defined in terms of comma-
   separated values with semicolon-separated parameters (as would many
   of the existing defined header fields), this would be invalid.
   However, since the receiving element does not know the definition of
   the syntax for this field, it must parse it as a header value.
   Proxies would forward this header field unchanged.  Endpoints would
   ignore the header field.

INVITE sip:vivekg@chair-dnrc.example.com;unknownparam SIP/2.0
TO :
 sip:vivekg@chair-dnrc.example.com ;   tag    = 1918181833n
from   : "J Rosenberg \\\""       <sip:jdrosen@example.com>
  ;
  tag = 98asjd8
MaX-fOrWaRdS: 0068
Call-ID: wsinv.ndaksdj@192.0.2.1
Content-Length   : 150
cseq: 0009
  INVITE
Via  : SIP  /   2.0
 /UDP
    192.0.2.2;branch=390skdjuw
s :
NewFangledHeader:   newfangled value
 continued newfangled value
UnknownHeaderWithUnusualValue: ;;,,;;,;
Content-Type: application/sdp
Route:
 <sip:services.example.com;lr;unknownwith=value;unknown-no-value>
v:  SIP  / 2.0  / TCP     spindle.example.com   ;
  branch  =   z9hG4bK9ikj8  ,
 SIP  /    2.0   / UDP  192.168.255.111   ; branch=
 z9hG4bK30239
m:"Quoted string \"\"" <sip:jdrosen@example.com> ; newparam =
      newvalue ;
  secondparam ; q = 0.33

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.3
s=-
c=IN IP4 192.0.2.4
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("wsinv.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }
   
   // Request Line
   tassert(msg->header(resip::h_RequestLine).method()==resip::INVITE);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="vivekg");
   tassert(msg->header(resip::h_RequestLine).uri().host()=="chair-dnrc.example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==1);
   resip::ExtensionParameter p_unknownparam("unknownparam");
   tassert(msg->header(resip::h_RequestLine).uri().exists(p_unknownparam));
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");
   
   //To
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName().empty());
   tassert(msg->header(resip::h_To).numKnownParams()==1);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(msg->header(resip::h_To).exists(resip::p_tag));
   tassert(msg->header(resip::h_To).param(resip::p_tag)=="1918181833n");
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="vivekg");
   tassert(msg->header(resip::h_To).uri().host()=="chair-dnrc.example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="J Rosenberg \\\\\\\"");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="98asjd8");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="jdrosen");
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);
   
   //Max-Forwards
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==68);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);
   
   
   //Call-ID
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="wsinv.ndaksdj@192.0.2.1");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);
   
   //Content-Length
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==150);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);
   
   //CSeq
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::INVITE);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_CSeq).sequence()==9);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);
   
   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==3);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="192.0.2.2");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(!(i->param(resip::p_branch).hasMagicCookie()));
   tassert(i->param(resip::p_branch).getTransactionId()=="390skdjuw");
   tassert(i->param(resip::p_branch).clientData().empty());
   
   i++;
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="TCP");
   tassert(i->sentHost()=="spindle.example.com");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="9ikj8");
   tassert(i->param(resip::p_branch).clientData().empty());
      
   i++;
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="192.168.255.111");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="30239");
   tassert(i->param(resip::p_branch).clientData().empty());
   
   
   //Subject
   tassert(msg->exists(resip::h_Subject));
   tassert(msg->header(resip::h_Subject).value()=="");
   tassert(msg->header(resip::h_Subject).numKnownParams()==0);
   tassert(msg->header(resip::h_Subject).numUnknownParams()==0);
   

   // Unknown headers
   resip::ExtensionHeader h_NewFangledHeader("NewFangledHeader");
   
   tassert(msg->exists(h_NewFangledHeader));
   tassert(msg->header(h_NewFangledHeader).size()==1);
   tassert(msg->header(h_NewFangledHeader).begin()->value()=="newfangled value\r\n continued newfangled value");
   tassert(msg->header(h_NewFangledHeader).begin()->numKnownParams()==0);
   tassert(msg->header(h_NewFangledHeader).begin()->numUnknownParams()==0);
   
   resip::ExtensionHeader h_UnknownHeaderWithUnusualValue("UnknownHeaderWithUnusualValue");
   
   tassert(msg->exists(h_UnknownHeaderWithUnusualValue));
   tassert(msg->header(h_UnknownHeaderWithUnusualValue).size()==1);
   tassert(msg->header(h_UnknownHeaderWithUnusualValue).begin()->value()==";;,,;;,;");
   tassert(msg->header(h_UnknownHeaderWithUnusualValue).begin()->numKnownParams()==0);
   tassert(msg->header(h_UnknownHeaderWithUnusualValue).begin()->numUnknownParams()==0);
   
   //Content-Type
   tassert(msg->exists(resip::h_ContentType));
   tassert(msg->header(resip::h_ContentType).type()=="application");
   tassert(msg->header(resip::h_ContentType).subType()=="sdp");
   tassert(msg->header(resip::h_ContentType).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentType).numUnknownParams()==0);
   
   //Contact
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   tassert(msg->header(resip::h_Contacts).begin()->displayName()=="Quoted string \\\"\\\"");
   tassert(msg->header(resip::h_Contacts).begin()->numKnownParams()==1);
   tassert(msg->header(resip::h_Contacts).begin()->numUnknownParams()==2);
   tassert(!(msg->header(resip::h_Contacts).begin()->isAllContacts()));
   tassert(msg->header(resip::h_Contacts).begin()->uri().numKnownParams()==0);
   tassert(msg->header(resip::h_Contacts).begin()->uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_Contacts).begin()->uri().scheme()=="sip");
   tassert(msg->header(resip::h_Contacts).begin()->uri().user()=="jdrosen");
   tassert(msg->header(resip::h_Contacts).begin()->uri().host()=="example.com");
   tassert(msg->header(resip::h_Contacts).begin()->uri().port()==0);
   tassert(msg->header(resip::h_Contacts).begin()->uri().password().empty());
   tassert(!(msg->header(resip::h_Contacts).begin()->uri().hasEmbedded()));

   resip::ExtensionParameter p_newparam("newparam");
   tassert(msg->header(resip::h_Contacts).begin()->exists(p_newparam));
   tassert(msg->header(resip::h_Contacts).begin()->param(p_newparam)=="newvalue");
   
   resip::ExtensionParameter p_secondparam("secondparam");
   tassert(msg->header(resip::h_Contacts).begin()->exists(p_secondparam));
   tassert(msg->header(resip::h_Contacts).begin()->param(p_secondparam)=="");
   
   tassert(msg->header(resip::h_Contacts).begin()->exists(resip::p_q));
   tassert(msg->header(resip::h_Contacts).begin()->param(resip::p_q)==330);
   
   
   
   

   InfoLog(<< "In case wsinv:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
intmeth()
{
#if 0
/*
   This message exercises a wider range of characters in several key
   syntactic elements than implementations usually see.  In particular,
   note the following:

   o  The Method contains non-alpha characters from token.  Note that %
      is not an escape character for this field.  A method of IN%56ITE
      is an unknown method.  It is not the same as a method of INVITE.

   o  The Request-URI contains unusual, but legal, characters.

   o  A branch parameter contains all non-alphanum characters from
      token.

   o  The To header field value's quoted string contains quoted-pair
      expansions, including a quoted NULL character.

   o  The name part of name-addr in the From header field value contains
      multiple tokens (instead of a quoted string) with all non-alphanum
      characters from the token production rule.  That value also has an
      unknown header parameter whose name contains the non-alphanum
      token characters and whose value is a non-ascii range UTF-8
      encoded string.  The tag parameter on this value contains the
      non-alphanum token characters.

   o  The Call-ID header field value contains the non-alphanum
      characters from word.  Notice that in this production:

      *  % is not an escape character.  It is only an escape character
         in productions matching the rule "escaped".

      *  " does not start a quoted string.  None of ',` or " imply that
         there will be a matching symbol later in the string.

      *  The characters []{}()<> do not have any grouping semantics.
         They are not required to appear in balanced pairs.

   o  There is an unknown header field (matching extension-header) with
      non-alphanum token characters in its name and a UTF8-NONASCII
      value.

   If this unusual URI has been defined at a proxy, the proxy will
   forward this request normally.  Otherwise, a proxy will generate a
   404.  Endpoints will generate a 501 listing the methods they
   understand in an Allow header field.


!interesting-Method0123456789_*+`.%indeed'~ sip:1_unusual.URI~(to-be!sure)&isn't+it$/crazy?,/;;*:&it+has=1,weird!*pas$wo~d_too.(doesn't-it)@example.com SIP/2.0
Via: SIP/2.0/TCP host1.example.com;branch=z9hG4bK-.!%66*_+`'~
To: "BEL:\ NUL:\  DEL:\" <sip:1_unusual.URI~(to-be!sure)&isn't+it$/crazy?,/;;*@example.com>
From: token1~` token2'+_ token3*%!.- <sip:mundane@example.com>;fromParam''~+*_!.-%="—Ä–∞–±–æ—Ç–∞—é—â–∏–π";tag=_token~1'+`*%!-.
Call-ID: intmeth.word%ZK-!.*_+'@word`~)(><:\/"][?}{
CSeq: 139122385 !interesting-Method0123456789_*+`.%indeed'~
Max-Forwards: 255
extensionHeader-!.%*+_`'~:ÔªøÂ§ßÂÅúÈõª
Content-Length: 0


*/
#endif

   FILE* fid= fopen("intmeth.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   // Request Line
   tassert(msg->header(resip::h_RequestLine).method()==resip::UNKNOWN);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="!interesting-Method0123456789_*+`.%indeed'~");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="1_unusual.URI~(to-be!sure)&isn't+it$/crazy?,/;;*");
   tassert(msg->header(resip::h_RequestLine).uri().password()=="&it+has=1,weird!*pas$wo~d_too.(doesn't-it)");
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");
   
   //To
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   resip::Data dispName;
   dispName+="BEL:\\";
   dispName+=(char)0x07;
   dispName+=" NUL:\\";
   dispName+=(char)0x00;
   dispName+=" DEL:\\";
   dispName+=(char)0x7F;   
   tassert(msg->header(resip::h_To).displayName()==dispName);
   tassert(!(msg->header(resip::h_To).isAllContacts()));

   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="1_unusual.URI~(to-be!sure)&isn't+it$/crazy?,/;;*");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==1);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="_token~1'+`*%!-.");
   
   resip::ExtensionParameter p_oddball("fromParam''~+*_!.-%");
   tassert(msg->header(resip::h_From).exists(p_oddball));
   resip::Data binaryParamVal;
   binaryParamVal+=(char)0xD1;
   binaryParamVal+=(char)0x80;
   binaryParamVal+=(char)0xD0;
   binaryParamVal+=(char)0xB0;
   binaryParamVal+=(char)0xD0;
   binaryParamVal+=(char)0xB1;
   binaryParamVal+=(char)0xD0;
   binaryParamVal+=(char)0xBE;
   binaryParamVal+=(char)0xD1;
   binaryParamVal+=(char)0x82;
   binaryParamVal+=(char)0xD0;
   binaryParamVal+=(char)0xB0;
   binaryParamVal+=(char)0xD1;
   binaryParamVal+=(char)0x8E;
   binaryParamVal+=(char)0xD1;
   binaryParamVal+=(char)0x89;
   binaryParamVal+=(char)0xD0;
   binaryParamVal+=(char)0xB8;
   binaryParamVal+=(char)0xD0;
   binaryParamVal+=(char)0xB9;
   tassert(msg->header(resip::h_From).param(p_oddball)==binaryParamVal);
   tassert(msg->header(resip::h_From).displayName()=="token1~` token2'+_ token3*%!.-");
   tassert(!(msg->header(resip::h_From).isAllContacts()));

   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="mundane");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);
   
   
   //Max-Forwards
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==255);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);
   
   //Call-ID
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="intmeth.word%ZK-!.*_+'@word`~)(><:\\/\"][?}{");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);
   
   //Content-Length
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);
   
   //CSeq
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::UNKNOWN);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="!interesting-Method0123456789_*+`.%indeed'~");
   tassert(msg->header(resip::h_CSeq).sequence()==139122385);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);
   
   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="TCP");
   tassert(i->sentHost()=="host1.example.com");
   tassert(i->sentPort()==0);
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="-.!%66*_+`'~");
   tassert(i->param(resip::p_branch).clientData().empty());
   

   
   // Unknown headers
   resip::ExtensionHeader h_extensionHeader("extensionHeader-!.%*+_`'~");
   
   tassert(msg->exists(h_extensionHeader));
   tassert(msg->header(h_extensionHeader).size()==1);
   resip::Data binaryHfv;
   binaryHfv+=(char)0xEF;
   binaryHfv+=(char)0xBB;
   binaryHfv+=(char)0xBF;
   binaryHfv+=(char)0xE5;
   binaryHfv+=(char)0xA4;
   binaryHfv+=(char)0xA7;
   binaryHfv+=(char)0xE5;
   binaryHfv+=(char)0x81;
   binaryHfv+=(char)0x9C;
   binaryHfv+=(char)0xE9;
   binaryHfv+=(char)0x9B;
   binaryHfv+=(char)0xBB;
   tassert(msg->header(h_extensionHeader).begin()->value()==binaryHfv);
   tassert(msg->header(h_extensionHeader).begin()->numKnownParams()==0);
   tassert(msg->header(h_extensionHeader).begin()->numUnknownParams()==0);
   
   
   InfoLog(<< "In case intmeth:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
esc01()
{
/*
   This INVITE exercises the % HEX HEX escaping mechanism in several
   places.  The request is syntactically valid.  Interesting features
   include the following:

   o  The request-URI has sips:user@example.com embedded in its
      userpart.  What that might mean to example.net is beyond the scope
      of this document.

   o  The From and To URIs have escaped characters in their userparts.

   o  The Contact URI has escaped characters in the URI parameters.
      Note that the "name" uri-parameter has a value of "value%41",
      which is NOT equivalent to "valueA".  Per [RFC3986], unescaping
      URI components is never performed recursively.

   A parser must accept this as a well-formed message.  The application
   using the message must treat the % HEX HEX expansions as equivalent
   to the character being encoded.  The application must not try to
   interpret % as an escape character in those places where % HEX HEX
   ("escaped" in the grammar) is not a valid part of the construction.
   In [RFC3261], "escaped" only occurs in the expansions of SIP-URI,
   SIPS-URI, and Reason-Phrase.
   
   
INVITE sip:sips%3Auser%40example.com@example.net SIP/2.0
To: sip:%75se%72@example.com
From: <sip:I%20have%20spaces@example.net>;tag=938
Max-Forwards: 87
i: esc01.239409asdfakjkn23onasd0-3234
CSeq: 234234 INVITE
Via: SIP/2.0/UDP host5.example.net;branch=z9hG4bKkdjuw
C: application/sdp
Contact:
  <sip:cal%6Cer@host5.example.net;%6C%72;n%61me=v%61lue%25%34%31>
Content-Length: 150

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("esc01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   // Request Line
   tassert(msg->header(resip::h_RequestLine).method()==resip::INVITE);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   // ?bwc? Is it appropriate to use a one-size-fits-all approach to unescaping
   // parsed components? ('@' is not legal in a userpart, but the internal
   // representation contains it)
   // What is the best approach: all (unescape everything that is printable),
   // none (leave escaping exactly as found), or context sensitive (escape
   // everything that is legal for a given field)?
   tassert(msg->header(resip::h_RequestLine).uri().user()=="sips%3Auser%40example.com");
   tassert_reset();
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.net");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");
   
   //To
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName().empty());
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);

   //From
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName().empty());
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   // ?bwc? ' ' is not legal in a userpart, but the internal
   // representation contains it. Is this appropriate?
   tassert(msg->header(resip::h_From).uri().user()=="I%20have%20spaces");
   tassert_reset();
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.net");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="938");
   
   //Max-Forwards
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==87);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);
   
   //Call-ID
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="esc01.239409asdfakjkn23onasd0-3234");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);
   
   //Content-Length
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==150);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);
   
   //CSeq
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::INVITE);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_CSeq).sequence()==234234);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);
   
   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="host5.example.net");
   tassert(i->sentPort()==0);
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);

   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="kdjuw");
   tassert(i->param(resip::p_branch).clientData().empty());
   
   
   
   //Content-Type
   tassert(msg->exists(resip::h_ContentType));
   tassert(msg->header(resip::h_ContentType).type()=="application");
   tassert(msg->header(resip::h_ContentType).subType()=="sdp");
   tassert(msg->header(resip::h_ContentType).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentType).numUnknownParams()==0);
   
   //Contact
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   tassert(msg->header(resip::h_Contacts).begin()->displayName().empty());
   tassert(!(msg->header(resip::h_Contacts).begin()->isAllContacts()));
   tassert(msg->header(resip::h_Contacts).begin()->uri().scheme()=="sip");
   tassert(msg->header(resip::h_Contacts).begin()->uri().user()=="caller");
   tassert(msg->header(resip::h_Contacts).begin()->uri().password().empty());
   tassert(msg->header(resip::h_Contacts).begin()->uri().host()=="host5.example.net");
   tassert(msg->header(resip::h_Contacts).begin()->uri().port()==0);
   tassert(!(msg->header(resip::h_Contacts).begin()->uri().hasEmbedded()));

   tassert(msg->header(resip::h_Contacts).begin()->uri().numKnownParams()==0);
   tassert(msg->header(resip::h_Contacts).begin()->uri().numUnknownParams()==2);

   // ?bwc? These params have escaped stuff in them; is it mandatory that we
   // treat escaped and unescaped versions of the same parameter as identical?
   
   resip::ExtensionParameter p_wonky1("%6C%72");
   resip::ExtensionParameter p_wonky2("n%61me");
   tassert(msg->header(resip::h_Contacts).begin()->uri().exists(p_wonky1));
   tassert(msg->header(resip::h_Contacts).begin()->uri().param(p_wonky1)=="");
   tassert(msg->header(resip::h_Contacts).begin()->uri().exists(p_wonky2));
   tassert(msg->header(resip::h_Contacts).begin()->uri().param(p_wonky2)=="v%61lue%25%34%31");


   tassert(msg->header(resip::h_Contacts).begin()->numKnownParams()==0);
   tassert(msg->header(resip::h_Contacts).begin()->numUnknownParams()==0);

   tassert_reset();

   InfoLog(<< "In case esc01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
escnull()
{
/*
   This register request contains several URIs with nulls in the
   userpart.  The message is well formed - parsers must accept this
   message.  Implementations must take special care when unescaping the
   Address-of-Record (AOR) in this request so as to not prematurely
   shorten the username.  This request registers two distinct contact
   URIs.

REGISTER sip:example.com SIP/2.0
To: sip:null-%00-null@example.com
From: sip:null-%00-null@example.com;tag=839923423
Max-Forwards: 70
Call-ID: escnull.39203ndfvkjdasfkq3w4otrq0adsfdfnavd
CSeq: 14398234 REGISTER
Via: SIP/2.0/UDP host5.example.com;branch=z9hG4bKkdjuw
Contact: <sip:%00@host5.example.com>
Contact: <sip:%00%00@host5.example.com>
L:0


*/
   FILE* fid= fopen("escnull.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   // Request Line
   //REGISTER sip:example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::REGISTER);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="REGISTER");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");
   
   //To: sip:null-%00-null@example.com
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName().empty());
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="null-%00-null");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: sip:null-%00-null@example.com;tag=839923423
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="839923423");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="null-%00-null");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);
   
   //Max-Forwards: 70
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==70);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);
   
   
   //Call-ID: escnull.39203ndfvkjdasfkq3w4otrq0adsfdfnavd
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="escnull.39203ndfvkjdasfkq3w4otrq0adsfdfnavd");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);
   
   //CSeq: 14398234 REGISTER
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::REGISTER);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="REGISTER");
   tassert(msg->header(resip::h_CSeq).sequence()==14398234);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);
   
   //Via: SIP/2.0/UDP host5.example.com;branch=z9hG4bKkdjuw
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="host5.example.com");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="kdjuw");
   tassert(i->param(resip::p_branch).clientData().empty());
      
   //Contact: <sip:%00@host5.example.com>
   //Contact: <sip:%00%00@host5.example.com>
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==2);
   resip::ParserContainer<resip::NameAddr>::iterator j=msg->header(resip::h_Contacts).begin();
   
   tassert(j->displayName()=="");
   tassert(j->numKnownParams()==0);
   tassert(j->numUnknownParams()==0);
   tassert(!(j->isAllContacts()));
   tassert(j->uri().numKnownParams()==0);
   tassert(j->uri().numUnknownParams()==0);
   tassert(j->uri().scheme()=="sip");
   tassert(j->uri().user()=="%00");
   tassert(j->uri().password().empty());
   tassert(j->uri().host()=="host5.example.com");
   tassert(j->uri().port()==0);
   tassert(!(j->uri().hasEmbedded()));

   j++;
   
   tassert(j->displayName()=="");
   tassert(j->numKnownParams()==0);
   tassert(j->numUnknownParams()==0);
   tassert(!(j->isAllContacts()));
   tassert(j->uri().numKnownParams()==0);
   tassert(j->uri().numUnknownParams()==0);
   tassert(j->uri().scheme()=="sip");
   tassert(j->uri().user()=="%00%00");
   tassert(j->uri().password().empty());
   tassert(j->uri().host()=="host5.example.com");
   tassert(j->uri().port()==0);
   tassert(!(j->uri().hasEmbedded()));

   //L:0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);
   
   


   InfoLog(<< "In case escnull:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
esc02()
{
/*
   In most of the places % can appear in a SIP message, it is not an
   escape character.  This can surprise the unwary implementor.  The
   following well-formed request has these properties:

   o  The request method is unknown.  It is NOT equivalent to REGISTER.

   o  The display name portion of the To and From header fields is
      "%Z%45".  Note that this is not the same as %ZE.

   o  This message has two Contact header field values, not three.
      <sip:alias2@host2.example.com> is a C%6Fntact header field value.

   A parser should accept this message as well formed.  A proxy would
   forward or reject the message depending on what the Request-URI meant
   to it.  An endpoint would reject this message with a 501.


RE%47IST%45R sip:registrar.example.com SIP/2.0
To: "%Z%45" <sip:resource@example.com>
From: "%Z%45" <sip:resource@example.com>;tag=f232jadfj23
Call-ID: esc02.asdfnqwo34rq23i34jrjasdcnl23nrlknsdf
Via: SIP/2.0/TCP host.example.com;branch=z9hG4bK209%fzsnel234
CSeq: 29344 RE%47IST%45R
Max-Forwards: 70
Contact: <sip:alias1@host1.example.com>
C%6Fntact: <sip:alias2@host2.example.com>
Contact: <sip:alias3@host3.example.com>
l: 0


*/
   FILE* fid= fopen("esc02.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //RE%47IST%45R sip:registrar.example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::UNKNOWN);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="RE%47IST%45R");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="registrar.example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: "%Z%45" <sip:resource@example.com>
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="%Z%45");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="resource");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: "%Z%45" <sip:resource@example.com>;tag=f232jadfj23
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="%Z%45");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="f232jadfj23");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="resource");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Call-ID: esc02.asdfnqwo34rq23i34jrjasdcnl23nrlknsdf
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="esc02.asdfnqwo34rq23i34jrjasdcnl23nrlknsdf");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //Via: SIP/2.0/TCP host.example.com;branch=z9hG4bK209%fzsnel234
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="TCP");
   tassert(i->sentHost()=="host.example.com");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="209%fzsnel234");
   tassert(i->param(resip::p_branch).clientData().empty());
   
   //CSeq: 29344 RE%47IST%45R
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::UNKNOWN);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="RE%47IST%45R");
   tassert(msg->header(resip::h_CSeq).sequence()==29344);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //Max-Forwards: 70
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==70);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Contact: <sip:alias1@host1.example.com>
   //Contact: <sip:alias3@host3.example.com>
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==2);
   resip::ParserContainer<resip::NameAddr>::iterator j=msg->header(resip::h_Contacts).begin();
   
   tassert(j->displayName()=="");
   tassert(j->numKnownParams()==0);
   tassert(j->numUnknownParams()==0);
   tassert(!(j->isAllContacts()));
   tassert(j->uri().numKnownParams()==0);
   tassert(j->uri().numUnknownParams()==0);
   tassert(j->uri().scheme()=="sip");
   tassert(j->uri().user()=="alias1");
   tassert(j->uri().password().empty());
   tassert(j->uri().host()=="host1.example.com");
   tassert(j->uri().port()==0);
   tassert(!(j->uri().hasEmbedded()));

   j++;
   
   tassert(j->displayName()=="");
   tassert(j->numKnownParams()==0);
   tassert(j->numUnknownParams()==0);
   tassert(!(j->isAllContacts()));
   tassert(j->uri().numKnownParams()==0);
   tassert(j->uri().numUnknownParams()==0);
   tassert(j->uri().scheme()=="sip");
   tassert(j->uri().user()=="alias3");
   tassert(j->uri().password().empty());
   tassert(j->uri().host()=="host3.example.com");
   tassert(j->uri().port()==0);
   tassert(!(j->uri().hasEmbedded()));

   //C%6Fntact: <sip:alias2@host2.example.com>
   resip::ExtensionHeader p_fakeContact("C%6Fntact");
   tassert(msg->exists(p_fakeContact));
   tassert(msg->header(p_fakeContact).size()==1);
   tassert(msg->header(p_fakeContact).begin()->value()=="<sip:alias2@host2.example.com>");
   
   //l: 0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);


   InfoLog(<< "In case esc02:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
lwsdisp()
{
/*
   This OPTIONS request is not valid per the grammar in RFC 3261 since
   there is no LWS between the token in the display name and < in the
   From header field value.  This has been identified as a specification
   bug that will be removed when RFC 3261 is revised.  Elements should
   accept this request as well formed.

OPTIONS sip:user@example.com SIP/2.0
To: sip:user@example.com
From: caller<sip:caller@example.com>;tag=323
Max-Forwards: 70
Call-ID: lwsdisp.1234abcd@funky.example.com
CSeq: 60 OPTIONS
Via: SIP/2.0/UDP funky.example.com;branch=z9hG4bKkdjuw
l: 0


*/
   FILE* fid= fopen("lwsdisp.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Request-Line
   //OPTIONS sip:user@example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="user");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: sip:user@example.com
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: caller<sip:caller@example.com>;tag=323
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="caller");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="323");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="caller");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Max-Forwards: 70
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==70);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Call-ID: lwsdisp.1234abcd@funky.example.com
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="lwsdisp.1234abcd@funky.example.com");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 60 OPTIONS
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_CSeq).sequence()==60);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //Via: SIP/2.0/UDP funky.example.com;branch=z9hG4bKkdjuw
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="funky.example.com");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="kdjuw");
   tassert(i->param(resip::p_branch).clientData().empty());

   //l: 0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);

   InfoLog(<< "In case lwsdisp:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
longreq()
{
/*
   This well-formed request contains header fields with many values and
   values that are very long.  Features include the following:

   o  The To header field has a long display name, and long uri
      parameter names and values.

   o  The From header field has long header parameter names and values,
      in particular, a very long tag.

   o  The Call-ID is one long token.

INVITE sip:user@example.com SIP/2.0
To: "I have a user name of extremeextremeextremeextremeextremeextremeextremeextremeextremeextreme proportion"<sip:user@example.com:6000;unknownparam1=verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongvalue;longparamnamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename=shortvalue;verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongParameterNameWithNoValue>
F: sip:amazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallername@example.net;tag=12982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982424;unknownheaderparamnamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename=unknowheaderparamvaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue;unknownValuelessparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamname
Call-ID: longreq.onereallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallylongcallid
CSeq: 3882340 INVITE
Unknown-LongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong-Name: unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-value; unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-parameter-name = unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-parameter-value
Via: SIP/2.0/TCP sip33.example.com
v: SIP/2.0/TCP sip32.example.com
V: SIP/2.0/TCP sip31.example.com
Via: SIP/2.0/TCP sip30.example.com
ViA: SIP/2.0/TCP sip29.example.com
VIa: SIP/2.0/TCP sip28.example.com
VIA: SIP/2.0/TCP sip27.example.com
via: SIP/2.0/TCP sip26.example.com
viA: SIP/2.0/TCP sip25.example.com
vIa: SIP/2.0/TCP sip24.example.com
vIA: SIP/2.0/TCP sip23.example.com
V :  SIP/2.0/TCP sip22.example.com
v :  SIP/2.0/TCP sip21.example.com
V  : SIP/2.0/TCP sip20.example.com
v  : SIP/2.0/TCP sip19.example.com
Via : SIP/2.0/TCP sip18.example.com
Via  : SIP/2.0/TCP sip17.example.com
Via: SIP/2.0/TCP sip16.example.com
Via: SIP/2.0/TCP sip15.example.com
Via: SIP/2.0/TCP sip14.example.com
Via: SIP/2.0/TCP sip13.example.com
Via: SIP/2.0/TCP sip12.example.com
Via: SIP/2.0/TCP sip11.example.com
Via: SIP/2.0/TCP sip10.example.com
Via: SIP/2.0/TCP sip9.example.com
Via: SIP/2.0/TCP sip8.example.com
Via: SIP/2.0/TCP sip7.example.com
Via: SIP/2.0/TCP sip6.example.com
Via: SIP/2.0/TCP sip5.example.com
Via: SIP/2.0/TCP sip4.example.com
Via: SIP/2.0/TCP sip3.example.com
Via: SIP/2.0/TCP sip2.example.com
Via: SIP/2.0/TCP sip1.example.com
Via: SIP/2.0/TCP host.example.com;received=192.0.2.5;branch=verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongbranchvalue
Max-Forwards: 70
Contact: <sip:amazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallername@host5.example.net>
Content-Type: application/sdp
l: 150

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("longreq.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Request Line
   //INVITE sip:user@example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::INVITE);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="user");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: "I have a user name of extremeextremeextremeextremeextremeextremeextremeextremeextremeextreme proportion"<sip:user@example.com:6000;unknownparam1=verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongvalue;longparamnamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename=shortvalue;verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongParameterNameWithNoValue>
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="I have a user name of extremeextremeextremeextremeextremeextremeextremeextremeextremeextreme proportion");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==6000);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==3);

   resip::ExtensionParameter p_unknownparam1("unknownparam1");
   tassert(msg->header(resip::h_To).uri().exists(p_unknownparam1));
   tassert(msg->header(resip::h_To).uri().param(p_unknownparam1)=="verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongvalue");

   resip::ExtensionParameter p_long("longparamnamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename");
   tassert(msg->header(resip::h_To).uri().exists(p_long));
   tassert(msg->header(resip::h_To).uri().param(p_long)=="shortvalue");

   resip::ExtensionParameter p_verylong("verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongParameterNameWithNoValue");
   tassert(msg->header(resip::h_To).uri().exists(p_verylong));
   tassert(msg->header(resip::h_To).uri().param(p_verylong)=="");

   //F: sip:amazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallername@example.net;tag=12982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982424;unknownheaderparamnamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename=unknowheaderparamvaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue;unknownValuelessparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamname
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==2);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="12982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982982424");
   
   resip::ExtensionParameter p_unknownheaderparameternameXalot("unknownheaderparamnamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamenamename");
   tassert(msg->header(resip::h_From).exists(p_unknownheaderparameternameXalot));
   tassert(msg->header(resip::h_From).param(p_unknownheaderparameternameXalot)=="unknowheaderparamvaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevaluevalue");

   resip::ExtensionParameter p_unknownvalueless("unknownValuelessparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamnameparamname");
   tassert(msg->header(resip::h_From).exists(p_unknownvalueless));
   tassert(msg->header(resip::h_From).param(p_unknownvalueless)=="");

   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="amazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallername");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.net");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Call-ID: longreq.onereallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallylongcallid
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="longreq.onereallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallyreallylongcallid");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 3882340 INVITE
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::INVITE);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_CSeq).sequence()==3882340);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //Unknown headers
   //Unknown-LongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong-Name: unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-value; unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-parameter-name = unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-parameter-value
   resip::ExtensionHeader h_UnknownLong("Unknown-LongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLongLong-Name");
   
   tassert(msg->exists(h_UnknownLong));
   tassert(msg->header(h_UnknownLong).size()==1);
   tassert(msg->header(h_UnknownLong).begin()->value()=="unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-value; unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-parameter-name = unknown-longlonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglong-parameter-value");
   tassert(msg->header(h_UnknownLong).begin()->numKnownParams()==0);
   tassert(msg->header(h_UnknownLong).begin()->numUnknownParams()==0);
   
   //Vias

   //Via: SIP/2.0/TCP sip33.example.com
   //v: SIP/2.0/TCP sip32.example.com
   //V: SIP/2.0/TCP sip31.example.com
   //Via: SIP/2.0/TCP sip30.example.com
   //ViA: SIP/2.0/TCP sip29.example.com
   //VIa: SIP/2.0/TCP sip28.example.com
   //VIA: SIP/2.0/TCP sip27.example.com
   //via: SIP/2.0/TCP sip26.example.com
   //viA: SIP/2.0/TCP sip25.example.com
   //vIa: SIP/2.0/TCP sip24.example.com
   //vIA: SIP/2.0/TCP sip23.example.com
   //V :  SIP/2.0/TCP sip22.example.com
   //v :  SIP/2.0/TCP sip21.example.com
   //V  : SIP/2.0/TCP sip20.example.com
   //v  : SIP/2.0/TCP sip19.example.com
   //Via : SIP/2.0/TCP sip18.example.com
   //Via  : SIP/2.0/TCP sip17.example.com
   //Via: SIP/2.0/TCP sip16.example.com
   //Via: SIP/2.0/TCP sip15.example.com
   //Via: SIP/2.0/TCP sip14.example.com
   //Via: SIP/2.0/TCP sip13.example.com
   //Via: SIP/2.0/TCP sip12.example.com
   //Via: SIP/2.0/TCP sip11.example.com
   //Via: SIP/2.0/TCP sip10.example.com
   //Via: SIP/2.0/TCP sip9.example.com
   //Via: SIP/2.0/TCP sip8.example.com
   //Via: SIP/2.0/TCP sip7.example.com
   //Via: SIP/2.0/TCP sip6.example.com
   //Via: SIP/2.0/TCP sip5.example.com
   //Via: SIP/2.0/TCP sip4.example.com
   //Via: SIP/2.0/TCP sip3.example.com
   //Via: SIP/2.0/TCP sip2.example.com
   //Via: SIP/2.0/TCP sip1.example.com
   //Via: SIP/2.0/TCP host.example.com;received=192.0.2.5;branch=verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongbranchvalue

   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==34);
   
   int i=33;
   resip::Vias::iterator iter;
   for(iter=msg->header(resip::h_Vias).begin();
         iter!=msg->header(resip::h_Vias).end(); iter++)
   {
      if(i==0)
      {
         break;
      }
      tassert(iter->numKnownParams()==0);
      tassert(iter->numUnknownParams()==0);
      tassert(iter->protocolName()=="SIP");
      tassert(iter->protocolVersion()=="2.0");
      tassert(iter->transport()=="TCP");
      tassert(iter->sentHost()==resip::Data("sip")+resip::Data(i)+resip::Data(".example.com"));
      tassert(iter->sentPort()==0);
      i--;
   }
   
   tassert(iter->numKnownParams()==2);
   tassert(iter->numUnknownParams()==0);
   tassert(iter->protocolName()=="SIP");
   tassert(iter->protocolVersion()=="2.0");
   tassert(iter->transport()=="TCP");
   tassert(iter->sentHost()=="host.example.com");
   tassert(iter->sentPort()==0);
   
   tassert(iter->exists(resip::p_branch));
   tassert(!(iter->param(resip::p_branch).hasMagicCookie()));
   tassert(iter->param(resip::p_branch).getTransactionId()=="verylonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglonglongbranchvalue");
   tassert(iter->param(resip::p_branch).clientData().empty());

   tassert(iter->exists(resip::p_received));
   tassert(iter->param(resip::p_received)=="192.0.2.5");

   //Max-Forwards: 70
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==70);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Contact: <sip:amazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallername@host5.example.net>
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   resip::ParserContainer<resip::NameAddr>::iterator j=msg->header(resip::h_Contacts).begin();
   
   tassert(j->displayName()=="");
   tassert(j->numKnownParams()==0);
   tassert(j->numUnknownParams()==0);
   tassert(!(j->isAllContacts()));
   tassert(j->uri().numKnownParams()==0);
   tassert(j->uri().numUnknownParams()==0);
   tassert(j->uri().scheme()=="sip");
   tassert(j->uri().user()=="amazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallernameamazinglylongcallername");
   tassert(j->uri().password().empty());
   tassert(j->uri().host()=="host5.example.net");
   tassert(j->uri().port()==0);
   tassert(!(j->uri().hasEmbedded()));

   //Content-Type: application/sdp
   tassert(msg->exists(resip::h_ContentType));
   tassert(msg->header(resip::h_ContentType).type()=="application");
   tassert(msg->header(resip::h_ContentType).subType()=="sdp");
   tassert(msg->header(resip::h_ContentType).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentType).numUnknownParams()==0);
   
   //l: 150
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==150);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);

   InfoLog(<< "In case longreq:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
dblreq()
{
/*
   This message contains a single SIP REGISTER request, which ostensibly
   arrived over UDP in a single datagram.  The packet contains extra
   octets after the body (which in this case has zero length).  The
   extra octets happen to look like a SIP INVITE request, but (per
   section 18.3 of [RFC3261]) they are just spurious noise that must be
   ignored.

   A SIP element receiving this datagram would handle the REGISTER
   request normally and ignore the extra bits that look like an INVITE
   request.  If the element is a proxy choosing to forward the REGISTER,
   the INVITE octets would not appear in the forwarded request.



REGISTER sip:example.com SIP/2.0
To: sip:j.user@example.com
From: sip:j.user@example.com;tag=43251j3j324
Max-Forwards: 8
I: dblreq.0ha0isndaksdj99sdfafnl3lk233412
Contact: sip:j.user@host.example.com
CSeq: 8 REGISTER
Via: SIP/2.0/UDP 192.0.2.125;branch=z9hG4bKkdjuw23492
Content-Length: 0


INVITE sip:joe@example.com SIP/2.0
t: sip:joe@example.com
From: sip:caller@example.net;tag=141334
Max-Forwards: 8
Call-ID: dblreq.0ha0isnda977644900765@192.0.2.15
CSeq: 8 INVITE
Via: SIP/2.0/UDP 192.0.2.15;branch=z9hG4bKkdjuw380234
Content-Type: application/sdp
Content-Length: 150

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.15
s=-
c=IN IP4 192.0.2.15
t=0 0
m=audio 49217 RTP/AVP 0 12
m =video 3227 RTP/AVP 31
a=rtpmap:31 LPC


*/
   FILE* fid= fopen("dblreq.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }
   
   //Request Line
   //REGISTER sip:example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::REGISTER);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="REGISTER");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: sip:j.user@example.com
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="j.user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: sip:j.user@example.com;tag=43251j3j324
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="43251j3j324");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="j.user");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Max-Forwards: 8
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==8);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //I: dblreq.0ha0isndaksdj99sdfafnl3lk233412
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="dblreq.0ha0isndaksdj99sdfafnl3lk233412");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //Contact: sip:j.user@host.example.com
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   resip::ParserContainer<resip::NameAddr>::iterator j=msg->header(resip::h_Contacts).begin();
   
   tassert(j->displayName()=="");
   tassert(j->numKnownParams()==0);
   tassert(j->numUnknownParams()==0);
   tassert(!(j->isAllContacts()));
   tassert(j->uri().numKnownParams()==0);
   tassert(j->uri().numUnknownParams()==0);
   tassert(j->uri().scheme()=="sip");
   tassert(j->uri().user()=="j.user");
   tassert(j->uri().password().empty());
   tassert(j->uri().host()=="host.example.com");
   tassert(j->uri().port()==0);
   tassert(!(j->uri().hasEmbedded()));

   //CSeq: 8 REGISTER
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::REGISTER);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="REGISTER");
   tassert(msg->header(resip::h_CSeq).sequence()==8);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //Via: SIP/2.0/UDP 192.0.2.125;branch=z9hG4bKkdjuw23492
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="192.0.2.125");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="kdjuw23492");
   tassert(i->param(resip::p_branch).clientData().empty());

   //Content-Length: 0
   tassert(msg->exists(resip::h_ContentLength));
   // .bwc. We configured SipMessage to take the Content-Length seriously
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);



   InfoLog(<< "In case dblreq:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
semiuri()
{
/*
   This request has a semicolon-separated parameter contained in the
   "user" part of the Request-URI (whose value contains an escaped @
   symbol).  Receiving elements will accept this as a well-formed
   message.  The Request-URI will parse so that the user part is
   "user;par=u@example.net".

OPTIONS sip:user;par=u%40example.net@example.com SIP/2.0
To: sip:j_user@example.com
From: sip:caller@example.org;tag=33242
Max-Forwards: 3
Call-ID: semiuri.0ha0isndaksdj
CSeq: 8 OPTIONS
Accept: application/sdp, application/pkcs7-mime,
        multipart/mixed, multipart/signed,
        message/sip, message/sipfrag
Via: SIP/2.0/UDP 192.0.2.1;branch=z9hG4bKkdjuw
l: 0


*/
   FILE* fid= fopen("semiuri.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Request Line
   //OPTIONS sip:user;par=u%40example.net@example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   // ?bwc? Need to determine how escaped stuff should be represented internally
   tassert(msg->header(resip::h_RequestLine).uri().user()=="user;par=u%40example.net");
   tassert_reset();
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: sip:j_user@example.com
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="j_user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: sip:caller@example.org;tag=33242
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="33242");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="caller");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.org");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Max-Forwards: 3
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==3);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Call-ID: semiuri.0ha0isndaksdj
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="semiuri.0ha0isndaksdj");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 8 OPTIONS
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_CSeq).sequence()==8);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //Accept: application/sdp, application/pkcs7-mime,
   //        multipart/mixed, multipart/signed,
   //        message/sip, message/sipfrag
   tassert(msg->exists(resip::h_Accepts));
   tassert(msg->header(resip::h_Accepts).size()==6);
   
   resip::Mimes::iterator a=msg->header(resip::h_Accepts).begin();
   
   tassert(a->type()=="application");
   tassert(a->subType()=="sdp");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);
   a++;

   tassert(a->type()=="application");
   tassert(a->subType()=="pkcs7-mime");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);
   a++;

   tassert(a->type()=="multipart");
   tassert(a->subType()=="mixed");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);
   a++;

   tassert(a->type()=="multipart");
   tassert(a->subType()=="signed");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);
   a++;

   tassert(a->type()=="message");
   tassert(a->subType()=="sip");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);
   a++;

   tassert(a->type()=="message");
   tassert(a->subType()=="sipfrag");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);

   //Via: SIP/2.0/UDP 192.0.2.1;branch=z9hG4bKkdjuw
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator i=msg->header(resip::h_Vias).begin();
   
   tassert(i->numKnownParams()==1);
   tassert(i->numUnknownParams()==0);
   tassert(i->protocolName()=="SIP");
   tassert(i->protocolVersion()=="2.0");
   tassert(i->transport()=="UDP");
   tassert(i->sentHost()=="192.0.2.1");
   tassert(i->sentPort()==0);
   
   tassert(i->exists(resip::p_branch));
   tassert(i->param(resip::p_branch).hasMagicCookie());
   tassert(i->param(resip::p_branch).getTransactionId()=="kdjuw");
   tassert(i->param(resip::p_branch).clientData().empty());

   //l: 0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);



   InfoLog(<< "In case semiuri:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
transports()
{
/*
   This request contains Via header field values with all known
   transport types and exercises the transport extension mechanism.
   Parsers must accept this message as well formed.  Elements receiving
   this message would process it exactly as if the 2nd and subsequent
   header field values specified UDP (or other transport).

OPTIONS sip:user@example.com SIP/2.0
To: sip:user@example.com
From: <sip:caller@example.com>;tag=323
Max-Forwards: 70
Call-ID:  transports.kijh4akdnaqjkwendsasfdj
Accept: application/sdp
CSeq: 60 OPTIONS
Via: SIP/2.0/UDP t1.example.com;branch=z9hG4bKkdjuw
Via: SIP/2.0/SCTP t2.example.com;branch=z9hG4bKklasjdhf
Via: SIP/2.0/TLS t3.example.com;branch=z9hG4bK2980unddj
Via: SIP/2.0/UNKNOWN t4.example.com;branch=z9hG4bKasd0f3en
Via: SIP/2.0/TCP t5.example.com;branch=z9hG4bK0a9idfnee
l: 0


*/
   FILE* fid= fopen("transports.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Request Line
   //OPTIONS sip:user@example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="user");
   tassert_reset();
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: sip:user@example.com
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: <sip:caller@example.com>;tag=323
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="323");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="caller");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Max-Forwards: 70
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==70);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Call-ID:  transports.kijh4akdnaqjkwendsasfdj
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="transports.kijh4akdnaqjkwendsasfdj");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //Accept: application/sdp
   tassert(msg->exists(resip::h_Accepts));
   tassert(msg->header(resip::h_Accepts).size()==1);
   
   resip::Mimes::iterator a=msg->header(resip::h_Accepts).begin();
   
   tassert(a->type()=="application");
   tassert(a->subType()=="sdp");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);

   //CSeq: 60 OPTIONS
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_CSeq).sequence()==60);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==5);
   resip::ParserContainer<resip::Via>::iterator v=msg->header(resip::h_Vias).begin();

   //Via: SIP/2.0/UDP t1.example.com;branch=z9hG4bKkdjuw
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="UDP");
   tassert(v->sentHost()=="t1.example.com");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="kdjuw");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   v++;
   //Via: SIP/2.0/SCTP t2.example.com;branch=z9hG4bKklasjdhf
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="SCTP");
   tassert(v->sentHost()=="t2.example.com");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="klasjdhf");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   v++;
   //Via: SIP/2.0/TLS t3.example.com;branch=z9hG4bK2980unddj
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="TLS");
   tassert(v->sentHost()=="t3.example.com");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="2980unddj");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   v++;
   //Via: SIP/2.0/UNKNOWN t4.example.com;branch=z9hG4bKasd0f3en
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="UNKNOWN");
   tassert(v->sentHost()=="t4.example.com");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="asd0f3en");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   v++;
   //Via: SIP/2.0/TCP t5.example.com;branch=z9hG4bK0a9idfnee
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="TCP");
   tassert(v->sentHost()=="t5.example.com");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="0a9idfnee");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   //l: 0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);

   InfoLog(<< "In case transports:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
mpart01()
{
#if 0
/*
   This MESSAGE request contains two body parts.  The second part is
   binary encoded and contains null (0x00) characters.  Receivers must
   take care to frame the received message properly.

   Parsers must accept this message as well formed, even if the
   application above the parser does not support multipart/signed.

   Additional examples of multipart/mime messages, in particular S/MIME
   messages, are available in the security call flow examples document
   [SIP-SEC].

MESSAGE sip:kumiko@example.org SIP/2.0
Via: SIP/2.0/UDP 127.0.0.1:5070;branch=z9hG4bK-d87543-4dade06d0bdb11ee-1--d87543-;rport
Max-Forwards: 70
Route: <sip:127.0.0.1:5080>
Identity: r5mwreLuyDRYBi/0TiPwEsY3rEVsk/G2WxhgTV1PF7hHuLIK0YWVKZhKv9Mj8UeXqkMVbnVq37CD+813gvYjcBUaZngQmXc9WNZSDNGCzA+fWl9MEUHWIZo1CeJebdY/XlgKeTa0Olvq0rt70Q5jiSfbqMJmQFteeivUhkMWYUA=
Contact: <sip:fluffy@127.0.0.1:5070>
To: <sip:kumiko@example.org>
From: <sip:fluffy@example.com>;tag=2fb0dcc9
Call-ID: 3d9485ad0c49859b@Zmx1ZmZ5LW1hYy0xNi5sb2NhbA..
CSeq: 1 MESSAGE
Content-Transfer-Encoding: binary
Content-Type: multipart/mixed;boundary=7a9cbec02ceef655
Date: Sat, 15 Oct 2005 04:44:56 GMT
User-Agent: SIPimp.org/0.2.5 (curses)
Content-Length: 553

--7a9cbec02ceef655
Content-Type: text/plain
Content-Transfer-Encoding: binary

Hello
--7a9cbec02ceef655
Content-Type: application/octet-stream
Content-Transfer-Encoding: binary

0ÇR	*ÜHÜ˜
†ÇC0Ç?1	0+0	*ÜHÜ˜
1Ç 0Ç0|0p10	UUS10U
California10USan Jose10U
sipit1)0'U Sipit Test Certificate Authorityï q30+0
	*ÜHÜ˜
 ÅÄéÙf˘HR-“Âóéùï™ÈÚ˛†fYqbíË⁄*®ÿ5
hŒˇÆ<Ω+ˇu›’déY=÷G(Úb ˜ÈAtû3
öÌ´€ì—B.{râ“ú¿…Æ.˚«¿œ˘/;~O¿'·Tm‰∂™:ª>fÃÀ]÷∆KÉÉú∏Êˇ-îOÂ{eºô–
--7a9cbec02ceef655--

*/
#endif

   FILE* fid= fopen("mpart01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Request Line
   //MESSAGE sip:kumiko@example.org SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::MESSAGE);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="MESSAGE");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="kumiko");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.org");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator v=msg->header(resip::h_Vias).begin();

   //Via: SIP/2.0/UDP 127.0.0.1:5070;branch=z9hG4bK-d87543-4dade06d0bdb11ee-1--d87543-;rport
   tassert(v->numKnownParams()==2);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="UDP");
   tassert(v->sentHost()=="127.0.0.1");
   tassert(v->sentPort()==5070);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   // .bwc. This branch parameter has old resip-specific tokens in it. The 
   // "d87543" used to be the resip cookie, but the resip cookie has since 
   // changed. So, the whole branch param is taken as the transaction id.
   tassert(v->param(resip::p_branch).getTransactionId()==
                                 "-d87543-4dade06d0bdb11ee-1--d87543-");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   tassert(v->exists(resip::p_rport));
   tassert(!(v->param(resip::p_rport).hasValue()));
   tassert(v->param(resip::p_rport).port()==0);

   //Max-Forwards: 70
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==70);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Route: <sip:127.0.0.1:5080>
   tassert(msg->exists(resip::h_Routes));
   tassert(msg->header(resip::h_Routes).size()==1);
   resip::ParserContainer<resip::NameAddr>::iterator r=msg->header(resip::h_Routes).begin();
   
   tassert(r->numKnownParams()==0);
   tassert(r->numUnknownParams()==0);
   tassert(r->displayName()=="");
   tassert(r->uri().scheme()=="sip");
   tassert(r->uri().user()=="");
   tassert(r->uri().password()=="");
   tassert(r->uri().host()=="127.0.0.1");
   tassert(r->uri().port()==5080);
   tassert(!(r->uri().hasEmbedded()));
   tassert(r->uri().numKnownParams()==0);
   tassert(r->uri().numUnknownParams()==0);
   

   //Identity: r5mwreLuyDRYBi/0TiPwEsY3rEVsk/G2WxhgTV1PF7hHuLIK0YWVKZhKv9Mj8UeXqkMVbnVq37CD+813gvYjcBUaZngQmXc9WNZSDNGCzA+fWl9MEUHWIZo1CeJebdY/XlgKeTa0Olvq0rt70Q5jiSfbqMJmQFteeivUhkMWYUA=
   tassert(msg->exists(resip::h_Identity));
   tassert(msg->header(resip::h_Identity).value()=="r5mwreLuyDRYBi/0TiPwEsY3rEVsk/G2WxhgTV1PF7hHuLIK0YWVKZhKv9Mj8UeXqkMVbnVq37CD+813gvYjcBUaZngQmXc9WNZSDNGCzA+fWl9MEUHWIZo1CeJebdY/XlgKeTa0Olvq0rt70Q5jiSfbqMJmQFteeivUhkMWYUA=");
   tassert(msg->header(resip::h_Identity).numKnownParams()==0);
   tassert(msg->header(resip::h_Identity).numUnknownParams()==0);
   
   //Contact: <sip:fluffy@127.0.0.1:5070>
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   resip::ParserContainer<resip::NameAddr>::iterator m=msg->header(resip::h_Contacts).begin();
   
   tassert(m->displayName()=="");
   tassert(m->numKnownParams()==0);
   tassert(m->numUnknownParams()==0);
   tassert(!(m->isAllContacts()));
   tassert(m->uri().numKnownParams()==0);
   tassert(m->uri().numUnknownParams()==0);
   tassert(m->uri().scheme()=="sip");
   tassert(m->uri().user()=="fluffy");
   tassert(m->uri().password().empty());
   tassert(m->uri().host()=="127.0.0.1");
   tassert(m->uri().port()==5070);
   tassert(!(m->uri().hasEmbedded()));

   //To: <sip:kumiko@example.org>
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="kumiko");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.org");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: <sip:fluffy@example.com>;tag=2fb0dcc9
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="2fb0dcc9");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="fluffy");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Call-ID: 3d9485ad0c49859b@Zmx1ZmZ5LW1hYy0xNi5sb2NhbA..
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="3d9485ad0c49859b@Zmx1ZmZ5LW1hYy0xNi5sb2NhbA..");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 1 MESSAGE
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::MESSAGE);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="MESSAGE");
   tassert(msg->header(resip::h_CSeq).sequence()==1);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   // .bwc. There appears to be some controversy over how this particular
   // header should be implemented...
   //Content-Transfer-Encoding: binary
   tassert(msg->exists(resip::h_ContentTransferEncoding));
   tassert(msg->header(resip::h_ContentTransferEncoding).value()=="binary");
   tassert(msg->header(resip::h_ContentTransferEncoding).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentTransferEncoding).numUnknownParams()==0);
   
   //Content-Type: multipart/mixed;boundary=7a9cbec02ceef655
   tassert(msg->exists(resip::h_ContentType));
   tassert(msg->header(resip::h_ContentType).type()=="multipart");
   tassert(msg->header(resip::h_ContentType).subType()=="mixed");
   tassert(msg->header(resip::h_ContentType).numKnownParams()==1);
   tassert(msg->header(resip::h_ContentType).numUnknownParams()==0);
   
   tassert(msg->header(resip::h_ContentType).exists(resip::p_boundary));
   tassert(msg->header(resip::h_ContentType).param(resip::p_boundary)=="7a9cbec02ceef655");

   //Date: Sat, 15 Oct 2005 04:44:56 GMT
   tassert(msg->exists(resip::h_Date));
   tassert(msg->header(resip::h_Date).dayOfWeek()==resip::Sat);
   tassert(msg->header(resip::h_Date).dayOfMonth()==15);
   tassert(msg->header(resip::h_Date).month()==resip::Oct);
   tassert(msg->header(resip::h_Date).year()==2005);
   tassert(msg->header(resip::h_Date).hour()==4);
   tassert(msg->header(resip::h_Date).minute()==44);
   tassert(msg->header(resip::h_Date).second()==56);
   tassert(msg->header(resip::h_Date).numKnownParams()==0);
   tassert(msg->header(resip::h_Date).numUnknownParams()==0);
   
   //User-Agent: SIPimp.org/0.2.5 (curses)
   tassert(msg->exists(resip::h_UserAgent));
   tassert(msg->header(resip::h_UserAgent).value()=="SIPimp.org/0.2.5 (curses)");
   tassert(msg->header(resip::h_UserAgent).numKnownParams()==0);
   tassert(msg->header(resip::h_UserAgent).numUnknownParams()==0);
   
   //Content-Length: 553
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==553);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);


   InfoLog(<< "In case mpart01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
unreason()
{
/*
   This 200 response contains a reason phrase other than "OK".  The
   reason phrase is intended for human consumption and may contain any
   string produced by

       Reason-Phrase   =  *(reserved / unreserved / escaped
                          / UTF8-NONASCII / UTF8-CONT / SP / HTAB)

   This particular response contains unreserved and non-ascii UTF-8
   characters.  This response is well formed.  A parser must accept this
   message.


SIP/2.0 200 = 2**3 * 5**2 –Ω–æ —Å—Ç–æ –¥–µ–≤—è–Ω–æ—Å—Ç–æ –¥–µ–≤—è—Ç—å - –ø—Ä–æ—Å—Ç–æ–µ
Via: SIP/2.0/UDP 192.0.2.198;branch=z9hG4bK1324923
Call-ID: unreason.1234ksdfak3j2erwedfsASdf
CSeq: 35 INVITE
From: sip:user@example.com;tag=11141343
To: sip:user@example.edu;tag=2229
Content-Length: 154
Content-Type: application/sdp
Contact: <sip:user@host198.example.com>

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.198
s=-
c=IN IP4 192.0.2.198
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("unreason.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }
   
   //Status Line
   //SIP/2.0 200 = 2**3 * 5**2 –Ω–æ —Å—Ç–æ –¥–µ–≤—è–Ω–æ—Å—Ç–æ –¥–µ–≤—è—Ç—å - –ø—Ä–æ—Å—Ç–æ–µ
   tassert(msg->header(resip::h_StatusLine).responseCode()==200);
   tassert(msg->header(resip::h_StatusLine).getSipVersion()=="SIP/2.0");
   
   resip::Data binaryReason("= 2**3 * 5**2 ");
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBD;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBE;
   binaryReason+=(char)0x20;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x81;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x82;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBE;
   binaryReason+=(char)0x20;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB4;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB5;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB2;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x8F;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBD;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBE;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x81;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x82;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBE;
   binaryReason+=(char)0x20;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB4;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB5;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB2;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x8F;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x82;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x8C;
   binaryReason+=(char)0x20;
   binaryReason+=(char)0x2D;
   binaryReason+=(char)0x20;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBF;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x80;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBE;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x81;
   binaryReason+=(char)0xD1;
   binaryReason+=(char)0x82;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xBE;
   binaryReason+=(char)0xD0;
   binaryReason+=(char)0xB5;   
   tassert(msg->header(resip::h_StatusLine).reason()==binaryReason);
   
   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator v=msg->header(resip::h_Vias).begin();

   //Via: SIP/2.0/UDP 192.0.2.198;branch=z9hG4bK1324923
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="UDP");
   tassert(v->sentHost()=="192.0.2.198");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="1324923");
   tassert(v->param(resip::p_branch).clientData().empty());
   
   //Call-ID: unreason.1234ksdfak3j2erwedfsASdf
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="unreason.1234ksdfak3j2erwedfsASdf");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 35 INVITE
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::INVITE);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_CSeq).sequence()==35);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //From: sip:user@example.com;tag=11141343
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="11141343");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="user");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //To: sip:user@example.edu;tag=2229
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==1);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(msg->header(resip::h_To).exists(resip::p_tag));
   tassert(msg->header(resip::h_To).param(resip::p_tag)=="2229");
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.edu");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //Content-Length: 154
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==154);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);

   //Content-Type: application/sdp
   tassert(msg->exists(resip::h_ContentType));
   tassert(msg->header(resip::h_ContentType).type()=="application");
   tassert(msg->header(resip::h_ContentType).subType()=="sdp");
   tassert(msg->header(resip::h_ContentType).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentType).numUnknownParams()==0);

   //Contact: <sip:user@host198.example.com>
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   resip::ParserContainer<resip::NameAddr>::iterator m=msg->header(resip::h_Contacts).begin();
   
   tassert(m->displayName()=="");
   tassert(m->numKnownParams()==0);
   tassert(m->numUnknownParams()==0);
   tassert(!(m->isAllContacts()));
   tassert(m->uri().numKnownParams()==0);
   tassert(m->uri().numUnknownParams()==0);
   tassert(m->uri().scheme()=="sip");
   tassert(m->uri().user()=="user");
   tassert(m->uri().password().empty());
   tassert(m->uri().host()=="host198.example.com");
   tassert(m->uri().port()==0);
   tassert(!(m->uri().hasEmbedded()));


   InfoLog(<< "In case unreason:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
noreason()
{
/*

   This well-formed response contains no reason phrase.  A parser must
   accept this message.  The space character after the reason code is
   required.  If it were not present, this message could be rejected as
   invalid (a liberal receiver would accept it anyway).

SIP/2.0 100 
Via: SIP/2.0/UDP 192.0.2.105;branch=z9hG4bK2398ndaoe
Call-ID: noreason.asndj203insdf99223ndf
CSeq: 35 INVITE
From: <sip:user@example.com>;tag=39ansfi3
To: <sip:user@example.edu>;tag=902jndnke3
Content-Length: 0
Contact: <sip:user@host105.example.com>


*/
   FILE* fid= fopen("noreason.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Status Line
   //SIP/2.0 100 
   tassert(msg->header(resip::h_StatusLine).responseCode()==100);
   tassert(msg->header(resip::h_StatusLine).getSipVersion()=="SIP/2.0");
   tassert(msg->header(resip::h_StatusLine).reason()=="");

   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator v=msg->header(resip::h_Vias).begin();

   //Via: SIP/2.0/UDP 192.0.2.105;branch=z9hG4bK2398ndaoe
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="UDP");
   tassert(v->sentHost()=="192.0.2.105");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="2398ndaoe");
   tassert(v->param(resip::p_branch).clientData().empty());

   //Call-ID: noreason.asndj203insdf99223ndf
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="noreason.asndj203insdf99223ndf");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 35 INVITE
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::INVITE);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="INVITE");
   tassert(msg->header(resip::h_CSeq).sequence()==35);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //From: <sip:user@example.com>;tag=39ansfi3
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="39ansfi3");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="user");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.com");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //To: <sip:user@example.edu>;tag=902jndnke3
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==1);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(msg->header(resip::h_To).exists(resip::p_tag));
   tassert(msg->header(resip::h_To).param(resip::p_tag)=="902jndnke3");
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.edu");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //Content-Length: 0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);

   //Contact: <sip:user@host105.example.com>
   tassert(msg->exists(resip::h_Contacts));
   tassert(msg->header(resip::h_Contacts).size()==1);
   resip::ParserContainer<resip::NameAddr>::iterator m=msg->header(resip::h_Contacts).begin();
   
   tassert(m->displayName()=="");
   tassert(m->numKnownParams()==0);
   tassert(m->numUnknownParams()==0);
   tassert(!(m->isAllContacts()));
   tassert(m->uri().numKnownParams()==0);
   tassert(m->uri().numUnknownParams()==0);
   tassert(m->uri().scheme()=="sip");
   tassert(m->uri().user()=="user");
   tassert(m->uri().password().empty());
   tassert(m->uri().host()=="host105.example.com");
   tassert(m->uri().port()==0);
   tassert(!(m->uri().hasEmbedded()));


   InfoLog(<< "In case noreason:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
badinv01()
{
/*

   The Via header field of this request contains additional semicolons
   and commas without parameters or values.  The Contact header field
   contains additional semicolons without parameters.  This message is
   syntactically invalid.

   An element receiving this request should respond with a 400 Bad
   Request error.

INVITE sip:user@example.com SIP/2.0
To: sip:j.user@example.com
From: sip:caller@example.net;tag=134161461246
Max-Forwards: 7
Call-ID: badinv01.0ha0isndaksdjasdf3234nas
CSeq: 8 INVITE
Via: SIP/2.0/UDP 192.0.2.15;;,;,,
Contact: "Joe" <sip:joe@example.org>;;;;
Content-Length: 152
Content-Type: application/sdp

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.15
s=-
c=IN IP4 192.0.2.15
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("badinv01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case badinv01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
clerr()
{
/*

   This is a request message with a Content Length that is larger than
   the actual length of the body.

   When sent over UDP (as this message ostensibly was), the receiving
   element should respond with a 400 Bad Request error.  If this message
   arrived over a stream-based transport, such as TCP, there's not much
   the receiving party could do but wait for more data on the stream and
   close the connection if none is forthcoming within a reasonable
   period of time.

INVITE sip:user@example.com SIP/2.0
Max-Forwards: 80
To: sip:j.user@example.com
From: sip:caller@example.net;tag=93942939o2
Contact: <sip:caller@hungry.example.net>
Call-ID: clerr.0ha0isndaksdjweiafasdk3
CSeq: 8 INVITE
Via: SIP/2.0/UDP host5.example.com;branch=z9hG4bK-39234-23523
Content-Type: application/sdp
Content-Length: 9999

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.155
s=-
c=IN IP4 192.0.2.155
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("clerr.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();

   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case clerr:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
ncl()
{
/*

   This request has a negative value for Content-Length.

   An element receiving this message should respond with an error.  This
   request appeared over UDP, so the remainder of the datagram can
   simply be discarded.  If a request like this arrives over TCP, the
   framing error is not recoverable, and the connection should be
   closed.  The same behavior is appropriate for messages that arrive
   without a numeric value in the Content-Length header field, such as
   the following:

      Content-Length: five

   Implementors should take extra precautions if the technique they
   choose for converting this ascii field into an integral form can
   return a negative value.  In particular, the result must not be used
   as a counter or array index.


INVITE sip:user@example.com SIP/2.0
Max-Forwards: 254
To: sip:j.user@example.com
From: sip:caller@example.net;tag=32394234
Call-ID: ncl.0ha0isndaksdj2193423r542w35
CSeq: 0 INVITE
Via: SIP/2.0/UDP 192.0.2.53;branch=z9hG4bKkdjuw
Contact: <sip:caller@example53.example.net>
Content-Type: application/sdp
Content-Length: -999

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.53
s=-
c=IN IP4 192.0.2.53
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("ncl.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case ncl:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
scalar02()
{
/*

   This request contains several scalar header field values outside
   their legal range.

      o  The CSeq sequence number is >2**32-1.

      o  The Max-Forwards value is >255.

      o  The Expires value is >2**32-1.

      o  The Contact expires parameter value is >2**32-1.

   An element receiving this request should respond with a 400 Bad
   Request due to the CSeq error.  If only the Max-Forwards field were
   in error, the element could choose to process the request as if the
   field were absent.  If only the expiry values were in error, the
   element could treat them as if they contained the default values for
   expiration (3600 in this case).

   Other scalar request fields that may contain aberrant values include,
   but are not limited to, the Contact q value, the Timestamp value, and
   the Via ttl parameter.


REGISTER sip:example.com SIP/2.0
Via: SIP/2.0/TCP host129.example.com;branch=z9hG4bK342sdfoi3
To: <sip:user@example.com>
From: <sip:user@example.com>;tag=239232jh3
CSeq: 36893488147419103232 REGISTER
Call-ID: scalar02.23o0pd9vanlq3wnrlnewofjas9ui32
Max-Forwards: 300
Expires: 10000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000
Contact: <sip:user@host129.example.com>
  ;expires=280297596632815
Content-Length: 0


*/
   FILE* fid= fopen("scalar02.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case scalar02:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
scalarlg()
{
/*

   This response contains several scalar header field values outside
   their legal range.

   o  The CSeq sequence number is >2**32-1.

   o  The Retry-After field is unreasonably large (note that RFC 3261
      does not define a legal range for this field).

   o  The Warning field has a warning-value with more than 3 digits.

   An element receiving this response will simply discard it.

SIP/2.0 503 Service Unavailable
Via: SIP/2.0/TCP host129.example.com;branch=z9hG4bKzzxdiwo34sw;received=192.0.2.129
To: <sip:user@example.com>
From: <sip:other@example.net>;tag=2easdjfejw
CSeq: 9292394834772304023312 OPTIONS
Call-ID: scalarlg.noase0of0234hn2qofoaf0232aewf2394r
Retry-After: 949302838503028349304023988
Warning: 1812 overture "In Progress"
Content-Length: 0


*/
   FILE* fid= fopen("scalarlg.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case scalarlg:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
quotbal()
{
/*

   This is a request with an unterminated quote in the display name of
   the To field.  An element receiving this request should return a 400
   Bad Request error.

   An element could attempt to infer a terminating quote and accept the
   message.  Such an element needs to take care that it makes a
   reasonable inference when it encounters

      To: "Mr J. User <sip:j.user@example.com> <sip:realj@example.net>

INVITE sip:user@example.com SIP/2.0
To: "Mr. J. User <sip:j.user@example.com>
From: sip:caller@example.net;tag=93334
Max-Forwards: 10
Call-ID: quotbal.aksdj
Contact: <sip:caller@host59.example.net>
CSeq: 8 INVITE
Via: SIP/2.0/UDP 192.0.2.59:5050;branch=z9hG4bKkdjuw39234
Content-Type: application/sdp
Content-Length: 152

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.15
s=-
c=IN IP4 192.0.2.15
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("quotbal.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case quotbal:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
ltgtruri()
{
/*

   This INVITE request is invalid because the Request-URI has been
   enclosed within in "<>".

   It is reasonable always to reject a request with this error with a
   400 Bad Request.  Elements attempting to be liberal with what they
   accept may choose to ignore the brackets.  If the element forwards
   the request, it must not include the brackets in the messages it
   sends.

INVITE <sip:user@example.com> SIP/2.0
To: sip:user@example.com
From: sip:caller@example.net;tag=39291
Max-Forwards: 23
Call-ID: ltgtruri.1@192.0.2.5
CSeq: 1 INVITE
Via: SIP/2.0/UDP 192.0.2.5
Contact: <sip:caller@host5.example.net>
Content-Type: application/sdp
Content-Length: 159

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.5
s=-
c=IN IP4 192.0.2.5
t=3149328700 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("ltgtruri.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case ltgtruri:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
lwsruri()
{
/*
   This INVITE has illegal LWS within the Request-URI.

   An element receiving this request should respond with a 400 Bad
   Request.

   An element could attempt to ignore the embedded LWS for those schemes
   (like SIP) where doing so would not introduce ambiguity.

INVITE sip:user@example.com; lr SIP/2.0
To: sip:user@example.com;tag=3xfe-9921883-z9f
From: sip:caller@example.net;tag=231413434
Max-Forwards: 5
Call-ID: lwsruri.asdfasdoeoi2323-asdfwrn23-asd834rk423
CSeq: 2130706432 INVITE
Via: SIP/2.0/UDP 192.0.2.1:5060;branch=z9hG4bKkdjuw2395
Contact: <sip:caller@host1.example.net>
Content-Type: application/sdp
Content-Length: 159

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=3149328700 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("lwsruri.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case lwsruri:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
lwsstart()
{
/*

   This INVITE has illegal multiple SP characters between elements of
   the start line.

   It is acceptable to reject this request as malformed.  An element
   that is liberal in what it accepts may ignore these extra SP
   characters when processing the request.  If the element forwards the
   request, it must not include these extra SP characters in the
   messages it sends.

INVITE  sip:user@example.com  SIP/2.0
Max-Forwards: 8
To: sip:user@example.com
From: sip:caller@example.net;tag=8814
Call-ID: lwsstart.dfknq234oi243099adsdfnawe3@example.com
CSeq: 1893884 INVITE
Via: SIP/2.0/UDP host1.example.com;branch=z9hG4bKkdjuw3923
Contact: <sip:caller@host1.example.net>
Content-Type: application/sdp
Content-Length: 150

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("lwsstart.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case lwsstart:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
trws()
{
/*

   This OPTIONS request contains SP characters between the SIP-Version
   field and the CRLF terminating the Request-Line.

   It is acceptable to reject this request as malformed.  An element
   that is liberal in what it accepts may ignore these extra SP
   characters when processing the request.  If the element forwards the
   request, it must not include these extra SP characters in the
   messages it sends.

OPTIONS sip:remote-target@example.com SIP/2.0  
Via: SIP/2.0/TCP host1.examle.com;branch=z9hG4bK299342093
To: <sip:remote-target@example.com>
From: <sip:local-resource@example.com>;tag=329429089
Call-ID: trws.oicu34958239neffasdhr2345r
Accept: application/sdp
CSeq: 238923 OPTIONS
Max-Forwards: 70
Content-Length: 0


*/
   FILE* fid= fopen("trws.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case trws:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
escruri()
{
/*

   This INVITE is malformed, as the SIP Request-URI contains escaped
   headers.

   It is acceptable for an element to reject this request with a 400 Bad
   Request.  An element could choose to be liberal in what it accepts
   and ignore the escaped headers.  If the element is a proxy, the
   escaped headers must not appear in the Request-URI of the forwarded
   request (and most certainly must not be translated into the actual
   header of the forwarded request).

INVITE sip:user@example.com?Route=%3Csip:example.com%3E SIP/2.0
To: sip:user@example.com
From: sip:caller@example.net;tag=341518
Max-Forwards: 7
Contact: <sip:caller@host39923.example.net>
Call-ID: escruri.23940-asdfhj-aje3br-234q098w-fawerh2q-h4n5
CSeq: 149209342 INVITE
Via: SIP/2.0/UDP host-of-the-hour.example.com;branch=z9hG4bKkdjuw
Content-Type: application/sdp
Content-Length: 150

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.1
s=-
c=IN IP4 192.0.2.1
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("escruri.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case escruri:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
baddate()
{
/*

   This INVITE is invalid, as it contains a non-GMT time zone in the SIP
   Date header field.

   It is acceptable to reject this request as malformed (though an
   element shouldn't do that unless the contents of the Date header
   field were actually important to its processing).  An element wishing
   to be liberal in what it accepts could ignore this value altogether
   if it wasn't going to use the Date header field anyway.  Otherwise,
   it could attempt to interpret this date and adjust it to GMT.

   RFC 3261 explicitly defines the only acceptable time zone designation
   as "GMT".  "UT", while synonymous with GMT per [RFC2822], is not
   valid.  "UTC" and "UCT" are also invalid.

INVITE sip:user@example.com SIP/2.0
To: sip:user@example.com
From: sip:caller@example.net;tag=2234923
Max-Forwards: 70
Call-ID: baddate.239423mnsadf3j23lj42--sedfnm234
CSeq: 1392934 INVITE
Via: SIP/2.0/UDP host.example.com;branch=z9hG4bKkdjuw
Date: Fri, 01 Jan 2010 16:00:00 EST
Contact: <sip:caller@host5.example.net>
Content-Type: application/sdp
Content-Length: 150

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.5
s=-
c=IN IP4 192.0.2.5
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("baddate.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case baddate:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
regbadct()
{
/*

   This REGISTER request is malformed.  The SIP URI contained in the
   Contact Header field has an escaped header, so the field must be in
   name-addr form (which implies that the URI must be enclosed in <>).

   It is reasonable for an element receiving this request to respond
   with a 400 Bad Request.  An element choosing to be liberal in what it
   accepts could infer the angle brackets since there is no ambiguity in
   this example.  In general, that won't be possible.

REGISTER sip:example.com SIP/2.0
To: sip:user@example.com
From: sip:user@example.com;tag=998332
Max-Forwards: 70
Call-ID: regbadct.k345asrl3fdbv@10.0.0.1
CSeq: 1 REGISTER
Via: SIP/2.0/UDP 135.180.130.133:5060;branch=z9hG4bKkdjuw
Contact: sip:user@example.com?Route=%3Csip:sip.example.com%3E
l: 0


*/
   FILE* fid= fopen("regbadct.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case regbadct:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
badaspec()
{
/*

   This request is malformed, since the addr-spec in the To header field
   contains spaces.  Parsers receiving this request must not break.  It
   is reasonable to reject this request with a 400 Bad Request response.
   Elements attempting to be liberal may ignore the spaces.

OPTIONS sip:user@example.org SIP/2.0
Via: SIP/2.0/UDP host4.example.com:5060;branch=z9hG4bKkdju43234
Max-Forwards: 70
From: "Bell, Alexander" <sip:a.g.bell@example.com>;tag=433423
To: "Watson, Thomas" < sip:t.watson@example.org >
Call-ID: badaspec.sdf0234n2nds0a099u23h3hnnw009cdkne3
Accept: application/sdp
CSeq: 3923239 OPTIONS
l: 0


*/
   FILE* fid= fopen("badaspec.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }
   
   

   InfoLog(<< "In case badaspec:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
baddn()
{
/*

   This OPTIONS request is malformed, since the display names in the To
   and From header fields contain non-token characters but are unquoted.

   It is reasonable always to reject this kind of error with a 400 Bad
   Request response.

   An element may attempt to be liberal in what it receives and infer
   the missing quotes.  If this element were a proxy, it must not
   propagate the error into the request it forwards.  As a consequence,
   if the fields are covered by a signature, there's not much point in
   trying to be liberal - the message should simply be rejected.

OPTIONS sip:t.watson@example.org SIP/2.0
Via:     SIP/2.0/UDP c.example.com:5060;branch=z9hG4bKkdjuw
Max-Forwards:      70
From:    Bell, Alexander <sip:a.g.bell@example.com>;tag=43
To:      Watson, Thomas <sip:t.watson@example.org>
Call-ID: baddn.31415@c.example.com
Accept: application/sdp
CSeq:    3923239 OPTIONS
l: 0

*/
   FILE* fid= fopen("baddn.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case baddn:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
badvers()
{
/*

   To an element implementing [RFC3261], this request is malformed due
   to its high version number.

   The element should respond to the request with a 505 Version Not
   Supported error.

OPTIONS sip:t.watson@example.org SIP/7.0
Via:     SIP/7.0/UDP c.example.com;branch=z9hG4bKkdjuw
Max-Forwards:     70
From:    A. Bell <sip:a.g.bell@example.com>;tag=qweoiqpe
To:      T. Watson <sip:t.watson@example.org>
Call-ID: badvers.31417@c.example.com
CSeq:    1 OPTIONS
l: 0


*/
   FILE* fid= fopen("badvers.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case badvers:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
mismatch01()
{
/*

   This request has mismatching values for the method in the start line
   and the CSeq header field.  Any element receiving this request will
   respond with a 400 Bad Request.

OPTIONS sip:user@example.com SIP/2.0
To: sip:j.user@example.com
From: sip:caller@example.net;tag=34525
Max-Forwards: 6
Call-ID: mismatch01.dj0234sxdfl3
CSeq: 8 INVITE
Via: SIP/2.0/UDP host.example.com;branch=z9hG4bKkdjuw
l: 0


*/
   FILE* fid= fopen("mismatch01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case mismatch01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
mismatch02()
{
/*

   This message has an unknown method in the start line, and a CSeq
   method tag that does not match.

   Any element receiving this response should respond with a 501 Not
   Implemented.  A 400 Bad Request is also acceptable, but choosing a
   501 (particularly at proxies) has better future-proof
   characteristics.

NEWMETHOD sip:user@example.com SIP/2.0
To: sip:j.user@example.com
From: sip:caller@example.net;tag=34525
Max-Forwards: 6
Call-ID: mismatch02.dj0234sxdfl3
CSeq: 8 INVITE
Contact: <sip:caller@host.example.net>
Via: SIP/2.0/UDP host.example.net;branch=z9hG4bKkdjuw
Content-Type: application/sdp
l: 138

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.1
c=IN IP4 192.0.2.1
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("mismatch02.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case mismatch02:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
bigcode()
{
/*

   This response has a response code larger than 699.  An element
   receiving this response should simply drop it.

SIP/2.0 4294967301 better not break the receiver
Via: SIP/2.0/UDP 192.0.2.105;branch=z9hG4bK2398ndaoe
Call-ID: bigcode.asdof3uj203asdnf3429uasdhfas3ehjasdfas9i
CSeq: 353494 INVITE
From: <sip:user@example.com>;tag=39ansfi3
To: <sip:user@example.edu>;tag=902jndnke3
Content-Length: 0
Contact: <sip:user@host105.example.com>


*/
   FILE* fid= fopen("bigcode.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case bigcode:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
badbranch()
{
/*

   This request indicates support for RFC 3261-style transaction
   identifiers by providing the z9hG4bK prefix to the branch parameter,
   but it provides no identifier.  A parser must not break when
   receiving this message.  An element receiving this request could
   reject the request with a 400 Response (preferably statelessly, as
   other requests from the source are likely also to have a malformed
   branch parameter), or it could fall back to the RFC 2543-style
   transaction identifier.

OPTIONS sip:user@example.com SIP/2.0
To: sip:user@example.com
From: sip:caller@example.org;tag=33242
Max-Forwards: 3
Via: SIP/2.0/UDP 192.0.2.1;branch=z9hG4bK
Accept: application/sdp
Call-ID: badbranch.sadonfo23i420jv0as0derf3j3n
CSeq: 8 OPTIONS
l: 0


*/
   FILE* fid= fopen("badbranch.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   //Request Line
   //OPTIONS sip:user@example.com SIP/2.0
   tassert(msg->header(resip::h_RequestLine).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_RequestLine).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_RequestLine).uri().scheme()=="sip");
   tassert(msg->header(resip::h_RequestLine).uri().user()=="user");
   tassert(msg->header(resip::h_RequestLine).uri().password().empty());
   tassert(msg->header(resip::h_RequestLine).uri().host()=="example.com");
   tassert(msg->header(resip::h_RequestLine).uri().port()==0);
   tassert(!(msg->header(resip::h_RequestLine).uri().hasEmbedded()));
   tassert(msg->header(resip::h_RequestLine).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_RequestLine).uri().numUnknownParams()==0);
   tassert(msg->header(resip::h_RequestLine).getSipVersion()=="SIP/2.0");

   //To: sip:user@example.com
   tassert(msg->exists(resip::h_To));
   tassert(msg->header(resip::h_To).displayName()=="");
   tassert(msg->header(resip::h_To).numKnownParams()==0);
   tassert(msg->header(resip::h_To).numUnknownParams()==0);
   tassert(!(msg->header(resip::h_To).isAllContacts()));
   tassert(msg->header(resip::h_To).uri().scheme()=="sip");
   tassert(msg->header(resip::h_To).uri().user()=="user");
   tassert(msg->header(resip::h_To).uri().password().empty());
   tassert(msg->header(resip::h_To).uri().host()=="example.com");
   tassert(msg->header(resip::h_To).uri().port()==0);
   tassert(!(msg->header(resip::h_To).uri().hasEmbedded()));
   tassert(msg->header(resip::h_To).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_To).uri().numUnknownParams()==0);

   //From: sip:caller@example.org;tag=33242
   tassert(msg->exists(resip::h_From));
   tassert(msg->header(resip::h_From).displayName()=="");
   tassert(msg->header(resip::h_From).numKnownParams()==1);
   tassert(msg->header(resip::h_From).numUnknownParams()==0);
   tassert(msg->header(resip::h_From).exists(resip::p_tag));
   tassert(msg->header(resip::h_From).param(resip::p_tag)=="33242");
   tassert(!(msg->header(resip::h_From).isAllContacts()));
   tassert(msg->header(resip::h_From).uri().scheme()=="sip");
   tassert(msg->header(resip::h_From).uri().user()=="caller");
   tassert(msg->header(resip::h_From).uri().password().empty());
   tassert(msg->header(resip::h_From).uri().host()=="example.org");
   tassert(msg->header(resip::h_From).uri().port()==0);
   tassert(!(msg->header(resip::h_From).uri().hasEmbedded()));
   tassert(msg->header(resip::h_From).uri().numKnownParams()==0);
   tassert(msg->header(resip::h_From).uri().numUnknownParams()==0);

   //Max-Forwards: 3
   tassert(msg->exists(resip::h_MaxForwards));
   tassert(msg->header(resip::h_MaxForwards).value()==3);
   tassert(msg->header(resip::h_MaxForwards).numKnownParams()==0);
   tassert(msg->header(resip::h_MaxForwards).numUnknownParams()==0);

   //Vias
   tassert(msg->exists(resip::h_Vias));
   tassert(msg->header(resip::h_Vias).size()==1);
   resip::ParserContainer<resip::Via>::iterator v=msg->header(resip::h_Vias).begin();

   //Via: SIP/2.0/UDP 192.0.2.1;branch=z9hG4bK
   tassert(v->numKnownParams()==1);
   tassert(v->numUnknownParams()==0);
   tassert(v->protocolName()=="SIP");
   tassert(v->protocolVersion()=="2.0");
   tassert(v->transport()=="UDP");
   tassert(v->sentHost()=="192.0.2.1");
   tassert(v->sentPort()==0);
   
   tassert(v->exists(resip::p_branch));
   tassert(v->param(resip::p_branch).hasMagicCookie());
   tassert(v->param(resip::p_branch).getTransactionId()=="");
   tassert(v->param(resip::p_branch).clientData().empty());

   //Accept: application/sdp
   tassert(msg->exists(resip::h_Accepts));
   tassert(msg->header(resip::h_Accepts).size()==1);
   
   resip::Mimes::iterator a=msg->header(resip::h_Accepts).begin();
   
   tassert(a->type()=="application");
   tassert(a->subType()=="sdp");
   tassert(a->numKnownParams()==0);
   tassert(a->numUnknownParams()==0);

   //Call-ID: badbranch.sadonfo23i420jv0as0derf3j3n
   tassert(msg->exists(resip::h_CallID));
   tassert(msg->header(resip::h_CallID).value()=="badbranch.sadonfo23i420jv0as0derf3j3n");
   tassert(msg->header(resip::h_CallID).numKnownParams()==0);
   tassert(msg->header(resip::h_CallID).numUnknownParams()==0);

   //CSeq: 8 OPTIONS
   tassert(msg->exists(resip::h_CSeq));
   tassert(msg->header(resip::h_CSeq).method()==resip::OPTIONS);
   tassert(msg->header(resip::h_CSeq).unknownMethodName()=="OPTIONS");
   tassert(msg->header(resip::h_CSeq).sequence()==8);
   tassert(msg->header(resip::h_CSeq).numKnownParams()==0);
   tassert(msg->header(resip::h_CSeq).numUnknownParams()==0);

   //l: 0
   tassert(msg->exists(resip::h_ContentLength));
   tassert(msg->header(resip::h_ContentLength).value()==0);
   tassert(msg->header(resip::h_ContentLength).numKnownParams()==0);
   tassert(msg->header(resip::h_ContentLength).numUnknownParams()==0);


   InfoLog(<< "In case badbranch:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
bcast()
{
/*

SIP/2.0 200 OK
Via: SIP/2.0/UDP 192.0.2.198;branch=z9hG4bK1324923
Via: SIP/2.0/UDP 255.255.255.255;branch=z9hG4bK1saber23
Call-ID: bcast.0384840201234ksdfak3j2erwedfsASdf
CSeq: 35 INVITE
From: sip:user@example.com;tag=11141343
To: sip:user@example.edu;tag=2229
Content-Length: 154
Content-Type: application/sdp
Contact: <sip:user@host28.example.com>

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.198
s=-
c=IN IP4 192.0.2.198
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("bcast.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case bcast:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
bext01()
{
/*

OPTIONS sip:user@example.com SIP/2.0
To: sip:j_user@example.com
From: sip:caller@example.net;tag=242etr
Max-Forwards: 6
Call-ID: bext01.0ha0isndaksdj
Require: nothingSupportsThis, nothingSupportsThisEither
Proxy-Require: noProxiesSupportThis, norDoAnyProxiesSupportThis
CSeq: 8 OPTIONS
Via: SIP/2.0/TLS fold-and-staple.example.com;branch=z9hG4bKkdjuw
Content-Length: 0


*/
   FILE* fid= fopen("bext01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case bext01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
cparam01()
{
/*

REGISTER sip:example.com SIP/2.0
Via: SIP/2.0/UDP saturn.example.com:5060;branch=z9hG4bKkdjuw
Max-Forwards: 70
From: sip:watson@example.com;tag=DkfVgjkrtMwaerKKpe
To: sip:watson@example.com
Call-ID: cparam01.70710@saturn.example.com
CSeq: 2 REGISTER
Contact: sip:+19725552222@gw1.example.net;unknownparam
l: 0


*/
   FILE* fid= fopen("cparam01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case cparam01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
cparam02()
{
/*

REGISTER sip:example.com SIP/2.0
Via: SIP/2.0/UDP saturn.example.com:5060;branch=z9hG4bKkdjuw
Max-Forwards: 70
From: sip:watson@example.com;tag=838293
To: sip:watson@example.com
Call-ID: cparam02.70710@saturn.example.com
CSeq: 3 REGISTER
Contact: <sip:+19725552222@gw1.example.net;unknownparam>
l: 0


*/
   FILE* fid= fopen("cparam02.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case cparam02:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
insuf()
{
/*

INVITE sip:user@example.com SIP/2.0
CSeq: 193942 INVITE
Via: SIP/2.0/UDP 192.0.2.95;branch=z9hG4bKkdj.insuf
Content-Type: application/sdp
l: 152

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.95
s=-
c=IN IP4 192.0.2.95
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("insuf.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case insuf:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
inv2543()
{
/*

INVITE sip:UserB@example.com SIP/2.0
Via: SIP/2.0/UDP iftgw.example.com
From: <sip:+13035551111@ift.client.example.net;user=phone>
Record-Route: <sip:UserB@example.com;maddr=ss1.example.com>
To: sip:+16505552222@ss1.example.net;user=phone
Call-ID: inv2543.1717@ift.client.example.com
CSeq: 56 INVITE
Content-Type: application/sdp

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.5
s=-
c=IN IP4 192.0.2.5
t=0 0
m=audio 49217 RTP/AVP 0

*/
   FILE* fid= fopen("inv2543.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case inv2543:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
invut()
{
/*

INVITE sip:user@example.com SIP/2.0
Contact: <sip:caller@host5.example.net>
To: sip:j.user@example.com
From: sip:caller@example.net;tag=8392034
Max-Forwards: 70
Call-ID: invut.0ha0isndaksdjadsfij34n23d
CSeq: 235448 INVITE
Via: SIP/2.0/UDP somehost.example.com;branch=z9hG4bKkdjuw
Content-Type: application/unknownformat
Content-Length: 40

<audio>
 <pcmu port="443"/>
</audio>

*/
   FILE* fid= fopen("invut.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case invut:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
mcl01()
{
/*

OPTIONS sip:user@example.com SIP/2.0
Via: SIP/2.0/UDP host5.example.net;branch=z9hG4bK293423
To: sip:user@example.com
From: sip:other@example.net;tag=3923942
Call-ID: mcl01.fhn2323orihawfdoa3o4r52o3irsdf
CSeq: 15932 OPTIONS
Content-Length: 13
Max-Forwards: 60
Content-Length: 5
Content-Type: text/plain

There's no way to know how many octets are supposed to be here.


*/
   FILE* fid= fopen("mcl01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case mcl01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
multi01()
{
/*

INVITE sip:user@company.com SIP/2.0
Contact: <sip:caller@host25.example.net>
Via: SIP/2.0/UDP 192.0.2.25;branch=z9hG4bKkdjuw
Max-Forwards: 70
CSeq: 5 INVITE
Call-ID: multi01.98asdh@192.0.2.1
CSeq: 59 INVITE
Call-ID: multi01.98asdh@192.0.2.2
From: sip:caller@example.com;tag=3413415
To: sip:user@example.com
To: sip:other@example.net
From: sip:caller@example.net;tag=2923420123
Content-Type: application/sdp
l: 154
Contact: <sip:caller@host36.example.net>
Max-Forwards: 5

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.25
s=-
c=IN IP4 192.0.2.25
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC


*/
   FILE* fid= fopen("multi01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case multi01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
novelsc()
{
/*

OPTIONS soap.beep://192.0.2.103:3002 SIP/2.0
To: sip:user@example.com
From: sip:caller@example.net;tag=384
Max-Forwards: 3
Call-ID: novelsc.asdfasser0q239nwsdfasdkl34
CSeq: 3923423 OPTIONS
Via: SIP/2.0/TCP host9.example.com;branch=z9hG4bKkdjuw39234
Content-Length: 0


*/
   FILE* fid= fopen("novelsc.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case novelsc:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
regaut01()
{
/*

REGISTER sip:example.com SIP/2.0
To: sip:j.user@example.com
From: sip:j.user@example.com;tag=87321hj23128
Max-Forwards: 8
Call-ID: regaut01.0ha0isndaksdj
CSeq: 9338 REGISTER
Via: SIP/2.0/TCP 192.0.2.253;branch=z9hG4bKkdjuw
Authorization: NoOneKnowsThisScheme opaque-data=here
Content-Length:0


*/
   FILE* fid= fopen("regaut01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case regaut01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
regescrt()
{
/*

REGISTER sip:example.com SIP/2.0
To: sip:user@example.com
From: sip:user@example.com;tag=8
Max-Forwards: 70
Call-ID: regescrt.k345asrl3fdbv@192.0.2.1
CSeq: 14398234 REGISTER
Via: SIP/2.0/UDP host5.example.com;branch=z9hG4bKkdjuw
M: <sip:user@example.com?Route=%3Csip:sip.example.com%3E>
L:0


*/
   FILE* fid= fopen("regescrt.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case regescrt:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
sdp01()
{
/*

INVITE sip:user@example.com SIP/2.0
To: sip:j_user@example.com
Contact: <sip:caller@host15.example.net>
From: sip:caller@example.net;tag=234
Max-Forwards: 5
Call-ID: sdp01.ndaksdj9342dasdd
Accept: text/nobodyKnowsThis
CSeq: 8 INVITE
Via: SIP/2.0/UDP 192.0.2.15;branch=z9hG4bKkdjuw
Content-Length: 150
Content-Type: application/sdp

v=0
o=mhandley 29739 7272939 IN IP4 192.0.2.5
s=-
c=IN IP4 192.0.2.5
t=0 0
m=audio 49217 RTP/AVP 0 12
m=video 3227 RTP/AVP 31
a=rtpmap:31 LPC

*/
   FILE* fid= fopen("sdp01.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case sdp01:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
unkscm()
{
/*

OPTIONS nobodyKnowsThisScheme:totallyopaquecontent SIP/2.0
To: sip:user@example.com
From: sip:caller@example.net;tag=384
Max-Forwards: 3
Call-ID: unkscm.nasdfasser0q239nwsdfasdkl34
CSeq: 3923423 OPTIONS
Via: SIP/2.0/TCP host9.example.com;branch=z9hG4bKkdjuw39234
Content-Length: 0


*/
   FILE* fid= fopen("unkscm.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case unkscm:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
unksm2()
{
/*

REGISTER sip:example.com SIP/2.0
To: isbn:2983792873
From: <http://www.example.com>;tag=3234233
Call-ID: unksm2.daksdj@hyphenated-host.example.com
CSeq: 234902 REGISTER
Max-Forwards: 70
Via: SIP/2.0/UDP 192.0.2.21:5060;branch=z9hG4bKkdjuw
Contact: <name:John_Smith>
l: 0


*/
   FILE* fid= fopen("unksm2.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case unksm2:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


void
zeromf()
{
/*

OPTIONS sip:user@example.com SIP/2.0
To: sip:user@example.com
From: sip:caller@example.net;tag=3ghsd41
Call-ID: zeromf.jfasdlfnm2o2l43r5u0asdfas
CSeq: 39234321 OPTIONS
Via: SIP/2.0/UDP host1.example.com;branch=z9hG4bKkdjuw2349i
Max-Forwards: 0
Content-Length: 0


*/
   FILE* fid= fopen("zeromf.dat","r");
   tassert(fid);
   resip::Data txt;
   char mBuf[1024];
   int result;
   while(!feof(fid))
   {
      result = fread(&mBuf,1,1024,fid);
      txt += resip::Data(mBuf,result);
   }
   fclose(fid);
   resip::SipMessage* msg = resip::SipMessage::make(txt);
   tassert_reset();
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();

   resip::SipMessage copy(*msg);

   resip::Data encoded;
   {
      resip::oDataStream str(encoded);
      msg->encode(str);
   }
   resip::Data copyEncoded;
   {
      resip::oDataStream str(copyEncoded);
      copy.encode(str);
   }

   InfoLog(<< "In case zeromf:" );
   InfoLog(<< "Original text:" << std::endl << txt );
   InfoLog(<< "Encoded form:" << std::endl << encoded );
   InfoLog(<< "Encoded form of copy:" << std::endl << copyEncoded );




}


int main()
{

resip::SipMessage::checkContentLength=true;

resip::Log::initialize("cout", "DEBUG", "RFC4475TortureTests");
try
{
   wsinv();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case wsinv : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   intmeth();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case intmeth : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   esc01();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case esc01 : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   escnull();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case escnull : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   esc02();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case esc02 : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   lwsdisp();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case lwsdisp : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   longreq();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case longreq : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   dblreq();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case dblreq : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   semiuri();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case semiuri : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   transports();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case transports : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   mpart01();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case mpart01 : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   unreason();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case unreason : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   noreason();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case noreason : " << e );
   InfoLog(<< "This message was valid." );
}


try
{
   badinv01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case badinv01 : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   clerr();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case clerr : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   scalar02();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case scalar02 : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   scalarlg();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case scalarlg : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   quotbal();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case quotbal : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   ltgtruri();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case ltgtruri : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   lwsruri();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case lwsruri : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   lwsstart();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case lwsstart : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   trws();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case trws : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   escruri();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case escruri : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   baddate();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case baddate : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   regbadct();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case regbadct : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   badaspec();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case badaspec : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   baddn();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case baddn : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   badvers();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case badvers : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   mismatch01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case mismatch01 : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   mismatch02();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case mismatch02 : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   bigcode();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case bigcode : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   badbranch();
}
catch(resip::BaseException& e)
{
   tassert(0);
   tassert_reset();
   InfoLog(<< "Exception caught in test case badbranch : " << e );
   InfoLog(<< "This message was (syntactically) valid." );
}


try
{
   bcast();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case bcast : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   bext01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case bext01 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   cparam01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case cparam01 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   cparam02();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case cparam02 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   insuf();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case insuf : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   inv2543();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case inv2543 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   invut();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case invut : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   mcl01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case mcl01 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   multi01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case multi01 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   ncl();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case ncl : " << e );
   InfoLog(<< "This message wasn't valid." );
}


try
{
   novelsc();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case novelsc : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   regaut01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case regaut01 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   regescrt();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case regescrt : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   sdp01();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case sdp01 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   unkscm();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case unkscm : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   unksm2();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case unksm2 : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


try
{
   zeromf();
}
catch(resip::BaseException& e)
{
   InfoLog(<< "Exception caught in test case zeromf : " << e );
   InfoLog(<< "This message was/wasn't valid." );
}


}

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
