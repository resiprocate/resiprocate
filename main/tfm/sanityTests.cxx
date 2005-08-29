#include <cppunit/TextTestRunner.h>
#include <cppunit/TextTestResult.h>

#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "tfm/CommandLineParser.hxx"
#include "tfm/Fixture.hxx"
#include "tfm/RouteGuard.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/predicates/ExpectUtils.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

using namespace CppUnit;
using namespace resip;

static const int WaitFor100 = 1000;
static const int WaitFor180 = 1000;
static const int WaitFor487 = 1000;
static const int WaitForAck = 1000;  //immediate ACK for 4xx and CANCEL; not ACK for 200
static const int WaitForCommand = 1000;
static const int WaitForResponse = 1000;
static const int WaitForRegistration = 1000;
static const int PauseTime = 100;
static const int WaitForPause = 1100;
static const int WaitForEndOfTest = 1000;
static const int WaitForEndOfSeq = 1000;
static const int Seconds = 1000;

const Data transport("udp");
static NameAddr localhost;

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

class TestHolder : public Fixture
{
   public:
      
///***************************************** tests start here ********************************//
      void testRegisterBasic()
      {
         WarningLog(<<"*!testRegisterBasic!*");
         
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      static boost::shared_ptr<SipMessage>
      largeCallId(boost::shared_ptr<SipMessage> msg)
      {
         const int oversize = 2048;
         Data callId(oversize, Data::Preallocate);
         for (int i = 0; i < oversize/resip::Random::maxLength; ++i)
         {
            callId += resip::Random::getRandomHex(resip::Random::maxLength);
         }
         callId += resip::Random::getRandomHex(oversize - resip::Random::maxLength*(oversize/resip::Random::maxLength));
         msg->header(h_CallId).value() = callId;

         return msg;
      }

      void testOversizeCallIdRegister()
      {
         WarningLog(<<"*!testOversizeCallIdRegister!*");
         
         Seq(condition(largeCallId, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/400, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testRegisterClientRetransmits()
      {
         WarningLog(<<"*!testRegisterClientRetransmits!*");

         boost::shared_ptr<SipMessage> reg;
         Seq(save(reg, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->retransmit(reg)),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteClientRetransmissionsWithRecovery()
      {
         WarningLog(<<"*!testInviteClientRetransmissionsWithRecovery!*");
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, derek->note("R1")),
                     jason->expect(INVITE, contact(derek), 1000, derek->note("R2")),
                     jason->expect(INVITE, contact(derek), 2000, chain(derek->note("R3"), jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor100, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      void testInviteBasic()
      {
         WarningLog(<<"*!testInviteBasic!*");
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(jason->invite(*derek),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                     jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
                     jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                     derek->expect(ACK, from(jason), WaitForResponse, jason->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      static boost::shared_ptr<SipMessage>
      largeContact(boost::shared_ptr<SipMessage> msg)
      {
         assert(msg->exists(h_Contacts) &&
                !msg->header(h_Contacts).empty());

         const int oversize = 2048;
         Data contactUser(oversize, Data::Preallocate);
         for (int i = 0; i < oversize/resip::Random::maxLength; ++i)
         {
            contactUser += resip::Random::getRandomHex(resip::Random::maxLength);
         }
         contactUser += resip::Random::getRandomHex(oversize - resip::Random::maxLength*(oversize/resip::Random::maxLength));
         msg->header(h_Contacts).front().uri().user() = contactUser;

         return msg;
      }

      void testOversizeContactRegister()
      {
         WarningLog(<<"*!testOversizeContactRegister!*");
         Seq(condition(largeContact, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/500, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testUnregisterAll()
      {
         WarningLog(<<"*!testUnregisterAll!*");
         
         NameAddr na;
		 set<NameAddr> all;
		 set<NameAddr> emptySet;
          
         na.setAllContacts();
         all.insert( na );

         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(jason1->getDefaultContacts() )),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts( mergeContacts(*jason1, *jason2) )),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason1->registerUser(0, all ),
             jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
             
         ExecuteSequences();
      }

      void testInfo()
      {
         WarningLog(<<"*!testInfo!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(derek->info(jason),
             derek->expect(INFO/407, from(proxy), 1000, derek->digestRespond()),
             jason->expect(INFO, from(derek), 1000, jason->ok()),
             derek->expect(INFO/200, from(jason), 1000, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }


      void testInviteClientRetransmitsAfter200()
      {
         WarningLog(<<"*!testInviteClientRetransmitsAfter200!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         boost::shared_ptr<SipMessage> inv;
         boost::shared_ptr<SipMessage> ok;
         Seq(save(inv, derek->invite(*jason)),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, from(derek), 2000, chain(jason->send100(), jason->ring(), (ok <= jason->answer()))),
                     derek->expect(INVITE/180, from(jason), WaitFor100, derek->noAction()),
                     derek->expect(INVITE/200, from(jason), WaitForResponse, chain(derek->retransmit(inv), derek->pause(500), jason->retransmit(ok))),
                     derek->expect(INVITE/200, from(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }


      void testNonInviteClientRetransmissionsWithRecovery()
      {
         WarningLog(<<"*!testNonInviteClientRetransmissionsWithRecovery!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(derek->info(jason),
             derek->expect(INFO/407, from(proxy), 1000, derek->digestRespond()),
             jason->expect(INFO, from(derek), 1000, jason->noAction()),
             jason->expect(INFO, from(derek), 1000, jason->ok()),
             derek->expect(INFO/200, from(jason), 1000, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testNonInviteClientRetransmissionsWithTimeout()
      {
         WarningLog(<<"*!testNonInviteClientRetransmissionsWithTimeout!*");
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(derek->info(jason),
             derek->expect(INFO/407, from(proxy), 1000, derek->digestRespond()),
             jason->expect(INFO, from(derek), 1000, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             jason->expect(INFO, from(derek), 4800, jason->noAction()),
             // note: 408 to NIT are not forwarded by repro
             //derek->expect(INFO/408, from(proxy), 4800, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }
      
      void testNonInviteServerRetransmission()
      {
         WarningLog(<<"*!testNonInviteServerRetransmission!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         boost::shared_ptr<SipMessage> infoMsg;
         Seq(save(infoMsg, derek->info(jason)),
             derek->expect(INFO/407, from(proxy), 1000, derek->digestRespond()),
             jason->expect(INFO, from(derek), 1000, jason->noAction()),
             jason->expect(INFO, from(derek), 1000, jason->ok()),
             derek->expect(INFO/200, from(jason), 1000, derek->retransmit(infoMsg)),
             derek->expect(INFO/200, from(jason), 1000, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }


      void testInviteTransportFailure()
      {
         WarningLog(<<"*!testInviteTransportFailure!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(chain(jason->closeTransport(), derek->invite(*jason)),
             optional(derek->expect(INVITE/100, from(proxy), 300+WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),
                                                                           derek->digestRespond())),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/503, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      void testInviteNoDNS()
      {
         WarningLog(<<"*!testInviteNoDNS!*");
         
         RouteGuard dGuard(*proxy, "sip:.*@.*", "sip:foobar@dfkaslkfdas.com");
         Seq(jason->invite(*derek),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/503, from(proxy), 5000, jason->ack()),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      void testBasic302()
      {
         WarningLog(<<"*!testBasic302!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, jason->send302()),
                     And(Sub(jason->expect(ACK, from(proxy), WaitForResponse, jason->noAction())),
                         Sub(derek->expect(INVITE/302, from(jason), WaitForResponse, chain(derek->ack(), derek->invite(*david))),
                             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
                             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                                 Sub(david->expect(INVITE, contact(derek), WaitForCommand, david->ok()),
                                     derek->expect(INVITE/200, contact(david), WaitForResponse, derek->ack()),
                                     david->expect(ACK, from(derek), WaitForCommand, david->noAction()))))))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      void testInviteBusy()
      {
         WarningLog(<<"*!testInviteBusy!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send486())),
                     jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                     derek->expect(INVITE/486, from(proxy), WaitForResponse, derek->ack()))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }

      void testInviteAllBusyContacts()
      {
         WarningLog(<<"*!testInviteAllBusyContacts!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         Seq(cullen->registerUser(60, cullen->getDefaultContacts()),
             cullen->expect(REGISTER/407, from(proxy), WaitForResponse, cullen->digestRespond()),
             cullen->expect(REGISTER/200, from(proxy), WaitForRegistration, cullen->noAction()),
             WaitForEndOfTest);
         Seq(enlai->registerUser(60, enlai->getDefaultContacts()),
             enlai->expect(REGISTER/407, from(proxy), WaitForResponse, enlai->digestRespond()),
             enlai->expect(REGISTER/200, from(proxy), WaitForRegistration, enlai->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         RouteGuard dGuard(*proxy, "sip:(.......)@.*", "sip:$1@" + david->getContact().uri().getAor(), Data::Empty, Data::Empty, 1);
         RouteGuard dGuard1(*proxy, "sip:(.......)@.*","sip:$1@" + cullen->getContact().uri().getAor(), Data::Empty, Data::Empty, 3);
         RouteGuard dGuard2(*proxy, "sip:(.......)@.*", "sip:$1@" + enlai->getContact().uri().getAor(), Data::Empty, Data::Empty, 7);

         Seq(jason->invite(*derek),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),

             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(david->expect(INVITE, contact(jason), WaitForCommand, chain(derek->send100(),david->send503())),
                     cullen->expect(INVITE, contact(derek), WaitForCommand, cullen->send503()),
                     enlai->expect(INVITE, contact(derek), WaitForCommand, enlai->send503()),
                     jason->expect(INVITE/480, from(proxy), 3*WaitForCommand, jason->ack()))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }

      void testInviteCalleeHangsUp()
      {
         WarningLog(<<"*!testInviteCalleeHangsUp!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor180, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, chain(jason->pause(PauseTime), jason->bye())),
                     derek->expect(BYE, from(jason), WaitForPause, derek->ok()),
                     jason->expect(BYE/200, from(derek), WaitForResponse, jason->noAction()))),
             WaitForEndOfTest);    

         ExecuteSequences();  
      }


      void testInviteCallerHangsUp()
      {
         WarningLog(<<"*!testInviteCallerHangsUp!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfSeq);
         
         ExecuteSequences();
         
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor100, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, chain(jason->pause(PauseTime), derek->bye(jason))),
                     jason->expect(BYE, from(derek), WaitForPause, jason->ok()),
                     derek->expect(BYE/200, from(jason), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);

         ExecuteSequences();  
      }

      void testInviteCallerCancels()
      {
         WarningLog(<<"*!testInviteCallerCancels!*");
         
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             1000);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             1000);
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, jason->ring()),
                     derek->expect(INVITE/180, from(jason), WaitFor180, derek->cancel()),
                             
                     And(Sub(jason->expect(CANCEL, from(proxy), WaitForCommand, chain(jason->ok(), jason->send487())),
                             And(Sub(jason->expect(ACK, from(proxy), WaitForAck, jason->noAction())),
                                 Sub(derek->expect(INVITE/487, from(jason), WaitFor487, derek->ack())))),
                         Sub(derek->expect(CANCEL/200, from(proxy), WaitForResponse, derek->noAction()))))),
             WaitForEndOfTest);    

         ExecuteSequences();  
      }

      /*
        INV ->
        <- 100 (cancels timer)
        CANCEL ->
        <- 200 (CANCEL)
        **END**
        */
      void testInviteCallerCancelsNo487()
      {
         WarningLog(<<"*!testInviteCallerCancelsNo487!*");
         
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             1000);
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, jason->ring()),
                     derek->expect(INVITE/180, from(jason), WaitFor180, derek->cancel()),
                     
                     And(Sub(jason->expect(CANCEL, from(proxy), WaitForCommand, jason->ok()),
                             derek->expect(INVITE/408, from(proxy), 90*1000, derek->ack())), // !jf! should be faster than this ?
                         Sub(derek->expect(CANCEL/200, from(proxy), WaitForResponse, derek->noAction()))))),
             WaitForEndOfTest);    
         
         ExecuteSequences();  
      }

      void testInviteNoAnswerCancel()
      {
         WarningLog(<<"*!testInviteNoAnswerCancel!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->invite(*derek),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(),jason->digestRespond())),

             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(derek->expect(INVITE, contact(jason), WaitForCommand, derek->ring()),
                             
                     jason->expect(INVITE/180, from(derek), WaitFor180, jason->cancel()),
                             
                     And(Sub(jason->expect(CANCEL/200, from(proxy), WaitForResponse, jason->noAction())),
                         Sub(derek->expect(CANCEL, from(proxy), WaitForCommand, chain(derek->ok(), derek->send487())))),
                             
                     And(Sub(derek->expect(ACK, from(proxy), WaitForAck, derek->noAction())),
                         Sub(jason->expect(INVITE/487, from(derek), WaitFor487, jason->ack()))))),
                  
             WaitForEndOfTest);    

         ExecuteSequences();  
      }

      void testInviteNotFound()
      {
         WarningLog(<<"*!testInviteNotFound!*");

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             optional(derek->expect(INVITE/100, from(proxy), WaitForResponse, derek->noAction())),
             derek->expect(INVITE/480, from(proxy), WaitForResponse, derek->ack()),
             WaitForEndOfTest);
         ExecuteSequences();
      }


      void testInviteNotFoundServerRetransmits()
      {
         WarningLog(<<"*!testInviteNotFoundServerRetransmits!*");

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/480, from(proxy), WaitForResponse, derek->noAction()),
             derek->expect(INVITE/480, from(proxy), 2*WaitForResponse, derek->noAction()),
             derek->expect(INVITE/480, from(proxy), 4*WaitForResponse, derek->ack()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/480, from(proxy), WaitForResponse, derek->noAction()),
             derek->expect(INVITE/480, from(proxy), 2*WaitForResponse, derek->noAction()),
             derek->expect(INVITE/480, from(proxy), 4*WaitForResponse, derek->ack()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteClientLateAck()
      {
         WarningLog(<<"*!testInviteClientLateAck!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         boost::shared_ptr<SipMessage> ok;
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), 1000, jason->ring()),
                     derek->expect(INVITE/180, from(jason), 1000, ok <= jason->answer()),
                     derek->expect(INVITE/200, contact(jason), 4500, chain(jason->pause(500), jason->retransmit(ok))),
                     derek->expect(INVITE/200, contact(jason), 4500, chain(jason->pause(500), jason->retransmit(ok))),
                     derek->expect(INVITE/200, contact(jason), 4500, chain(jason->pause(500), jason->retransmit(ok))),
                     derek->expect(INVITE/200, contact(jason), 4500, derek->ack()),
                     jason->expect(ACK, from(derek), 4500, jason->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }
      
      void testInviteClientMissedAck()
      {
         WarningLog(<<"*!testInviteClientMissedAck!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         boost::shared_ptr<SipMessage> ok;
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), 1000, jason->ring()),
                     derek->expect(INVITE/180, from(jason), 1000, ok <= jason->answer()),
                     derek->expect(INVITE/200, contact(jason), 1000, chain(jason->pause(250), jason->retransmit(ok))),
                     derek->expect(INVITE/200, contact(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), 1000, jason->retransmit(ok)),
                     derek->expect(INVITE/200, contact(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), 1000, jason->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testInviteClientMissedAck2()
      {
         WarningLog(<<"*!testInviteClientMissedAck!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         boost::shared_ptr<SipMessage> ok;
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), 1000, jason->ring()),
                     derek->expect(INVITE/180, from(jason), 1000, ok <= jason->answer()),
                     derek->expect(INVITE/200, contact(jason), 1000, jason->retransmit(ok)),
                     derek->expect(INVITE/200, contact(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), 1000, jason->retransmit(ok)),
                     derek->expect(INVITE/200, contact(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), 1000, jason->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      class CheckRport
      {
         public:
            CheckRport(int rport) : mRport(rport)
            {
            }
            bool operator()(boost::shared_ptr<Event> event) const
            {
               SipEvent* msgEvent = dynamic_cast<SipEvent*>(event.get());
               assert(msgEvent);
               boost::shared_ptr<resip::SipMessage> msg = msgEvent->getMessage();
               assert(msg.get());

               DebugLog (<< "Looking for rport=" << mRport << endl << *msg);
               
               if (!msg->header(h_Vias).empty())
               {
                  DebugLog (<< "Top via " 
                            << (msg->header(h_Vias).front().exists(p_rport) 
                                ? " has rport " : " no rport"));
                  DebugLog (<< "Got port="  << msg->header(h_Vias).front().param(p_rport).port()
                            << " expecting port=" << mRport);
               }
               return (!msg->header(h_Vias).empty() && 
                       msg->header(h_Vias).front().exists(p_rport) && 
                       // !dlb! happy with anything
                       (msg->header(h_Vias).front().param(p_rport).port() != 0 ||
                        msg->header(h_Vias).front().param(p_rport).port() == mRport));
            }
            int mRport;
      };

      static boost::shared_ptr<SipMessage>& addRport(boost::shared_ptr<SipMessage>& msg)
      { 
         assert(!msg->header(h_Vias).empty());
         // mentioning it makes it so
         msg->header(h_Vias).front().param(p_rport);
         return msg;
      }

      void testInviteForRport()
      {
         WarningLog(<<"*!testInviteForRport!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         // This assumes that receive and send port are the same
         // otherwise, could just check for existence of rport parameter
         CheckRport checkRport(derek->getPort());
         Seq(condition(addRport, derek->invite(*jason)),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->check(checkRport)))),
                 Sub(jason->expect(INVITE, contact(derek), 1000, jason->ring()),
                     derek->expect(INVITE/180, from(jason), 1000, jason->answer()),
                     derek->expect(INVITE/200, contact(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), 1000, jason->noAction()))),
             WaitForEndOfSeq);
         
         ExecuteSequences();
      }
      
      void testAttendedExtensionToExtensionTransfer()
      {
         WarningLog(<<"*!testAttendedExtensionToExtensionTransfer!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         //103 calls 102, 103 puts 102 on hold.
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor180, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, jason->noAction()))),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->reInvite(*jason),
             //optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             //derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, jason->answer()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, jason->noAction()))),
             WaitForEndOfTest);

         
         ExecuteSequences();
         
         //103 calls 104

         Seq(derek->invite(*david),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(david->expect(INVITE, contact(derek), WaitForCommand, chain(david->ring(), david->answer())),
                     derek->expect(INVITE/180, from(david), WaitFor180, derek->noAction()),
                     derek->expect(INVITE/200, contact(david), WaitForResponse, derek->ack()),
                     david->expect(ACK, from(derek), WaitForResponse, david->noAction()))),
             WaitForEndOfTest);
         
         ExecuteSequences();
      
         //103 puts 104 on hold
         
         Seq(derek->reInvite(*david),
             //optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             //derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(david->expect(INVITE, contact(derek), WaitForCommand, david->answer()),
                     derek->expect(INVITE/200, contact(david), WaitForResponse, derek->ack()),
                     david->expect(ACK, from(derek), WaitForResponse, david->noAction()))),
             1000);
         
         ExecuteSequences();

         //102 refers 103 to 104

         Seq(derek->referReplaces(jason->getContact().uri(), david->getAddressOfRecord()),
             jason->expect(REFER, from(derek), WaitForCommand, chain(jason->send202(), jason->inviteReferReplaces())),
             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
                     jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
                     optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
                     david->expect(INVITE, contact(jason), WaitForCommand, chain(david->send100(), david->ok())),
                     jason->expect(INVITE/200, contact(david), WaitForResponse, chain(jason->ack(), jason->notify200(*derek))),
                     And(Sub(david->expect(ACK, from(jason), WaitForResponse, david->noAction())),
                         Sub(derek->expect(NOTIFY, from(jason), WaitForCommand, derek->ok()),
                             jason->expect(NOTIFY/200, from(derek), WaitForResponse, jason->noAction())))),
                 Sub(derek->expect(REFER/202, from(jason), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }

      void testBlindTransferExtensionToExtensionHangupImmediately()
      {
         WarningLog(<<"*!testBlindTransferExtensionToExtensionHangupImmediately!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         //103 calls 102, 102 puts 103 on hold.
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor180, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, chain(jason->pause(PauseTime), jason->reInvite(*derek))),
                     And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitForPause, jason->noAction()))),
                         Sub(derek->expect(INVITE, contact(jason), WaitForPause, chain(derek->ring(), derek->answer())),
                             jason->expect(INVITE/180, from(derek), WaitFor180, jason->noAction()),
                             jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                             derek->expect(ACK, from(jason), WaitForResponse, derek->noAction()))))),
             WaitForEndOfTest);
         
         ExecuteSequences();
         
         
         //102 refers 103 to 104

         Seq(jason->refer(derek->getContact().uri(), david->getAddressOfRecord()),
             derek->expect(REFER, from(jason), WaitForCommand, chain(derek->send202(), derek->inviteReferredBy())),
             And(Sub(jason->expect(REFER/202, from(derek), WaitForResponse, jason->noAction())),
                 Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                     derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
                     And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                         Sub(david->expect(INVITE, contact(derek), WaitForCommand, chain(david->send100(), david->answer())),
                             derek->expect(INVITE/200, contact(david), WaitForResponse, chain(derek->ack(), derek->notify200(*jason))),
                             And(Sub(jason->expect(NOTIFY, from(derek), WaitForCommand, chain(jason->ok(), jason->note("sending bye"), jason->bye())),
                                     And(Sub(derek->expect(NOTIFY/200, from(jason), WaitForResponse, derek->noAction())),
                                         Sub(derek->expect(BYE, from(jason), WaitForCommand, derek->ok()),
                                             jason->expect(BYE/200, from(derek), WaitForResponse, jason->noAction())))),
                                 Sub(david->expect(ACK, from(derek), WaitForAck, chain(david->note("sending bye"), david->bye())),
                                     derek->expect(BYE, from(david), WaitForCommand, derek->ok()),
                                     david->expect(BYE/200, from(proxy), WaitForResponse, david->noAction()))))))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }


      void testConferenceConferencorHangsUp()
      {
         WarningLog(<<"*!testConferenceConferencorHangsUp!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         //103 calls 102, 102 puts 103 on hold.
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->ring(), jason->answer())),
                     derek->expect(INVITE/180, from(jason), WaitFor180, derek->noAction()),
                     derek->expect(INVITE/200, contact(jason), WaitForResponse, derek->ack()),
                     jason->expect(ACK, from(derek), WaitForResponse, chain(jason->pause(PauseTime), jason->reInvite(*derek))),
                     And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                         Sub(derek->expect(INVITE, contact(jason), WaitForPause, chain(derek->ring(), derek->answer())),
                             jason->expect(INVITE/180, from(derek), WaitFor180, jason->noAction()),
                             jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                             derek->expect(ACK, from(jason), WaitForResponse, derek->noAction()))))),
             WaitForEndOfTest);
         
         ExecuteSequences();
         
         //102 calls 104
         Seq(jason->invite(*david),
             optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
             jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(),jason->digestRespond())),

             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(david->expect(INVITE, contact(jason), WaitForCommand, chain(david->ring(), david->answer())),
                     jason->expect(INVITE/180, from(david), WaitFor180, jason->noAction()),
                     jason->expect(INVITE/200, contact(david), WaitForResponse, jason->ack()),
                     david->expect(ACK, from(jason), WaitForResponse, david->noAction()))),
             1000);
         
         ExecuteSequences();
         
         //102 takes 103 off hold
         Seq(jason->reInvite(*derek),
             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                     jason->expect(INVITE/180, from(derek), WaitFor180, jason->noAction()),
                     jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                     derek->expect(ACK, from(jason), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         
         ExecuteSequences();
         
         //102 hangs up - 103 and 104 should be connected by refer
         //first 102 places 103 on hold

         Seq(jason->reInvite(*derek),
             And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
                 Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                     jason->expect(INVITE/180, from(derek), WaitFor180, jason->noAction()),
                     jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                     derek->expect(ACK, from(jason), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         
         ExecuteSequences();

         //102 refers with replaces 103 to 104

         Seq(jason->referReplaces(derek->getContact().uri(), david->getAddressOfRecord()),
             derek->expect(REFER, from(jason), WaitForCommand, chain(derek->send202(), derek->inviteReferReplaces())),
             And(Sub(jason->expect(REFER/202, from(derek), WaitForResponse, jason->noAction())),
                 Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                     derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),derek->digestRespond())),
                     optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                     david->expect(INVITE, contact(derek), WaitForCommand, chain(david->send100(), david->ok())),
                     derek->expect(INVITE/200, contact(david), WaitForResponse, chain(derek->ack(), derek->notify200(*jason))),
                     And(Sub(david->expect(ACK, from(derek), WaitForResponse, david->noAction())),
                         Sub(jason->expect(NOTIFY, from(derek), WaitForCommand, jason->ok()),
                             derek->expect(NOTIFY/200, from(jason), WaitForResponse, chain(derek->pause(PauseTime), derek->bye())),
                             jason->expect(BYE, from(derek), WaitForPause, jason->ok()),
                             derek->expect(BYE/200, from(jason), WaitForResponse, derek->noAction()))))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }


      void testInviteForkOneAnswers()
      {
         WarningLog(<<"*!testInviteForkOneAnswers!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(And(Sub(jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->ring(), jason2->pause(PauseTime), jason2->answer())),
                             And(Sub(derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction())),
                                 Sub(derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()))),
                             derek->expect(INVITE/200, contact(jason2), WaitForPause, derek->ack()),
                             jason2->expect(ACK, from(derek), WaitForResponse, chain(jason2->pause(PauseTime), jason2->bye())),
                             derek->expect(BYE, from(jason2), WaitForResponse, derek->ok()),
                             jason2->expect(BYE/200, from(derek), WaitForResponse, jason2->noAction())),
                         Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
                             jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487())),
                             jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction()))))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }


      void testForkedInviteClientLateAck()
      {
         WarningLog(<<"*!testForkedInviteClientLateAck!*");

         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         boost::shared_ptr<SipMessage> ok;
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(And(Sub(jason2->expect(INVITE, contact(derek), WaitForCommand, 
                                                  chain(jason2->ring(), jason2->pause(PauseTime), ok <= jason2->answer())),
                             And(Sub(derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction())),
                                 Sub(derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()))),
                             derek->expect(INVITE/200, contact(jason2), WaitForPause, chain(jason2->pause(500),jason2->retransmit(ok))),
                             derek->expect(INVITE/200, contact(jason2), WaitForPause, derek->ack()),
                             jason2->expect(ACK, from(derek), WaitForResponse, chain(jason2->pause(PauseTime), jason2->bye())),
                             derek->expect(BYE, from(jason2), WaitForResponse, derek->ok()),
                             jason2->expect(BYE/200, from(derek), WaitForResponse, jason2->noAction())),
                         Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
                             jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487())),
                             jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction()))))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }


      void testInviteForkOneBusy()
      {
         WarningLog(<<"*!testInviteForkOneBusy!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(And(Sub(jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->ring(), jason2->pause(PauseTime), jason2->answer())),
                             derek->expect(INVITE/180, from(jason2), WaitForResponse, derek->noAction()),
                             derek->expect(INVITE/200, contact(jason2), WaitForResponse, derek->ack()),
                             jason2->expect(ACK, from(derek), WaitForResponse, chain(jason2->pause(PauseTime), jason2->bye())),
                             derek->expect(BYE, from(jason2), WaitForCommand, derek->ok()),
                             jason2->expect(BYE/200, from(derek), WaitForResponse, jason2->noAction())),
                         Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->send486()),
                             jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction()))))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }


      void testInviteForkBothAnswerNoProvisional()
      {
         WarningLog(<<"*!testInviteForkBothAnswerNoProvisional!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();


         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->noAction()))),
                 Sub(jason2->expect(INVITE, contact(derek), 1000, jason2->answer()),
                     derek->expect(INVITE/200, contact(jason2), 1000, derek->ack()),
                     jason2->expect(ACK, from(proxy), 1000, jason2->noAction())),
                 Sub(jason1->expect(INVITE, contact(derek), 1000, jason1->answer()),
                     derek->expect(INVITE/200, contact(jason1), 1000, derek->ack()),
                     jason1->expect(ACK, from(proxy), 1000, jason1->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  

#if 0
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->noAction()))),
                 Sub(jason2->expect(INVITE, contact(derek), 1000, jason2->answer()),
                     derek->expect(INVITE/200, contact(jason2), 1000, derek->ack()),
                     jason2->expect(ACK, from(proxy), 1000, jason2->noAction())),
                 Sub(jason1->expect(INVITE, contact(derek), 1000, chain(jason1->pause(100), jason1->answer())),
                     derek->expect(INVITE/200, contact(jason1), 1000, derek->ack()),
                     jason1->expect(ACK, from(proxy), 1000, jason1->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();
#endif
      }
      
      void testInviteForkBothBusy()
      {
         WarningLog(<<"*!testInviteForkBothBusy!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         CountDown count486(2, "count486");

         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(And(Sub(jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->pause(PauseTime), chain(jason2->send486(), count486.dec()))),
                             jason2->expect(ACK, from(proxy), WaitForAck, jason2->noAction())),
                         Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, chain(jason1->send486(), count486.dec())),
                             jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction()))),
                     derek->expect(INVITE/486, from(proxy), WaitForResponse, derek->ack()))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }
                     
      void testInviteForkThreeCallerCancels()
      {
         WarningLog(<<"*!testInviteForkThreeCallerCancels!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason3->registerUser(60, jason3->getDefaultContacts()),
             jason3->expect(REGISTER/407, from(proxy), WaitForResponse, jason3->digestRespond()),
             jason3->expect(REGISTER/200, from(proxy), WaitForRegistration, jason3->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         CountDown count487(3, "count487");
         
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(And(Sub(jason2->expect(INVITE, contact(derek), WaitForCommand, jason2->ring()),
                             derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction())),
                         Sub(jason3->expect(INVITE, contact(derek), WaitForCommand, jason3->ring()),
                             derek->expect(INVITE/180, from(jason3), WaitFor180, derek->noAction())),
                         Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()), // used to pause before ring
                             derek->expect(INVITE/180, from(jason1), WaitForPause, chain(derek->note("cancelling"), derek->cancel())),
                             And(Sub(derek->expect(CANCEL/200, from(proxy), WaitForResponse, derek->noAction()),
                                     derek->expect(INVITE/487, from(proxy), count487, WaitFor487, chain(derek->note("break2b"), derek->ack()))),
                                 Sub(jason2->expect(CANCEL, from(proxy), WaitForCommand, chain(jason2->ok(), jason2->send487(), count487.dec())),
                                     jason2->expect(ACK, from(proxy), WaitForAck, chain(jason2->note("break1a"),jason2->noAction()))),
                                 Sub(jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487(), count487.dec())),
                                     jason1->expect(ACK, from(proxy), WaitForAck, chain(jason1->note("break2a"),jason1->noAction()))),
                                 Sub(jason3->expect(CANCEL, from(proxy), WaitForCommand, chain(jason3->ok(), jason3->send487(), count487.dec())),
                                     jason3->expect(ACK, from(proxy), WaitForAck, chain(jason3->note("break2c"),jason3->noAction())))))))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }

      void testInviteForkCallerCancels()
      {
         WarningLog(<<"*!testInviteForkCallerCancels!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         CountDown count487(2, "count487");
         
         Seq(derek->invite(*jason),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(And(Sub(jason2->expect(INVITE, contact(derek), WaitForCommand, jason2->ring()),
                             derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction())),
                         Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, chain(jason1->pause(PauseTime),jason1->ring())),
                             derek->expect(INVITE/180, from(jason1), WaitForPause, chain(derek->note("cancelling"), derek->cancel())),
                             And(Sub(derek->expect(CANCEL/200, from(proxy), WaitForResponse, derek->noAction()),
                                     derek->expect(INVITE/487, from(proxy), count487, WaitFor487, chain(derek->note("break2b"), derek->ack()))),
                                 Sub(jason2->expect(CANCEL, from(proxy), WaitForCommand, chain(jason2->ok(), jason2->send487(), count487.dec())),
                                     jason2->expect(ACK, from(proxy), 1000, chain(jason2->note("break1a"),jason2->noAction()))),
                                 Sub(jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487(), count487.dec())),
                                     jason1->expect(ACK, from(proxy), 1000, chain(jason1->note("break2a"),jason1->noAction())))))))),
             WaitForEndOfTest);
         
         ExecuteSequences();  
      }

      void testSpiral()
      {
         WarningLog(<<"*!testSpiral!*");
         RouteGuard dGuard(*proxy, "sip:spiral@.*", david->getAddressOfRecordString());

         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(derek->invite(proxy->makeUrl("9spiral").uri()),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(david->expect(INVITE, contact(derek), WaitForCommand, chain(david->ring(), david->answer())),
                     derek->expect(INVITE/180, from(david), WaitFor100, derek->noAction()),
                     derek->expect(INVITE/200, contact(david), WaitForResponse, derek->ack()),
                     david->expect(ACK, from(derek), WaitForResponse, derek->noAction()))),
             WaitForEndOfTest);
         ExecuteSequences();  
      }

      void testInviteCallerCancelsWithRace()
      {
         WarningLog(<<"*!testInviteCallerCancelsWithRace!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             1000);
         ExecuteSequences();

         Seq(david->invite(*derek),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             
             And(Sub(optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction()))),
                 Sub(derek->expect(INVITE, contact(david), WaitForCommand, derek->ring()),
                     david->expect(INVITE/180, from(derek), WaitFor180, chain(derek->ok(), derek->pause(25), david->cancel())),
                     And(Sub(david->expect(CANCEL/200, from(proxy), WaitForCommand, derek->noAction())),
                         Sub(david->expect(INVITE/200, contact(derek), WaitForCommand, david->ack()),
                             derek->expect(ACK, from(proxy), WaitForAck, derek->noAction()))))),
             WaitForEndOfTest);    
         
         ExecuteSequences();  
      }


      // JF
      ///////////////////////////////////////////////////////////
      // REGISTER TESTS
      ///////////////////////////////////////////////////////////
      void testDigestRegister()
      {
         WarningLog(<<"*!testDigestRegister!*");
         
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
             1000);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(derek->getDefaultContacts(), 60)),
             1000);
         ExecuteSequences();

         // second time, it has the credentials already
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
             1000);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(derek->getDefaultContacts(), 60)),
             1000);
         ExecuteSequences();

      }

      void testDigestInviteBasic()
      {
         WarningLog(<<"*!testDigestInviteBasic!*");

         // second time, it may have the credentials already
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
             1000);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(derek->getDefaultContacts(), 60)),
             1000);
         ExecuteSequences();

         Seq(david->invite(*jason),
             optional(david->expect(INVITE/100, from(proxy), 2000, david->noAction())),
             david->expect(INVITE/407, from(proxy), 2000, chain(david->ack(), david->pause(2000), david->digestRespond())),
             And( Sub(jason->expect(INVITE, contact(david), 2000, chain(jason->ring(), jason->answer())),
                      david->expect(INVITE/180, from(jason), 2000, david->noAction()),
                      david->expect(INVITE/200, contact(jason), 2000, david->ack()),
                      jason->expect(ACK, from(david), 2000, jason->noAction())),
                  Sub(optional(david->expect(INVITE/100, from(proxy), 2000, david->noAction())))),
             1000);
         ExecuteSequences();  
      }
      
      void test2Serial()
      {
         WarningLog(<<"*!test2Serial!*");

         Seq(jason->registerUser(61, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(jason->getDefaultContacts(), 61)),
             500);
         ExecuteSequences();

         Seq(derek->registerUser(62, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(derek->getDefaultContacts(), 62)),
             500);
         ExecuteSequences();
      }

      void test2Parallel()
      {
         WarningLog(<<"*!test2Parallel!*");

         Seq(jason->registerUser(63, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(jason->getDefaultContacts(), 63)),
             500);
         Seq(derek->registerUser(64, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(derek->getDefaultContacts(), 64)),
             500);

         ExecuteSequences();
      }

      void testMultiContactSerial()
      {
         WarningLog(<<"*!testMultiContactSerial!*");

         Seq(jason1->registerUser(65, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), 1000, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(jason1->getDefaultContacts(), 65)),
             500);
         ExecuteSequences();         

         Seq(jason2->registerUser(66, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), 1000, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), 3000, new CheckFetchedContacts(mergeContacts(*jason2, *jason1))),
             500);
         ExecuteSequences();
      }

      void testDetailsQValue()
      {
         WarningLog(<<"*!testDetailsQValue!*");
         
         NameAddr con = *(jason->getDefaultContacts().begin());
         con.param(p_q) = 0.1;
         
         set<NameAddr> contacts;
         contacts.insert(con);
                  
         Seq(jason->registerUser(67, con),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(contacts, 67)),
             500);
         
         ExecuteSequences();
      }

      void testDetailsExpires()
      {
         WarningLog(<<"*!testDetailsExpires!*");

         NameAddr con = *(jason->getDefaultContacts().begin());
         con.param(p_expires) = 60000;
         
         set<NameAddr> contacts;
         contacts.insert(con);
         
         Seq(jason->registerUser(68, con),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 68)),
             500);
         
         ExecuteSequences();
      }

      void testMixedExpires()
      {
         WarningLog(<<"*!testMixedExpires!*");

         NameAddr cond = *(jason->getDefaultContacts().begin());
         NameAddr conk = *(derek->getDefaultContacts().begin());
         cond.param(p_expires) = 60000;
         
         set<NameAddr> contacts;
         contacts.insert(cond);
         contacts.insert(conk);

         Seq(jason->registerUser(3000, contacts),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 3000)),
             500);
         
         ExecuteSequences();
      }
                      
      void testDetailsQValueExpires()
      {
         WarningLog(<<"*!testDetailsQValueExpires!*");

         NameAddr con = *(jason->getDefaultContacts().begin());
         con.param(p_expires) = 60000;
         con.param(p_q) = 0.1;

         set<NameAddr> contacts;
         contacts.insert(con);

         Seq(jason->registerUser(69, con),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 69)),
             500);
         
         ExecuteSequences();
      }
      
      void testMultiple1()
      {         
         WarningLog(<<"*!testMultiple1!*");

         set<NameAddr> contacts = mergeContacts(*jason, *derek);
         
         Seq(jason->registerUser(70, contacts),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 70)),
             500);

         ExecuteSequences();
      }
      
      void testRefresh()
      {
         WarningLog(<<"*!testRefresh!*");

         Seq(derek->registerUser(71, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 71)),
             500);
         ExecuteSequences();
         
         sleepSeconds(1);

         NameAddr con = *(derek->getDefaultContacts().begin());
         con.param(p_expires) = 6000;

         set<NameAddr> contacts;
         contacts.insert(con);

         Seq(derek->registerUser(72,contacts),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(contacts, 72)),
             500);
         
         ExecuteSequences();
      }

      void testChangeQValue()
      {         
         WarningLog(<<"*!testChangeQValue!*");

         NameAddr con = *(jason->getDefaultContacts().begin());
         con.param(p_q) = 0.1;
         set<NameAddr> contactsBefore;
         contactsBefore.insert(con);
         
         con.param(p_q) = 0.5;         
         set<NameAddr> contactsAfter;
         contactsAfter.insert(con);
         
         Seq(jason->registerUser(73, contactsBefore),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsBefore, 73)),
             500);
         ExecuteSequences();

         Seq(jason->registerUser(74, contactsAfter),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsAfter, 74)),
             500);
         ExecuteSequences();
      }
                               
      void testFetch()
      {         
         WarningLog(<<"*!testFetch!*");
         set<NameAddr> contacts = mergeContacts(*jason, *derek);
         
         Seq(jason->registerUser(75, contacts),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 75)),
             500);
         ExecuteSequences();
         
         set<NameAddr> nullSet;
         Seq(jason->registerUser(76, nullSet),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(contacts)),
             500);
         ExecuteSequences();
      }

      void testExpiryCleanup()
      {
         WarningLog(<<"*!testExpiryCleanup!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 60)),
             500);

         ExecuteSequences();
         sleepSeconds(61);
         // !jf! cause registration bindings to expire here
         assert(0);
         
         set<NameAddr> emptySet;
         Seq(derek->registerUser(78, emptySet),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 78)),
             500);
         ExecuteSequences();
      }     

      void testSetThenRemoveSpecific()
      {
         WarningLog(<<"*!testSetThenRemoveSpecific!*");

         Seq(derek->registerUser(79, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 79)),
             500);
         ExecuteSequences();

         NameAddr con = *(derek->getDefaultContacts().begin());
         con.param(p_expires) = 0;

         set<NameAddr> contacts;
         contacts.insert(con);

         set<NameAddr> emptySet;
         Seq(derek->registerUser(0, contacts),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();
      }

      void testSetTwoRemoveOne()
      {
         WarningLog(<<"*!testSetTwoRemoveOne!*");
         
         Seq(derek->registerUser(80, mergeContacts(*jason, *derek)),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(mergeContacts(*jason, *derek), 80)),
             500);
         ExecuteSequences();
         
         NameAddr tmpCon = *jason->getDefaultContacts().begin();
         tmpCon.param(p_expires) = 0;
         set<NameAddr> contacts;
         contacts.insert(tmpCon);
         
         set<NameAddr> emptySet;
         Seq(derek->registerUser(0, contacts),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(derek->getDefaultContacts())),
             500);
         ExecuteSequences();
      }

      void testSimulSetAndRemove()
      {
         WarningLog(<<"*!testSimulSetAndRemove!*");

         Seq(derek->registerUser(81, mergeContacts(*jason, *derek)),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(mergeContacts(*jason, *derek), 81)),
             500);
         ExecuteSequences();

         set<NameAddr> contacts;
         set<NameAddr> emptySet;

         NameAddr tmpCon = *jason->getDefaultContacts().begin();
         tmpCon.param(p_expires) = 0;
         contacts.insert(tmpCon);
         contacts.insert(*david->getDefaultContacts().begin());

         Seq(derek->registerUser(60, contacts),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(mergeContacts(*derek, *david))),
             500);
         ExecuteSequences();
      }

      //this doesn't work right now...removes have to be done specifically.
      //removes should be symmetric with refreshes
      void testSetThenRemoveByHeader()
      {
         WarningLog(<<"*!testSetThenRemoveByHeader!*");

         Seq(derek->registerUser(82, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 82)),
             500);
         ExecuteSequences();

         //CPPUNIT_ASSERT(LocationServer::Instance().exists(derek->getAddressOfRecordString()));

         set<NameAddr> emptySet;
         Seq(derek->registerUser(0, derek->getDefaultContacts()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();
         
         //CPPUNIT_ASSERT(LocationServer::Instance().count(derek->getAddressOfRecordString()) == 0);
      }

      void testSetThenRemoveAll()
      {
         WarningLog(<<"*!testSetThenRemoveAll!*");

         Seq(derek->registerUser(83, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         //CPPUNIT_ASSERT(LocationServer::Instance().count(derek->getAddressOfRecordString()) == 1);
         
         set<NameAddr> emptySet;
         Seq(derek->removeRegistrationBindings(),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();
         
         sleepSeconds(1);
         //CPPUNIT_ASSERT(LocationServer::Instance().count(derek->getAddressOfRecordString()) == 0);
      }

      void testUnregisterExpired()
      {
         WarningLog(<<"*!testUnregisterExpired!*");
         
         set<NameAddr> emptySet;
         Seq(derek->registerUser(3, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, 
                                  chain(derek->pause(4*Seconds),
                                        derek->registerUser(0, derek->getDefaultContacts()))),
             derek->expect(REGISTER/200, from(proxy), 4*Seconds + WaitForResponse, new CheckContacts(emptySet, 60)),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      // !dlb! move to ProvisioningUtils
      class SetCallId
      {
         public:
            SetCallId(const Data& callId) : 
               mCallId(callId)
            {}

            boost::shared_ptr<SipMessage> operator()(boost::shared_ptr<resip::SipMessage>& msg) const
            {
               DebugLog (<< "SetCallId=" << mCallId << endl << *msg);

               msg->header(h_CallId).value() = mCallId;

               return msg;
            }

         private:
            Data mCallId;
      };

      // !dlb! move to ProvisioningUtils
      class SetCSeqSequence
      {
         public:
            SetCSeqSequence(int seq) : 
               mSeq(seq)
            {}

            boost::shared_ptr<SipMessage> operator()(boost::shared_ptr<resip::SipMessage>& msg) const
            {
               DebugLog (<< "SetCSeqSequence=" << mSeq << endl << *msg);

               msg->header(h_CSeq).sequence() = mSeq;

               return msg;
            }

         private:
            int mSeq;
      };

      // !dlb! move to ProvisioningUtils
      class StripAuth
      {
         public:
            StripAuth()
            {}

            boost::shared_ptr<SipMessage> operator()(boost::shared_ptr<resip::SipMessage>& msg) const
            {
               DebugLog (<< "StripAuth" << endl << *msg);

               msg->remove(h_ProxyAuthorizations);
               msg->remove(h_WWWAuthenticates);

               return msg;
            }

         private:
            int mSeq;
      };

      void testRegisterNewCallId()
      {
         WarningLog(<< "*!testRegisterNewCallId!*");

         SetCallId scallid("callid2345");
         
         Seq(jason->registerUser(83, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         Seq(condition(scallid, jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);

         ExecuteSequences();
      }

      void testRegisterOutOfSequence()
      {
         WarningLog(<< "*!testRegisterOutOfSequence!*");

         SetCallId scallid("callid2346");
         SetCSeqSequence sseq(4);
         SetCSeqSequence sseq1(5);
         StripAuth stripAuths;
         
         Seq(condition(compose(sseq, scallid), jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), 1000, condition(sseq1, jason->digestRespond())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         Seq(condition(compose(stripAuths, sseq, scallid), jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), 1000, condition(sseq1, jason->digestRespond())),
             jason->expect(REGISTER/400, from(proxy), 10000, jason->noAction()),
             500);

         ExecuteSequences();
      }

      void testRegisterNewCallIdSameSequence()
      {
         WarningLog(<< "*!testRegisterNewCallIdSameSequence!*");

         SetCallId scallid("callid2347");
         SetCSeqSequence sseq(4);
         SetCSeqSequence sseq1(5);
         StripAuth stripAuths;
         
         Seq(condition(sseq, jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), 1000, condition(sseq1, jason->digestRespond())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         // new callid, same sequence
         Seq(condition(compose(stripAuths, sseq, scallid), jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), 1000, condition(compose(sseq1, scallid), jason->digestRespond())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);

         ExecuteSequences();
      }

      // provisioning here(automatic cleanup)
      static void createStatic()
      {
      }
};

#define TEST(_method) \
   suiteOfTests->addTest(new CppUnit::TestCaller<TestHolder>(#_method, &TestHolder::_method))
class MyTestCase
{
   public:
      static CppUnit::Test* suite()
      {
         CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "Suite1" );
#if 0
         TEST(testRegisterBasic);
         TEST(testRegisterClientRetransmits);
         TEST(testInviteBasic);
         TEST(testInviteClientRetransmissionsWithRecovery);
         TEST(testUnregisterAll);
         //TEST(testOversizeCallIdRegister);
         //TEST(testOversizeContactRegister);
         TEST(testInfo);
         TEST(testInviteClientRetransmitsAfter200);
         TEST(testNonInviteClientRetransmissionsWithRecovery);
         TEST(testNonInviteClientRetransmissionsWithTimeout);
         TEST(testNonInviteServerRetransmission);
         //TEST(testInviteTransportFailure);
         TEST(testInviteNoDNS);
         TEST(testBasic302);
         TEST(testInviteBusy);
         TEST(testInviteAllBusyContacts);
         TEST(testInviteCalleeHangsUp);
         TEST(testInviteCallerHangsUp);
         TEST(testInviteCallerCancels);
         TEST(testInviteCallerCancelsNo487);
         TEST(testInviteNoAnswerCancel);
         TEST(testInviteNotFound);
         TEST(testInviteNotFoundServerRetransmits);
         TEST(testInviteClientLateAck);
         TEST(testInviteClientMissedAck);
         TEST(testInviteClientMissedAck2);
         TEST(testInviteForRport);
         TEST(testAttendedExtensionToExtensionTransfer);
         TEST(testBlindTransferExtensionToExtensionHangupImmediately);
         TEST(testConferenceConferencorHangsUp);
         TEST(testInviteForkOneAnswers);
         TEST(testForkedInviteClientLateAck);
         TEST(testInviteForkOneBusy);
         TEST(testInviteForkBothAnswerNoProvisional);
         TEST(testInviteForkBothBusy);
         TEST(testInviteForkThreeCallerCancels);
         TEST(testInviteForkCallerCancels);         
#else
         TEST(testInviteBusy);
         //TEST(testInviteAllBusyContacts);
         TEST(testInviteCalleeHangsUp);
         TEST(testInviteCallerHangsUp);
         TEST(testInviteCallerCancels);
         TEST(testInviteCallerCancelsNo487);
         TEST(testInviteNoAnswerCancel);
         TEST(testInviteNotFound);
         TEST(testInviteNotFoundServerRetransmits);
#endif         
         return suiteOfTests;
      }
};

int main(int argc, char** argv)
{
   try
   {
      CommandLineParser args(argc, argv);
      Log::initialize(args.mLogType, args.mLogLevel, argv[0]);
      resip::Timer::T100 = 0;
      
      TestHolder::createStatic();
      Fixture::initialize(argc, argv);
      
      CppUnit::TextUi::TestRunner runner;

      runner.addTest( MyTestCase::suite() );
      runner.run();
      DebugLog(<< "Finished");

      Fixture::destroyStatic();
   }
   catch (BaseException& e)
   {
      cerr << "Fatal error: " << e << endl;
      exit(-1);
   }

   return 0;
}

// Copyright 2005 Purplecomm, Inc.
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
