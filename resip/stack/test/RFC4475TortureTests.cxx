#include <iostream>
#include <memory>

#include "resip/stack/Contents.hxx"
#include "resip/stack/SdpContents.hxx"
#include "resip/stack/SipMessage.hxx"
#include "resip/stack/ExtensionHeader.hxx"
#include "resip/stack/Uri.hxx"
#include "rutil/Logger.hxx"
#include "tassert.h"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

void
badaspec()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
badbranch()
{
/*

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



}


void
baddate()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
baddn()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
badinv01()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
badvers()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
bigcode()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
clerr()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
dblreq()
{
/*

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



}


void
esc01()
{
/*

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



}


void
esc02()
{
/*

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



}


void
escnull()
{
/*

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



}


void
escruri()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
intmeth()
{
/*

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



}


void
longreq()
{
/*

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



}


void
ltgtruri()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
lwsdisp()
{
/*

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



}


void
lwsruri()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
lwsstart()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
mismatch01()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
mismatch02()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
mpart01()
{
/*

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



}


void
ncl()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
noreason()
{
/*

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



}


void
quotbal()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
regbadct()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
scalar02()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



}


void
scalarlg()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
semiuri()
{
/*

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



}


void
test()
{
/*

INVITE sip:alan@jasomi.com
TO : alan@jasomi.com
From: ralph@example.com
MaX-fOrWaRdS: 0068
Call-ID: test.0ha0isndaksdj@192.0.2.1
Xyzzy-2: this is the number ten : 10
Xyzzy-3: INVITE
Xyzzy: 10000000000
Meaning: foo bar spam
Foobar roobar
Content-Length: 18
Content-Type: application/sdp

v=0
testing=123

*/
   FILE* fid= fopen("test.dat","r");
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



}


void
transports()
{
/*

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



}


void
trws()
{
/*

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
   tassert(msg);
   tassert_reset();
   if(!msg)
   {
      return;
   }

   std::auto_ptr<resip::SipMessage> message(msg);
   msg->parseAllHeaders();



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



}


void
unreason()
{
/*

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



}


void
wsinv()
{
/*

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



}


int main()
{
try
{
   badaspec();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case badaspec : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   badbranch();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case badbranch : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   baddate();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case baddate : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   baddn();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case baddn : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   badinv01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case badinv01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   badvers();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case badvers : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   bcast();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case bcast : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   bext01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case bext01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   bigcode();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case bigcode : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   clerr();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case clerr : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   cparam01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case cparam01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   cparam02();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case cparam02 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   dblreq();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case dblreq : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   esc01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case esc01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   esc02();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case esc02 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   escnull();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case escnull : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   escruri();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case escruri : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   insuf();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case insuf : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   intmeth();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case intmeth : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   inv2543();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case inv2543 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   invut();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case invut : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   longreq();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case longreq : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   ltgtruri();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case ltgtruri : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   lwsdisp();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case lwsdisp : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   lwsruri();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case lwsruri : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   lwsstart();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case lwsstart : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   mcl01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case mcl01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   mismatch01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case mismatch01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   mismatch02();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case mismatch02 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   mpart01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case mpart01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   multi01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case multi01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   ncl();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case ncl : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   noreason();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case noreason : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   novelsc();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case novelsc : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   quotbal();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case quotbal : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   regaut01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case regaut01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   regbadct();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case regbadct : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   regescrt();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case regescrt : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   scalar02();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case scalar02 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   scalarlg();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case scalarlg : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   sdp01();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case sdp01 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   semiuri();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case semiuri : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   test();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case test : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   transports();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case transports : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   trws();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case trws : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   unkscm();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case unkscm : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   unksm2();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case unksm2 : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   unreason();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case unreason : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   wsinv();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case wsinv : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


try
{
   zeromf();
}
catch(resip::BaseException& e)
{
   std::cerr << "Exception caught in test case zeromf : " << e << std::endl;
   std::cerr << "This message was/wasn't valid." << std::endl;
}


}
