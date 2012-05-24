#include "cppunit/TestCase.h"
#include "cppunit/TestCaller.h"
#include "cppunit/TestSuite.h"
#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

#include "tfm/repro/TestRepro.hxx"
#include "tfm/repro/TestReproUser.hxx"

#include "tfm/TfmDns.hxx"

#include "rutil/dns/ExternalDns.hxx"
#include "rutil/dns/ExternalDnsFactory.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/MD5Stream.hxx"
#include "tfm/repro/CommandLineParser.hxx"
#include "tfm/DnsUtils.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestProxy.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/Security.hxx"
#endif

#include "DumFixture.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace std;
using namespace resip;

Security* DumFixture::security = 0;
TestProxy* DumFixture::proxy = 0;

DumUserAgent* DumFixture::derek = 0;
DumUserAgent* DumFixture::jason = 0;
DumUserAgent* DumFixture::scott = 0;
DumUserAgent* DumFixture::duane = 0;
DumUserAgent* DumFixture::hiss = 0;
TestReproUser* DumFixture::david = 0;
TestSipEndPoint* DumFixture::sipEndPoint = 0;
TestSipEndPoint* DumFixture::serviceEndPoint = 0;
StunEndPoint* DumFixture::stunEndPoint = 0;

SdpContents* DumFixture::standardOffer = 0;
SdpContents* DumFixture::standardAnswer = 0;

SdpContents* DumFixture::standardOfferAudio = 0;
SdpContents* DumFixture::standardAnswerAudio = 0;

TfdTestSipEndPoint* DumFixture::sipEndPointTcp = 0;

TestSipEndPoint* DumFixture::registrar = 0;
Data DumFixture::domain = "lcoal";
Data DumFixture::ipProxy1("127.0.0.1");
Data DumFixture::domainProxy1("proxy1" + domain);
int DumFixture::portProxy1 = 7080;
Data DumFixture::ipProxy2("127.0.0.1");
Data DumFixture::domainProxy2("proxy2" + domain);
int DumFixture::portProxy2 = 7070;
Data DumFixture:: ipProxy3("127.0.0.1");
Data DumFixture::domainProxy3("proxy3" + domain);
int DumFixture:: portProxy3 = 7060;

int DumFixture::portLocalhost = 5060;


static DumUserAgent* createDumUserAgent(const Data& user, const Data& host = "localhost", int port = 0)
{
   InfoLog(<< "Creating user " << user);
   Uri aor;
   aor.host() = host;
   aor.port() = port;
   aor.user() = user;
   SharedPtr<MasterProfile> prof = DumUserAgent::makeProfile(aor, user);
   DumUserAgent* ua = new DumUserAgent(prof, DumFixture::proxy);
   ua->init();
   return ua;
}

static DumUserAgent* createAnonDumUserAgent(const Data& user, const Data& host = "localhost", int port = 0)
{
   InfoLog(<< "Creating user " << user);
   Uri aor;
   aor.host() = host;
   aor.port() = port;
   aor.user() = user;
   SharedPtr<MasterProfile> prof = DumUserAgent::makeProfile(aor, user);
   //!dcm! -- truly terrible hack. Should be banned by the geneva convention,
   //clean up post-haste.
   SharedPtr<MasterProfile> anonProf = resip::shared_dynamic_cast<MasterProfile, UserProfile>(DumUserAgent::makeProfile(aor, user)->getAnonymousUserProfile());   
   DumUserAgent* ua = new DumUserAgent(anonProf, DumFixture::proxy);
   ua->init();
   return ua;
}

DumFixture::DumFixture() 
{
}


DumFixture::~DumFixture() 
{
}

void
DumFixture::initialize(int argc, char** argv)
{
   //ExternalDnsFactory::set(auto_ptr<ExternalDns>(createExternalDns()));

   InfoLog(<< "Setting up proxy");
#ifdef USE_SSL
   security = new Security();
#endif
   // enable for TLS testing
   //security = new resip::Security(getenv("PWD"));
   CommandLineParser args(argc, argv);
   proxy = new TestRepro("proxy", "localhost", args, Data::Empty, security);

   {
      Data offer("v=0\r\n"
                 "o=dumTfm 2087 3916 IN IP4 127.0.0.1\r\n"
                 "s=SIP Call\r\n"
                 "c=IN IP4 127.0.0.1\r\n"
                 "t=0 0\r\n"
                 "m=audio 48172 RTP/AVP 103\r\n"
                 "a=rtpmap:103 AMR-WB/16000\r\n"
                 "a=sendrecv\r\n"
                 "a=x-rtp-session-id:F705710F6BA346DA938102546C4A16DC\r\n"
                 "m=video 3380 RTP/AVP 115\r\n"
                 "a=sendrecv\r\n"
                 "a=x-rtp-session-id:41ABCFE1DDC74876822943DEE5952528\r\n");
      
   
      Data answer("v=0\r\n"
                  "o=dumTfm 2088 3917 IN IP4 127.0.0.4\r\n"
                  "s=SIP Call\r\n"
                  "c=IN IP4 127.0.0.4\r\n"
                  "t=0 0\r\n"
                  "m=audio 48172 RTP/AVP 103\r\n"
                  "a=rtpmap:103 AMR-WB/16000\r\n"
                  "a=sendrecv\r\n"
                  "m=video 3380 RTP/AVP 115\r\n"
                  "a=sendrecv\r\n");
   
      HeaderFieldValue hfv(offer.data(), offer.size());
      Mime type("application", "sdp");

      SdpContents o(hfv, type);
      standardOffer = new SdpContents(o);      
   
      HeaderFieldValue hfv1(answer.data(), answer.size());
      SdpContents a(hfv1, type);
      standardAnswer = new SdpContents(a);

      Data answerAudio("v=0\r\n"
                       "o=dumTfm 2087 3916 IN IP4 127.0.0.1\r\n"
                       "s=SIP Call\r\n"
                       "c=IN IP4 127.0.0.1\r\n"
                       "t=0 0\r\n"
                       "m=audio 48172 RTP/AVP 103\r\n"
                       "a=rtpmap:103 AMR-WB/16000\r\n"
                       "a=sendrecv\r\n"
                       "a=x-rtp-session-id:F705710F6BA346DA938102546C4A16DC\r\n");
      
      Data offerAudio("v=0\r\n"
                      "o=dumTfm 2088 3918 IN IP4 127.0.0.4\r\n"
                      "s=SIP Call\r\n"
                      "c=IN IP4 127.0.0.4\r\n"
                      "t=0 0\r\n"
                      "m=audio 48172 RTP/AVP 103\r\n"
                      "a=rtpmap:103 AMR-WB/16000\r\n");

      HeaderFieldValue hfv2(offerAudio.data(), offerAudio.size());
      SdpContents b(hfv2, type);
      standardOfferAudio = new SdpContents(b);

      HeaderFieldValue hfv3(answerAudio.data(), answerAudio.size());
      SdpContents c(hfv3, type);
      standardAnswerAudio = new SdpContents(c);
   }

   InfoLog(<< "Creating SIP proxy end point");
   Uri aorSipEndPoint;
   aorSipEndPoint.user() = "";
   aorSipEndPoint.host() = "127.0.0.1";
   aorSipEndPoint.port() = 6060;
   //aorSipEndPoint.param(p_transport) = Tuple::toData(TCP);
   sipEndPoint = new TestSipEndPoint(aorSipEndPoint, TestSipEndPoint::NoOutboundProxy, true, "127.0.0.1");

   Uri aorSipEndPointTcp;
   aorSipEndPointTcp.user() = "";
   aorSipEndPointTcp.host() = "127.0.0.1";
   aorSipEndPointTcp.port() = 6090;
   aorSipEndPointTcp.param(p_transport) = Tuple::toData(TCP);
   sipEndPointTcp = new TfdTestSipEndPoint(aorSipEndPointTcp, TestSipEndPoint::NoOutboundProxy, "127.0.0.1");

   InfoLog(<< "Creating Service end point");
   Uri aorServiceEndPoint;
   aorServiceEndPoint.user() = "";
   aorServiceEndPoint.host() = "127.0.0.1";
   aorServiceEndPoint.port() = 6070;
   //aorServiceEndPoint.param(p_transport) = Tuple::toData(TCP);
   serviceEndPoint = new TestSipEndPoint(aorServiceEndPoint, TestSipEndPoint::NoOutboundProxy, true, "127.0.0.1");

   derek = createDumUserAgent("derek");
   derek->addClientSubscriptionHandler("refer", derek);
   derek->addClientSubscriptionHandler("presence", derek);
   derek->addClientSubscriptionHandler("message-summary", derek);
   derek->addServerSubscriptionHandler("presence", derek);
   derek->addServerSubscriptionHandler("message-summary", derek);
   derek->addServerSubscriptionHandler("test", derek);
   derek->addClientPublicationHandler("presence", derek);
   derek->addClientPagerMessageHandler(derek);
   derek->addServerPagerMessageHandler(derek);
   derek->addOutOfDialogHandler(REFER, derek);
//   derek->run();

   jason = createDumUserAgent("jason");
   jason->addClientSubscriptionHandler("refer", jason);
   jason->addClientSubscriptionHandler("presence", jason);
   jason->addClientSubscriptionHandler("message-summary", jason);
   jason->addClientSubscriptionHandler("unknownEvent", jason);

   jason->addServerSubscriptionHandler("presence", jason);
   jason->addServerSubscriptionHandler("message-summary", jason);
   jason->addClientPagerMessageHandler(jason);
   jason->addServerPagerMessageHandler(jason);
//   jason->run();

   scott = createDumUserAgent("scott");
   scott->addClientSubscriptionHandler("presence", scott);
   scott->addClientSubscriptionHandler("message-summary", scott);
   scott->addServerSubscriptionHandler("presence", scott);
   scott->addServerSubscriptionHandler("message-summary", scott);
   scott->addServerSubscriptionHandler("refer", scott);
   scott->addOutOfDialogHandler(REFER, scott);
   scott->addSupportedOptionTag(Token("norefersub"));
//   scott->run();

   duane = createDumUserAgent("duane", sipEndPoint->getUri().host(), sipEndPoint->getUri().port());
   duane->addClientSubscriptionHandler("refer", duane);
   duane->addClientSubscriptionHandler("presence", duane);
   duane->addClientPublicationHandler("presence", duane);
   duane->addClientSubscriptionHandler("message-summary", duane);
   duane->addServerSubscriptionHandler("presence", duane);
   duane->addServerSubscriptionHandler("message-summary", duane);
//   duane->run();
   
   hiss = createAnonDumUserAgent("hiss", sipEndPoint->getUri().host(), sipEndPoint->getUri().port());


   InfoLog(<< "Creating user david");
   Uri j;
   j.user() = "david";
   j.host() = "localhost";
   david = new TestReproUser(*proxy, j, j.user(), j.user(), UDP, TestSipEndPoint::NoOutboundProxy, Data::Empty, security);

   Uri aorRegistrar;
   aorRegistrar.user() = "";
   aorRegistrar.host() = ipProxy2;
   aorRegistrar.port() = portProxy2;
   registrar = new TestSipEndPoint(aorRegistrar, TestSipEndPoint::NoOutboundProxy, true, ipProxy2);
}

void 
DumFixture::setUp()
{
}

void 
DumFixture::tearDown()
{
   if (SequenceClass::CPUSequenceSet)
   {
      if (SequenceClass::CPUSequenceSet->executionFailed())
      {
         InfoLog(<<"===================================================");
         WarningLog(<<"FAILED: Sleeping until retransmissions finish.");
         InfoLog(<<"===================================================");
#ifndef WIN32
         sleep(35); // could sleep longer here
#else
         Sleep(35000);
#endif

      }
      else
      {
         // !dlb! not quite right -- could have failed on CPPUNIT_ASSERT
         InfoLog(<<"===================================================");
         WarningLog(<<"PASSED");
         InfoLog(<<"===================================================");
      }
   }

   if( david )
      david->clean();
   if (scott)
      scott->clean();
   if (jason)
      jason->clean();
   if (derek)
      derek->clean();
   if (sipEndPoint)
      sipEndPoint->clean();
   
   if( stunEndPoint )
      stunEndPoint->close();
}
 
void 
DumFixture::destroyStatic()
{
   if( derek )
   {
      delete derek; 
      derek = 0;
   }
   if( jason )
   {
      delete jason; 
      jason = 0;
   }
   if( david )
   {
      delete david; 
      david = 0;
   }
   if( scott )
   {
      delete scott; 
      scott = 0;
   }

   if( proxy )
   {
      delete proxy;
      proxy = 0;
   }

   if( sipEndPoint )
   {
      delete sipEndPoint;
      sipEndPoint = 0;
   }

   if ( sipEndPointTcp )
   {
      delete sipEndPointTcp;
      sipEndPointTcp = 0;
   }

   if( stunEndPoint )
   {
      delete stunEndPoint;
      stunEndPoint = 0;
   }

   if (registrar)
   {
      delete registrar;
      registrar = 0;
   }
}

auto_ptr<Pidf>
DumFixture::makePidf(const DumUserAgent* dua)
{
   std::auto_ptr<Pidf> pidf(new Pidf);
   
   pidf->setSimpleStatus(true, "online", Data::from(dua->getAor().uri()));
   pidf->getTuples().front().id = dua->getProfile()->getDefaultFrom().uri().getAor();
   pidf->getTuples().front().attributes["displayName"] = "displayName";
   pidf->setEntity(dua->getAor().uri());

   return pidf;
}

ExternalDns*
DumFixture::createExternalDns()
{
   TfmDns* dns = new TfmDns();

   list<TfmDns::SRV> srvs;
   TfmDns::SRV srv1(5, 10, portProxy1, domainProxy1);
   TfmDns::SRV srv2(10, 10, portProxy2, domainProxy2);
   TfmDns::SRV srv3(20, 10, portProxy3, domainProxy3);
   srvs.push_back(srv1);
   srvs.push_back(srv2);
   srvs.push_back(srv3);
   dns->addSrvs("_sip._udp." + domain, srvs);

   list<TfmDns::Host> hosts;
   TfmDns::Host host1(ipProxy1);
   hosts.push_back(host1);
   dns->addHosts(domainProxy1, hosts);

   hosts.clear();
   TfmDns::Host host2(ipProxy2);
   hosts.push_back(host2);
   dns->addHosts(domainProxy2, hosts);

   hosts.clear();
   TfmDns::Host host3(ipProxy3);
   hosts.push_back(host3);
   dns->addHosts(domainProxy3, hosts);

   //add localhost
   hosts.clear();
   TfmDns::Host localhost("127.0.0.1");
   hosts.push_back(localhost);
   dns->addHosts("localhost", hosts);


   return dns;
}

DumUserAgent*
DumFixture::createUserAgentForFailoverTest(const Data& user, const Data& hostDomain)
{
   Uri aor;
   aor.user() = user;
   aor.host() = hostDomain;

   resip::SharedPtr<MasterProfile> prof = DumUserAgent::makeProfile(aor, user);
   DumUserAgent* agent  = new DumUserAgent(prof);
   agent->init();

   return agent;
}
