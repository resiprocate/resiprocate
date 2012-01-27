#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

#include "resip/stack/ApiCheckList.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "tfm/remoteproxy/CommandLineParser.hxx"
#include "tfm/remoteproxy/ProxyFixture.hxx"
#include "tfm/RouteGuard.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/predicates/ExpectUtils.hxx"

#include "rutil/EsLogger.hxx"

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

// If defined test which are labeled with BUGTEST or BADTESTS will
// be run.
//#define RUN_ALL_TESTS
// TODO: Probably should make it a runtime choice to run all tests.
static const int sRunTestsKnownToFail = 
#ifdef RUN_ALL_TESTS
                                        1;
#else
                                        0;
#endif

const Data transport("udp");
static NameAddr localhost;
static NameAddr all;
static const set<NameAddr> emptySet;

void sleepSeconds(unsigned int seconds)
{
#ifdef WIN32
   Sleep(seconds*1000);
#else
   sleep(seconds);
#endif
}

class TestHolder : public ProxyFixture
{
   public:
            static boost::shared_ptr<SipMessage>
      bogusAuth(boost::shared_ptr<SipMessage> msg)
      {
         if(msg->exists(h_ProxyAuthorizations))
         {
            Auths::iterator i = msg->header(h_ProxyAuthorizations).begin();
            for(; i!=msg->header(h_ProxyAuthorizations).end(); ++i)
            {
               i->param(p_response)="ffffffffffffffffffffffffffffffff";
            }
         }

         if(msg->exists(h_Authorizations))
         {
            Auths::iterator i = msg->header(h_Authorizations).begin();
            for(; i!=msg->header(h_Authorizations).end(); ++i)
            {
               i->param(p_response)="ffffffffffffffffffffffffffffffff";
            }
         
         }
      
         return msg;
      }


      static boost::shared_ptr<SipMessage>
      noUserInTo(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(h_To).uri().user()="";
         return msg;
      }
         
      static boost::shared_ptr<SipMessage>
      userInReqUri(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(h_RequestLine).uri().user()="boom";
         return msg;
      }

   static boost::shared_ptr<SipMessage>
   unknownHostInTo(boost::shared_ptr<SipMessage> msg)
   {
      
      msg->header(h_To).uri().host()="po9eiwi7hpfnqvn89hpbcn";
      return msg;
   }
   
   static boost::shared_ptr<SipMessage>
   unknownHostInFrom(boost::shared_ptr<SipMessage> msg)
   {
      
      msg->header(h_From).uri().host()="po9eiwi7hpfnqvn89hpbcn";
      return msg;
   }
   
   static boost::shared_ptr<SipMessage>
   unknownUserInTo(boost::shared_ptr<SipMessage> msg)
   {
      msg->header(h_To).uri().user()="n97n3qwbefcpp";
      return msg;
   }

   static boost::shared_ptr<SipMessage>
   unknownUserInFrom(boost::shared_ptr<SipMessage> msg)
   {
      msg->header(h_From).uri().user()="n97n3qwbefcpp";
      return msg;
      }

      static boost::shared_ptr<SipMessage>
      largeCallId(boost::shared_ptr<SipMessage> msg)
      {
         const int oversize = 4096;
         Data callId(oversize, Data::Preallocate);
         for (int i = 0; i < oversize/resip::Random::maxLength; ++i)
         {
            callId += resip::Random::getRandomHex(resip::Random::maxLength);
         }
         callId += resip::Random::getRandomHex(oversize - resip::Random::maxLength*(oversize/resip::Random::maxLength));
         msg->header(h_CallId).value() = callId;

         return msg;
      }

      static boost::shared_ptr<SipMessage>
      largeContact(boost::shared_ptr<SipMessage> msg)
      {
         assert(msg->exists(h_Contacts) &&
                !msg->header(h_Contacts).empty());
         
         const int oversize = 4096;
         Data contactUser(oversize, Data::Preallocate);
         for (int i = 0; i < oversize/resip::Random::maxLength; ++i)
         {
            contactUser += resip::Random::getRandomHex(resip::Random::maxLength);
         }
         contactUser += resip::Random::getRandomHex(oversize - resip::Random::maxLength*(oversize/resip::Random::maxLength));
         msg->header(h_Contacts).front().uri().user() = contactUser;

         return msg;
      }

      static boost::shared_ptr<SipMessage>
      addRecordRoute(boost::shared_ptr<SipMessage> msg)
      {
         resip::NameAddr rr("bogus@blahblahblah");
         msg->header(h_RecordRoutes).push_front(rr);
         return msg;
      }
      
      class AssertNoRecordRoutes : public ExpectAction
      {
         public:
            AssertNoRecordRoutes();
            virtual void operator()(boost::shared_ptr<Event> event)
            {
               SipEvent* sipEvent = dynamic_cast<SipEvent*>(event.get());
               assert(sipEvent);
               boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();
               assert(!msg->exists(h_RecordRoutes));
            }
      };

///***************************************** tests start here ********************************//

//*****************************Registrar tests********************************//

//***********************New Registration Cases******************//

//*********Sunny day cases**********//



   void testRegisterBasic()
      {
         WarningLog(<<"*!testRegisterBasic!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

   //See RFC3261 Sec 10.2, para 3
   void testRegisterWithRecordRoute()
   {
      WarningLog(<<"*!testRegisterWithRecordRoute!*");

      Seq(condition(addRecordRoute,jason->registerUser(60, jason->getDefaultContacts())),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, chain(new AssertNoRecordRoutes,condition(addRecordRoute,jason->digestRespond()))),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new AssertNoRecordRoutes),
          WaitForEndOfTest);
   
      ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }
   
      void testMultiple1()
      {
         WarningLog(<<"*!testMultiple1!*");

         set<NameAddr> contacts = mergeContacts(*jason, *derek);
         
         Seq(jason->registerUser(70, contacts),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 70)),
             500);

         ExecuteSequences();


         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
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
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 3000)),
             500);
         
         ExecuteSequences();  

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

   void testThirdPartyRegistration()
      {
      WarningLog(<<"*!testThirdPartyRegistration!*");
      set<NameAddr> contacts;
      
      
      contacts.insert(*(derek->getDefaultContacts().begin()));

      Seq
      (
         jason->registerUser(0,all),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckFetchedContacts(emptySet)),
         500
      );
      
      ExecuteSequences();

      Seq
      (
         jason->registerUser(3000,contacts),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckContacts(contacts,3000)),
         500
      );
      
         ExecuteSequences();
         
      Seq
      (
         jason->registerUser(0,all),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckFetchedContacts(emptySet)),
         500
      );
      
      ExecuteSequences();
         
   }
   
   
      void testDetailsQValue()
      {
         WarningLog(<<"*!testDetailsQValue!*");
         
         NameAddr con = *(jason->getDefaultContacts().begin());
         
         set<NameAddr> contacts;
         contacts.insert(con);
                     
         Seq(jason->registerUser(67, con),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(contacts, 67)),
             500);
         
         ExecuteSequences();  

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
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
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 68)),
             500);
         
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

//*********Rainy day cases**********//

   void testRegister407Dropped()
         {
      WarningLog(<<"*!testRegister407Dropped!*");
      
      boost::shared_ptr<SipMessage> reg;
      Seq
      (
         save(reg,jason->registerUser(60,jason->getDefaultContacts())),
         jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->retransmit(reg)),
         jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
         }

   
   void testRegisterBogusAuth()
   {
      WarningLog(<<"*!testRegisterBogusAuth!*");
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/401, from(proxy), WaitForResponse, condition(bogusAuth,jason->digestRespond())),
         jason->expect(REGISTER/403, from(proxy), WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      }

   void testRegisterLateDigestResponse()
      {
      WarningLog(<<"*!testRegisterLateDigestResponse!*");
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/401, from(proxy), WaitForResponse, chain(jason->pause(3000000),jason->digestRespond())),
         jason->expect(REGISTER/401, from(proxy), 3000000, jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60) ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }
   
      void testRegisterClientRetransmits()
      {
         WarningLog(<<"*!testRegisterClientRetransmits!*");

         boost::shared_ptr<SipMessage> reg;
         Seq(save(reg, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->retransmit(reg)),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

   void testRegisterNoUserInTo()
   {
      WarningLog(<<"*!testRegisterNoUserInTo!*");
      
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()), //There would be a condition noUserInTo here, but that would corrupt the registration message that gets reused in every test. (This is broken behavior on tfm's part)
         jason->expect(REGISTER/401,from(proxy), WaitForResponse, condition(noUserInTo,jason->digestRespond())),
         jason->expect(REGISTER/403,from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }



   void testRegisterUserInReqUri()
   {
      WarningLog(<<"*!testRegisterUserInReqUri!*");
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/401,from(proxy), WaitForResponse, condition(userInReqUri,jason->digestRespond())),
         jason->expect(REGISTER/400,from(proxy),WaitForResponse,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testRegisterUnknownAorHost()
   {
      WarningLog(<<"*!testRegisterUnknownAorHost!*");
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,condition(unknownHostInTo,jason->digestRespond())),
         jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testRegisterUnknownAorUser()
   {
      WarningLog(<<"*!testRegisterUnknownAorUser!*");
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
         jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

      void testOversizeCallIdRegister()
      {
         WarningLog(<<"*!testOversizeCallIdRegister!*");
         
         Seq(condition(largeCallId, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/400, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testOversizeContactRegister()
      {
         WarningLog(<<"*!testOversizeContactRegister!*");
         Seq(condition(largeContact, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/500, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

//*******************Refresh type tests*******************/

//*********Sunny day cases**********//

      void testRefresh()
      {
         WarningLog(<<"*!testRefresh!*");

         Seq(derek->registerUser(71, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 71)),
             500);
         ExecuteSequences();
         
         sleepSeconds(1);

         NameAddr con = *(derek->getDefaultContacts().begin());
         con.param(p_expires) = 6000;

         set<NameAddr> contacts;
         contacts.insert(con);

         Seq(derek->registerUser(72,contacts),
            optional(derek->expect(REGISTER/401,from(proxy),WaitForResponse, derek->digestRespond())),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(contacts, 72)),
             500);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

   void testRefreshMulti()
   {
      WarningLog(<<"*!testRefreshMulti!*");
   
      set<NameAddr> contacts = mergeContacts(*jason, *derek);

      Seq
      (
         jason->registerUser(60,contacts),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(contacts,60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      sleepSeconds(1);
      
      Seq
      (
         jason->registerUser(120,contacts),
         optional(jason->expect(REGISTER/401,from(proxy),WaitForResponse, jason->digestRespond())),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse, new CheckContacts(contacts,120)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   
         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   void testRefreshThirdParty()
   {
      WarningLog(<<"*!testRefreshThirdParty!*");
      
      Seq
      (
         jason->registerUser(0,all),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckFetchedContacts(emptySet)),
         500
      );
      
      ExecuteSequences();

      Seq
      (
         jason->registerUser(60,derek->getDefaultContacts()),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jason->registerUser(120,derek->getDefaultContacts()),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),120)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jason->registerUser(0,all),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckFetchedContacts(emptySet)),
         500
      );
      
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
         
         con.remove(p_q);
         set<NameAddr> withoutQ;
         withoutQ.insert(con);
         
         Seq(jason->registerUser(73, contactsBefore),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsBefore, 73)),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(74, contactsAfter),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsAfter, 74)),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(74, withoutQ),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(withoutQ, 74)),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }
                               

//*********Rainy day cases**********//

       
       
//*******************Unregister type tests*******************/

//*********Sunny day cases**********//

      void testSetThenRemoveSpecific()
      {
         WarningLog(<<"*!testSetThenRemoveSpecific!*");

         Seq(derek->registerUser(79, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 79)),
             500);
         ExecuteSequences();

         NameAddr con = *(derek->getDefaultContacts().begin());
         con.param(p_expires) = 0;

         set<NameAddr> contacts;
         contacts.insert(con);

         Seq(derek->registerUser(0, contacts),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

   void testUnregisterMulti()
   {
      WarningLog(<<"*!testUnregisterMulti!*");
      
      set<NameAddr> contacts = mergeContacts(*jason, *derek);
      
      Seq(jason->registerUser(70, contacts),
          jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 70)),
          500);

      ExecuteSequences();

      Seq
      (
         jason->registerUser(0,contacts),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(emptySet,0)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   
   }

   void testUnregisterExpired()
   {
      WarningLog(<<"*!testUnregisterExpired!*");
      
      Seq(derek->registerUser(5, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      sleepSeconds(6);
      
      
      Seq
      (
         derek->registerUser(0, derek->getDefaultContacts()),
         derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), 4*Seconds + WaitForResponse, new CheckContacts(emptySet, 0)),
         WaitForEndOfTest
      );
      ExecuteSequences();

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   void testUnregisterAll()
   {
      WarningLog(<<"*!testUnregisterAll!*");
      
      NameAddr na;
      
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(jason1->getDefaultContacts())),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts( mergeContacts(*jason1, *jason2) )),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason1->registerUser(0, all ),
          jason1->expect(REGISTER/401,from(proxy),WaitForResponse,jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   

//*********Rainy day cases**********//

   void testUnregisterAllBadExpires()
   {
      WarningLog(<<"*!testUnregisterAllBadExpires!*");
      
      
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason1->getDefaultContacts(),60)),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts( mergeContacts(*jason1, *jason2) )),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason1->registerUser(10, all ),
          jason1->expect(REGISTER/401,from(proxy),WaitForResponse,jason1->digestRespond()),
          jason1->expect(REGISTER/400, from(proxy), WaitForResponse, jason1->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq
      (
         jason->registerUser(0,all),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckFetchedContacts(emptySet)),
         500
      );
      
      ExecuteSequences();
      
   }

   void testUnregisterNonExistent()
   {
      WarningLog(<<"*!testUnregisterNonExistent!*");
      
      
      Seq
      (
         condition(unknownUserInTo,jason->registerUser(0,jason->getDefaultContacts())),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
         jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
       
       
//*******************Fetch type tests*******************/

//*********Sunny day cases**********//

   void testFetch()
   {         
      WarningLog(<<"*!testFetch!*");
      set<NameAddr> contacts = mergeContacts(*jason, *derek);
      
      Seq(jason->registerUser(75, contacts),
          jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 75)),
          500);
      ExecuteSequences();
      
      set<NameAddr> nullSet;
      Seq(jason->registerUser(76, nullSet),
          jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(contacts)),
          500);
      ExecuteSequences();
      
      
      Seq
      (
         jason->registerUser(0,all),
         jason->expect(REGISTER/401, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckFetchedContacts(emptySet)),
         500
      );
      
      ExecuteSequences();
      
   }

   void testExpiryCleanup()
   {
      WarningLog(<<"*!testExpiryCleanup!*");

      Seq(derek->registerUser(10, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 60)),
          500);

      ExecuteSequences();
      sleepSeconds(12);
      // !jf! cause registration bindings to expire here
      
      Seq(derek->registerUser(78, emptySet),
          derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 78)),
          500);
      ExecuteSequences();

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }     



//*********Rainy day cases**********//

   void testFetchNonExistent()
   {         
      WarningLog(<<"*!testFetchNonExistent!*");

      set<NameAddr> nullSet;
      Seq
      (
         condition(unknownUserInTo,jason->registerUser(76, nullSet)),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
         jason->expect(REGISTER/404, from(proxy), 10000, jason->noAction()),
         500
      );
      
      ExecuteSequences();
   }


//**************************INVITE scenarios************************//


//*******************Non-forking INVITES******************//


//*************Sunny Day***************//       
       
   void testInviteBasic()
   {
      WarningLog(<<"*!testInviteBasic!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
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

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   void testInviteBasicTls()
   {
      WarningLog(<<"*!testInviteBasicTls!*");
      
      Seq
      (
         derekTls->registerUser(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/401, from(proxy), WaitForResponse, derekTls->digestRespond()),
         derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, derekTls->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->invite(*derekTls),
         optional(jasonTls->expect(INVITE/100, from(proxy), WaitFor100, jasonTls->noAction())),
         jasonTls->expect(INVITE/407, from(proxy), WaitForResponse, chain(jasonTls->ack(), jasonTls->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTls->expect(INVITE/100, from(proxy), WaitFor100, jasonTls->noAction()))
            ),
            Sub
            (
               derekTls->expect(INVITE, contact(jasonTls), WaitForCommand, chain(derekTls->ring(), derekTls->answer())),
               jasonTls->expect(INVITE/180, from(derekTls), WaitFor100, jasonTls->noAction()),
               jasonTls->expect(INVITE/200, contact(derekTls), WaitForResponse, jasonTls->ack()),
               derekTls->expect(ACK, from(jasonTls), WaitForResponse, jasonTls->noAction())
            )
         ),
         WaitForEndOfTest
      );
         
      
      ExecuteSequences();

         Seq(derekTls->registerUser(0, all),
             derekTls->expect(REGISTER/401, from(proxy), WaitForResponse, derekTls->digestRespond()),
             derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, derekTls->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   void testInviteBasicRedirect()
   {
      WarningLog(<<"*!testInviteBasicRedirect!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/302,from(proxy),WaitForResponse,chain(new CheckContacts(derek->getDefaultContacts(),60),jason->ack())),
          WaitForEndOfTest);
      ExecuteSequences();  

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   
   void testInviteCallerHangsUp()
   {
      WarningLog(<<"*!testInviteCallerHangsUp!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
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

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   void testInviteCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteCalleeHangsUp!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfSeq);
      
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

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }

   void testInviteCallerCancels()
   {
      WarningLog(<<"*!testInviteCallerCancels!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
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

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }


   void testInviteBusy()
   {
      WarningLog(<<"*!testInviteBusy!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

         Seq(derek->registerUser(0, all),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(jason->registerUser(0, all),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
   }


//*************Cloudy Day***************//

   void testSpiral()
   {
      WarningLog(<<"*!testSpiral!*");

      
      Seq(cullen->registerUser(60, proxy->makeUrl("jason")),
          cullen->expect(REGISTER/401, from(proxy), WaitForResponse, cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForRegistration, cullen->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUser(60, proxy->makeUrl("enlai")),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(enlai->registerUser(60, proxy->makeUrl("david")),
          enlai->expect(REGISTER/401, from(proxy), WaitForResponse, enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForRegistration, enlai->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(david->registerUser(60, david->getDefaultContacts()),
          david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*cullen),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(david->expect(INVITE, contact(derek), WaitForCommand, chain(david->ring(), david->answer())),
                  derek->expect(INVITE/180, from(david), WaitFor100, derek->noAction()),
                  derek->expect(INVITE/200, contact(david), WaitForResponse, derek->ack()),
                  david->expect(ACK, from(derek), WaitForResponse, derek->noAction()))),
          WaitForEndOfTest);
      ExecuteSequences();  
      
            
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(david->registerUser(0, all ),
          david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(cullen->registerUser(0, all ),
          cullen->expect(REGISTER/401,from(proxy),WaitForResponse,cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      
      
   }


   void testSpiralWithCancel()
   {
      WarningLog(<<"*!testSpiralWithCancel!*");

      Seq(cullen->registerUser(60, proxy->makeUrl("jason")),
          cullen->expect(REGISTER/401, from(proxy), WaitForResponse, cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForRegistration, cullen->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUser(60, proxy->makeUrl("enlai")),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(enlai->registerUser(60, proxy->makeUrl("david")),
          enlai->expect(REGISTER/401, from(proxy), WaitForResponse, enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForRegistration, enlai->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(david->registerUser(60, david->getDefaultContacts()),
          david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq
      (
         derek->invite(*cullen),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),derek->digestRespond())),

         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
               derek->expect(INVITE/180, from(david), WaitFor100, derek->cancel()),
               derek->expect(CANCEL/200, from(proxy),WaitForResponse, derek->noAction()),
               derek->expect(INVITE/487, from(david),WaitForResponse, derek->ack())
            ),
            Sub
            (
               david->expect(INVITE, contact(derek), WaitForCommand, david->ring()),
               david->expect(CANCEL, from(proxy), WaitForResponse, chain(david->ok(),david->send487())),
               david->expect(ACK, from(proxy),WaitForResponse,david->noAction())
            )
         ),
         
         WaitForEndOfTest
      );
      ExecuteSequences();
      
      
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(david->registerUser(0, all ),
          david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(cullen->registerUser(0, all ),
          cullen->expect(REGISTER/401,from(proxy),WaitForResponse,cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
   }

   void testInviteCallerCancelsWithRace()
   {
      WarningLog(<<"*!testInviteCallerCancelsWithRace!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
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

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInvite200BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite200BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          1000);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
         jason->expect(INVITE/407,from(proxy),WaitForResponse,chain(jason->ack(),jason->digestRespond())),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),WaitForResponse,jason->noAction())),
               jason->expect(INVITE/180,from(proxy),WaitForResponse,jason->noAction()),
               jason->expect(INVITE/200,contact(derek),WaitForResponse,chain(jason->cancel(),jason->pause(30),jason->ack())),
               jason->expect(CANCEL/200,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,chain(derek->ring(),derek->answer())),
               derek->expect(ACK,contact(jason),WaitForCommand,derek->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInvite486BeatsCancelServerSide()
   {
      WarningLog(<<"*!testInvite486BeatsCancelServerSide!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
         1000
      );
      ExecuteSequences();

      Seq
      (
         david->invite(*derek),
         optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
         david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
          
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE, contact(david), WaitForCommand, derek->ring()),
               david->expect(INVITE/180, from(derek), WaitFor180, chain(derek->send486(), david->cancel())),
               derek->expect(ACK, from(proxy), WaitForResponse, derek->noAction())
            ),
            Sub
            (
               david->expect(CANCEL/200, from(proxy), WaitForCommand, david->noAction())
            ),
            Sub
            (
               david->expect(INVITE/486, contact(derek),WaitForResponse,david->ack())
            )
            
         ),
         WaitForEndOfTest
      );    
      
      ExecuteSequences();  

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInvite486BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite486BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          1000);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
         jason->expect(INVITE/407,from(proxy),WaitForResponse,chain(jason->ack(),jason->digestRespond())),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),WaitForResponse,jason->noAction())),
               jason->expect(INVITE/180,contact(derek),WaitForResponse,jason->noAction()),
               jason->expect(INVITE/486,contact(derek),WaitForResponse,chain(jason->cancel(),jason->pause(30),jason->ack())),
               jason->expect(CANCEL/200,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,chain(derek->ring(),derek->send486())),
               derek->expect(ACK,from(proxy),WaitForCommand,derek->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }



   void testInvite503BeatsCancelServerSide()
   {
      WarningLog(<<"*!testInvite503BeatsCancelServerSide!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
         1000
      );
      ExecuteSequences();

      Seq
      (
         david->invite(*derek),
         optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
         david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
          
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE, contact(david), WaitForCommand, derek->ring()),
               david->expect(INVITE/180, from(derek), WaitFor180, chain(derek->send503(), david->cancel())),
               derek->expect(ACK, from(proxy), WaitForResponse, derek->noAction())
            ),
            Sub
            (
               david->expect(CANCEL/200, from(proxy), WaitForCommand, david->noAction())
            ),
            Sub
            (
               david->expect(INVITE/480, contact(derek),WaitForResponse,david->ack())
            )
            
         ),

         WaitForEndOfTest
      );    
      
      ExecuteSequences();  

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInvite503BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite503BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          1000);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
         jason->expect(INVITE/407,from(proxy),WaitForResponse,chain(jason->ack(),jason->digestRespond())),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),WaitForResponse,jason->noAction())),
               jason->expect(INVITE/180,contact(derek),WaitForResponse,jason->noAction()),
               jason->expect(INVITE/480,contact(derek),WaitForResponse,chain(jason->cancel(),jason->pause(30),jason->ack())),
               jason->expect(CANCEL/200,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,chain(derek->ring(),derek->send503())),
               derek->expect(ACK,from(proxy),WaitForCommand,derek->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
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

   void testInvite488Response()
   {
      WarningLog(<<"*!testInvite488Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

            And
            (
               Sub
               (
                  optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))
               ),
               Sub
               (
                  jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send488())),
                  And
                  (
                     Sub
                     (
                        jason->expect(ACK, from(proxy), WaitForAck, jason->noAction())
                     ),
                     Sub
                     (
                        derek->expect(INVITE/488, from(proxy), WaitForResponse, derek->ack())
                     )
                  )
               )
            ),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInvite480Response()
   {
      WarningLog(<<"*!testInvite480Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send480())),
                  jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                  derek->expect(INVITE/480, from(proxy), WaitForResponse, derek->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInvite500Response()
   {
      WarningLog(<<"*!testInvite500Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send500())),
                  jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                  derek->expect(INVITE/500, from(proxy), WaitForResponse, derek->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInvite503Response()
   {
      WarningLog(<<"*!testInvite503Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send503())),
                  jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                  derek->expect(INVITE/480, from(proxy), WaitForResponse, derek->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   

   void testInvite600Response()
   {
      WarningLog(<<"*!testInvite600Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send600())),
                  jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                  derek->expect(INVITE/600, from(proxy), WaitForResponse, derek->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInvite603Response()
   {
      WarningLog(<<"*!testInvite603Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send603())),
                  jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                  derek->expect(INVITE/603, from(proxy), WaitForResponse, derek->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteServerSpams180()
   {
      WarningLog(<<"*!testInviteServerSpams180!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();


      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitForResponse,derek->noAction())),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,from(proxy),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->ring(),jason->ring(),jason->answer()))
            )
         ),
         jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteBogusAuth()
   {
      WarningLog(<<"*!testInviteBogusAuth!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), condition(bogusAuth,jason->digestRespond()))),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/403,from(proxy),WaitForResponse,jason->ack()),
         WaitForEndOfTest
      
      );
      ExecuteSequences();  

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void
   testInviteUDPToTCPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTCPCallerHangsUp!*");
      Seq
      (
         jasonTcp->registerUser(60, jasonTcp->getDefaultContacts()),
         jasonTcp->expect(REGISTER/401,from(proxy),1000,jasonTcp->digestRespond()),
         jasonTcp->expect(REGISTER/200,from(proxy),1000,new CheckContacts(jasonTcp->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         derek->invite(*jasonTcp),
         optional(derek->expect(INVITE/100,from(proxy),1000,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),1000,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),1000,derek->noAction()))
            ),
            Sub
            (
               jasonTcp->expect(INVITE,contact(derek),1000,chain(jasonTcp->ring(),jasonTcp->answer())),
               derek->expect(INVITE/180,contact(jasonTcp),1000,derek->noAction()),
               derek->expect(INVITE/200,contact(jasonTcp),1000,derek->ack()),
               jasonTcp->expect(ACK,from(proxy),1000,chain(jasonTcp->pause(1000),derek->bye())),
               jasonTcp->expect(BYE,from(proxy),2000,jasonTcp->ok()),
               derek->expect(BYE/200,contact(jasonTcp),1000,derek->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();

      Seq(jasonTcp->registerUser(0, all ),
          jasonTcp->expect(REGISTER/401,from(proxy),WaitForResponse,jasonTcp->digestRespond()),
          jasonTcp->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void
   testInviteUDPToTCPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTCPCalleeHangsUp!*");
      Seq
      (
         jasonTcp->registerUser(60, jasonTcp->getDefaultContacts()),
         jasonTcp->expect(REGISTER/401,from(proxy),1000,jasonTcp->digestRespond()),
         jasonTcp->expect(REGISTER/200,from(proxy),1000,new CheckContacts(jasonTcp->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         derek->invite(*jasonTcp),
         optional(derek->expect(INVITE/100,from(proxy),1000,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),1000,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),1000,derek->noAction()))
            ),
            Sub
            (
               jasonTcp->expect(INVITE,contact(derek),1000,chain(jasonTcp->ring(),jasonTcp->answer())),
               derek->expect(INVITE/180,contact(jasonTcp),1000,derek->noAction()),
               derek->expect(INVITE/200,contact(jasonTcp),1000,derek->ack()),
               jasonTcp->expect(ACK,from(proxy),1000,chain(jasonTcp->pause(1000),jasonTcp->bye())),
               derek->expect(BYE,from(proxy),2000,derek->ok()),
               jasonTcp->expect(BYE/200,contact(derek),1000,jasonTcp->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();

      Seq(jasonTcp->registerUser(0, all ),
          jasonTcp->expect(REGISTER/401,from(proxy),WaitForResponse,jasonTcp->digestRespond()),
          jasonTcp->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void
   testInviteTCPToUDPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToUDPCallerHangsUp!*");
      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/401,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTcp->invite(*derek),
         optional(jasonTcp->expect(INVITE/100,from(proxy),1000,jasonTcp->noAction())),
         jasonTcp->expect(INVITE/407,from(proxy),1000,chain(jasonTcp->ack(),jasonTcp->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTcp->expect(INVITE/100,from(proxy),1000,derek->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,contact(jasonTcp),1000,chain(derek->ring(),derek->answer())),
               jasonTcp->expect(INVITE/180,contact(derek),1000,jasonTcp->noAction()),
               jasonTcp->expect(INVITE/200,contact(derek),1000,jasonTcp->ack()),
               derek->expect(ACK,from(proxy),1000,chain(derek->pause(1000),jasonTcp->bye())),
               derek->expect(BYE,from(proxy),2000,derek->ok()),
               jasonTcp->expect(BYE/200,contact(derek),1000,jasonTcp->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void
   testInviteTCPToUDPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToUDPCalleeHangsUp!*");
      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/401,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTcp->invite(*derek),
         optional(jasonTcp->expect(INVITE/100,from(proxy),1000,jasonTcp->noAction())),
         jasonTcp->expect(INVITE/407,from(proxy),1000,chain(jasonTcp->ack(),jasonTcp->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTcp->expect(INVITE/100,from(proxy),1000,derek->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,contact(jasonTcp),1000,chain(derek->ring(),derek->answer())),
               jasonTcp->expect(INVITE/180,contact(derek),1000,jasonTcp->noAction()),
               jasonTcp->expect(INVITE/200,contact(derek),1000,jasonTcp->ack()),
               derek->expect(ACK,from(proxy),1000,chain(derek->pause(1000),derek->bye())),
               jasonTcp->expect(BYE,from(proxy),2000,jasonTcp->ok()),
               derek->expect(BYE/200,contact(jasonTcp),1000,derek->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteUDPToTLSCallerHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTLSCallerHangsUp!*");
      Seq
      (
         derekTls->registerUser(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/401,from(proxy),1000,derekTls->digestRespond()),
         derekTls->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTls->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derekTls),
         optional(jason->expect(INVITE/100,from(proxy),1000,jason->noAction())),
         jason->expect(INVITE/407,from(proxy),1000,chain(jason->ack(),jason->digestRespond())),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),1000,derekTls->noAction()))
            ),
            Sub
            (
               derekTls->expect(INVITE,contact(jason),1000,chain(derekTls->ring(),derekTls->answer())),
               jason->expect(INVITE/180,contact(derekTls),1000,jason->noAction()),
               jason->expect(INVITE/200,contact(derekTls),1000,jason->ack()),
               derekTls->expect(ACK,from(proxy),1000,chain(jason->pause(1000),jason->bye())),
               derekTls->expect(BYE,from(proxy),2000,derekTls->ok()),
               jason->expect(BYE/200,contact(derekTls),1000,jason->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derekTls->registerUser(0, all ),
          derekTls->expect(REGISTER/401,from(proxy),WaitForResponse,derekTls->digestRespond()),
          derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteUDPToTLSCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTLSCalleeHangsUp!*");
      Seq
      (
         derekTls->registerUser(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/401,from(proxy),1000,derekTls->digestRespond()),
         derekTls->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTls->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derekTls),
         optional(jason->expect(INVITE/100,from(proxy),1000,jason->noAction())),
         jason->expect(INVITE/407,from(proxy),1000,chain(jason->ack(),jason->digestRespond())),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),1000,derekTls->noAction()))
            ),
            Sub
            (
               derekTls->expect(INVITE,contact(jason),1000,chain(derekTls->ring(),derekTls->answer())),
               jason->expect(INVITE/180,contact(derekTls),1000,jason->noAction()),
               jason->expect(INVITE/200,contact(derekTls),1000,jason->ack()),
               derekTls->expect(ACK,from(proxy),1000,chain(derekTls->pause(1000),derekTls->bye())),
               jason->expect(BYE,from(proxy),2000,jason->ok()),
               derekTls->expect(BYE/200,contact(jason),1000,derekTls->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derekTls->registerUser(0, all ),
          derekTls->expect(REGISTER/401,from(proxy),WaitForResponse,derekTls->digestRespond()),
          derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTCPToTLSCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToTLSCallerHangsUp!*");
      Seq
      (
         derekTls->registerUser(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/401,from(proxy),1000,derekTls->digestRespond()),
         derekTls->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTls->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTcp->invite(*derekTls),
         optional(jasonTcp->expect(INVITE/100,from(proxy),1000,jasonTcp->noAction())),
         jasonTcp->expect(INVITE/407,from(proxy),1000,chain(jasonTcp->ack(),jasonTcp->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTcp->expect(INVITE/100,from(proxy),1000,derekTls->noAction()))
            ),
            Sub
            (
               derekTls->expect(INVITE,contact(jasonTcp),1000,chain(derekTls->ring(),derekTls->answer())),
               jasonTcp->expect(INVITE/180,contact(derekTls),1000,jasonTcp->noAction()),
               jasonTcp->expect(INVITE/200,contact(derekTls),1000,jasonTcp->ack()),
               derekTls->expect(ACK,from(proxy),1000,chain(jasonTcp->pause(1000),jasonTcp->bye())),
               derekTls->expect(BYE,from(proxy),2000,derekTls->ok()),
               jasonTcp->expect(BYE/200,contact(derekTls),1000,jasonTcp->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derekTls->registerUser(0, all ),
          derekTls->expect(REGISTER/401,from(proxy),WaitForResponse,derekTls->digestRespond()),
          derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTCPToTLSCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToTLSCalleeHangsUp!*");
      Seq
      (
         derekTls->registerUser(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/401,from(proxy),1000,derekTls->digestRespond()),
         derekTls->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTls->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTcp->invite(*derekTls),
         optional(jasonTcp->expect(INVITE/100,from(proxy),1000,jasonTcp->noAction())),
         jasonTcp->expect(INVITE/407,from(proxy),1000,chain(jasonTcp->ack(),jasonTcp->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTcp->expect(INVITE/100,from(proxy),1000,derekTls->noAction()))
            ),
            Sub
            (
               derekTls->expect(INVITE,contact(jasonTcp),1000,chain(derekTls->ring(),derekTls->answer())),
               jasonTcp->expect(INVITE/180,contact(derekTls),1000,jasonTcp->noAction()),
               jasonTcp->expect(INVITE/200,contact(derekTls),1000,jasonTcp->ack()),
               derekTls->expect(ACK,from(proxy),1000,chain(derekTls->pause(1000),derekTls->bye())),
               jasonTcp->expect(BYE,from(proxy),2000,jasonTcp->ok()),
               derekTls->expect(BYE/200,contact(jasonTcp),1000,derekTls->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derekTls->registerUser(0, all ),
          derekTls->expect(REGISTER/401,from(proxy),WaitForResponse,derekTls->digestRespond()),
          derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTLSToUDPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToUDPCallerHangsUp!*");
      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/401,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->invite(*derek),
         optional(jasonTls->expect(INVITE/100,from(proxy),1000,jasonTls->noAction())),
         jasonTls->expect(INVITE/407,from(proxy),1000,chain(jasonTls->ack(),jasonTls->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTls->expect(INVITE/100,from(proxy),1000,derek->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,contact(jasonTls),1000,chain(derek->ring(),derek->answer())),
               jasonTls->expect(INVITE/180,contact(derek),1000,jasonTls->noAction()),
               jasonTls->expect(INVITE/200,contact(derek),1000,jasonTls->ack()),
               derek->expect(ACK,from(proxy),1000,chain(jasonTls->pause(1000),jasonTls->bye())),
               derek->expect(BYE,from(proxy),2000,derek->ok()),
               jasonTls->expect(BYE/200,contact(derek),1000,jasonTls->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTLSToUDPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToUDPCalleeHangsUp!*");
      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/401,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->invite(*derek),
         optional(jasonTls->expect(INVITE/100,from(proxy),1000,jasonTls->noAction())),
         jasonTls->expect(INVITE/407,from(proxy),1000,chain(jasonTls->ack(),jasonTls->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTls->expect(INVITE/100,from(proxy),1000,derek->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,contact(jasonTls),1000,chain(derek->ring(),derek->answer())),
               jasonTls->expect(INVITE/180,contact(derek),1000,jasonTls->noAction()),
               jasonTls->expect(INVITE/200,contact(derek),1000,jasonTls->ack()),
               derek->expect(ACK,from(proxy),1000,chain(derek->pause(1000),derek->bye())),
               jasonTls->expect(BYE,from(proxy),2000,jasonTls->ok()),
               derek->expect(BYE/200,contact(jasonTls),1000,derek->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTLSToTCPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToTCPCallerHangsUp!*");
      Seq
      (
         derekTcp->registerUser(60, derekTcp->getDefaultContacts()),
         derekTcp->expect(REGISTER/401,from(proxy),1000,derekTcp->digestRespond()),
         derekTcp->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTcp->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->invite(*derekTcp),
         optional(jasonTls->expect(INVITE/100,from(proxy),1000,jasonTls->noAction())),
         jasonTls->expect(INVITE/407,from(proxy),1000,chain(jasonTls->ack(),jasonTls->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTls->expect(INVITE/100,from(proxy),1000,derekTcp->noAction()))
            ),
            Sub
            (
               derekTcp->expect(INVITE,contact(jasonTls),1000,chain(derekTcp->ring(),derekTcp->answer())),
               jasonTls->expect(INVITE/180,contact(derekTcp),1000,jasonTls->noAction()),
               jasonTls->expect(INVITE/200,contact(derekTcp),1000,jasonTls->ack()),
               derekTcp->expect(ACK,from(proxy),1000,chain(jasonTls->pause(1000),jasonTls->bye())),
               derekTcp->expect(BYE,from(proxy),2000,derekTcp->ok()),
               jasonTls->expect(BYE/200,contact(derekTcp),1000,jasonTls->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derekTcp->registerUser(0, all ),
          derekTcp->expect(REGISTER/401,from(proxy),WaitForResponse,derekTcp->digestRespond()),
          derekTcp->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTLSToTCPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToTCPCalleeHangsUp!*");
      Seq
      (
         derekTcp->registerUser(60, derekTcp->getDefaultContacts()),
         derekTcp->expect(REGISTER/401,from(proxy),1000,derekTcp->digestRespond()),
         derekTcp->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTcp->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->invite(*derekTcp),
         optional(jasonTls->expect(INVITE/100,from(proxy),1000,jasonTls->noAction())),
         jasonTls->expect(INVITE/407,from(proxy),1000,chain(jasonTls->ack(),jasonTls->digestRespond())),
         And
         (
            Sub
            (
               optional(jasonTls->expect(INVITE/100,from(proxy),1000,derekTcp->noAction()))
            ),
            Sub
            (
               derekTcp->expect(INVITE,contact(jasonTls),1000,chain(derekTcp->ring(),derekTcp->answer())),
               jasonTls->expect(INVITE/180,contact(derekTcp),1000,jasonTls->noAction()),
               jasonTls->expect(INVITE/200,contact(derekTcp),1000,jasonTls->ack()),
               derekTcp->expect(ACK,from(proxy),1000,chain(derekTcp->pause(1000),derekTcp->bye())),
               jasonTls->expect(BYE,from(proxy),2000,jasonTls->ok()),
               derekTcp->expect(BYE/200,contact(jasonTls),1000,derekTcp->noAction())
            )
         ),
         WaitForEndOfSeq
         
      );
      
      ExecuteSequences();
      

      Seq(derekTcp->registerUser(0, all ),
          derekTcp->expect(REGISTER/401,from(proxy),WaitForResponse,derekTcp->digestRespond()),
          derekTcp->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteTLSToTCP()
   {
      WarningLog(<<"*!testInviteUDPToTLS!*");
      
   }
   
   
   void testInviteRecursiveRedirect()
   {
      WarningLog(<<"*!testInviteRecursiveRedirect!*");
      
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(),jason->digestRespond())),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         derek->expect(INVITE,contact(jason),WaitForCommand,derek->send300(cullen->getDefaultContacts())),
         And
         (
            Sub
            (
               derek->expect(ACK,from(proxy),WaitForCommand,derek->noAction())
            ),
            Sub
            (
               cullen->expect(INVITE,contact(jason),WaitForCommand,chain(cullen->ring(),cullen->answer())),
               jason->expect(INVITE/180,contact(cullen),WaitForResponse,jason->noAction()),
               jason->expect(INVITE/200,contact(cullen),WaitForResponse,jason->ack()),
               cullen->expect(ACK,contact(jason),WaitForCommand,cullen->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

//*************Rainy Day***************//


   void testInvite407Dropped()
   {
      WarningLog(<<"*!testInvite407Dropped!*");

      boost::shared_ptr<SipMessage> msg;

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(save(msg,jason->invite(*derek)),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, jason->noAction()),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
              Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                  jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
                  jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                  derek->expect(ACK, from(jason), WaitForResponse, jason->noAction()))),
          WaitForEndOfTest);
      ExecuteSequences();  

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteAck407Dropped()
   {
      WarningLog(<<"*!testInviteAck407Dropped!*");
      
      
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,chain(derek->ring(),derek->answer())),
               jason->expect(INVITE/180,contact(derek),WaitForResponse,jason->noAction()),
               jason->expect(INVITE/200,contact(derek),WaitForResponse,jason->ack()),
               derek->expect(ACK,contact(jason),WaitForCommand,derek->noAction())
            ),
            Sub
            (
               jason->expect(INVITE/407,from(proxy),WaitForResponse,jason->ack())
            )
         ),
         
         WaitForEndOfTest
         
      );
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   
   void testInviteClientRetransmissionsWithRecovery()
   {
      WarningLog(<<"*!testInviteClientRetransmissionsWithRecovery!*");
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteClientLateAck()
   {
      WarningLog(<<"*!testInviteClientLateAck!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInvite1xxDropped()
   {
      WarningLog(<<"*!testInvite1xxDropped!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> inv;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), inv <= derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ring()),
               derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->retransmit(inv)),
               derek->expect(INVITE/180,contact(jason),WaitForResponse,chain(derek->noAction(),jason->answer())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteClientRetransmitsAfter200()
   {
      WarningLog(<<"*!testInviteClientRetransmitsAfter200!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteClientMissedAck()
   {
      WarningLog(<<"*!testInviteClientMissedAck!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testTimerC()
   {
      WarningLog(<<"*!testTimerC!*");
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      Seq(derek->registerUser(600, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          1000);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse,chain(jason->ack(),jason->digestRespond())),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100, from(proxy),WaitFor100,jason->noAction())),
               jason->expect(INVITE/180,contact(derek),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForResponse,derek->ring()),
               derek->expect(CANCEL,from(proxy),200000,chain(derek->ok(),derek->send487()))
            )
         ),
         And
         (
            Sub
            (
               derek->expect(ACK, from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               jason->expect(INVITE/487,from(proxy),WaitForResponse,jason->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
      

   void testInviteServerSpams200()
   {
      WarningLog(<<"*!testInviteServerSpams200!*");
      
      boost::shared_ptr<SipMessage> ok;

      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      
      ExecuteSequences();
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub//Soak up a 100 at some point
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub//The meat
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(), ok <= jason->answer())),
               derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(derek->ack(),jason->pause(25),jason->retransmit(ok),jason->retransmit(ok),jason->retransmit(ok))),
               And
               (
                  Sub//Soak up the 200s whenever they come in
                  (
                     derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
                     derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
                     derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
                  ),
                  Sub//Soak up the acks whenever they come in
                  (
                     jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
                     jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
                     jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
                     jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())                  
                  )
               )
            )
            
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteServerSends180After200()
   {
      WarningLog(<<"*!testInviteServerSends180After200!*");
      

      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      
      ExecuteSequences();

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer(),jason->ring())),
               derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   
   void testInviteClientSpamsInvite()
   {
      WarningLog(<<"*!testInviteClientSpamsInvite!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      
      ExecuteSequences();

      boost::shared_ptr<SipMessage> inv;
      
      Seq
      (
         chain(save(inv,derek->invite(*jason)),
         derek->retransmit(inv),
         derek->retransmit(inv),
         derek->retransmit(inv)),
         And
         (
            Sub //Soak up 100s
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
               
               And
               (
                  Sub //Soak up 407s whenever they happen to come in
                  (
                     derek->expect(INVITE/407,from(proxy),WaitForResponse,derek->noAction()),
                     derek->expect(INVITE/407,from(proxy),WaitForResponse,derek->noAction()),
                     derek->expect(INVITE/407,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub //The meat of the call
                  (
                     jason->expect(INVITE,contact(jason),WaitForCommand,chain(jason->ring(),jason->answer())),
                     derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
                     derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
                     jason->expect(ACK,contact(jason),WaitForCommand,jason->noAction())
                  ),
                  Sub //soak up 100 if it comes in at some point
                  (
                     optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
                  )
               )
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteClientSpamsAck407()
   {
      WarningLog(<<"*!testInviteClientSpamsAck407!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      
      ExecuteSequences();

      boost::shared_ptr<SipMessage> ack;
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(ack <= derek->ack(), derek->retransmit(ack),derek->retransmit(ack),derek->retransmit(ack),derek->digestRespond() )),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())),
               derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteClientSpamsAck200()
   {
      WarningLog(<<"*!testInviteClientSpamsAck200!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      
      ExecuteSequences();

      boost::shared_ptr<SipMessage> ack;
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())),
         derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack <= derek->ack(), derek->retransmit(ack),derek->retransmit(ack),derek->retransmit(ack))),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteCallerCancelsNo487()
   {
      WarningLog(<<"*!testInviteCallerCancelsNo487!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteServerRetransmits486()
   {
      WarningLog(<<"*!testInviteServerRetransmits486!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> busy;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         
         And
         (
            Sub //Server side
            (
               jason->expect(INVITE,from(proxy),WaitForCommand,busy <= jason->send486()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->retransmit(busy)),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub //Client side
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/486,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteServerRetransmits503()
   {
      WarningLog(<<"*!testInviteServerRetransmits503!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> error;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),

         And
         (
            Sub //Server side
            (
               jason->expect(INVITE,from(proxy),WaitForCommand,error <= jason->send503()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->retransmit(error)),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub //Client side
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/480,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteServerRetransmits603()
   {
      WarningLog(<<"*!testInviteServerRetransmits603!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> error;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub //Server side
            (
               jason->expect(INVITE,from(proxy),WaitForCommand,error <= jason->send603()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->retransmit(error)),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub //Client side
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/603,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteNoDNS()
   {
      WarningLog(<<"*!testInviteNoDNS!*");

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(jason->registerUser(60,resip::NameAddr("sip:foobar@liueawbhrviu")),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();
      
      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/480, from(proxy), 5000, derek->ack()),
          WaitForEndOfTest);
      ExecuteSequences();  
      

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
   }

   void testInviteNoDNSTcp()
   {
      WarningLog(<<"*!testInviteNoDNSTcp!*");

      Seq(jason->registerUser(60,resip::NameAddr("sip:foobar@liueawbhrviu")),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();
      
      
      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/480, from(proxy), 5000, derek->ack()),
          WaitForEndOfTest);
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteTransportFailure()
   {
      WarningLog(<<"*!testInviteTransportFailure!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteClientDiesAfterFirstInvite()
   {
      WarningLog(<<"*!testInviteClientDiesAfterFirstInvite!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),600,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),1100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),2100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         derek->expect(INVITE/407,from(proxy),4100,derek->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteClientDiesAfterSecondInvite()
   {
      WarningLog(<<"*!testInviteClientDiesAfterSecondInvite!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      boost::shared_ptr<SipMessage> ok;
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,from(proxy),WaitForCommand,chain(jason->ring(),ok<=jason->answer())),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(derek->noAction(),jason->pause(500),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(1000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(2000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),
               derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok)))
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteServerDead()
   {
      WarningLog(<<"testInviteServerDead");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
            
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),600,jason->noAction()), // <- original
               jason->expect(INVITE,contact(derek),600,jason->noAction()), // <- first retransmission
               jason->expect(INVITE,contact(derek),1100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),16100,jason->noAction())), // <- might get sent, depending on timing
               derek->expect(INVITE/408,from(proxy),21000,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteLoop()
   {
      WarningLog(<<"*!testInviteLoop!*");

      Seq
      (
         jason->registerUser(60,proxy->makeUrl("jason")),
         jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*jason),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(),david->digestRespond())),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/483,from(proxy),2000,david->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
   }
   
   void testInviteForgedUserInFrom()
   {
      WarningLog(<<"*!testInviteForgedUserInFrom!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
            
      Seq
      (
         condition(unknownUserInFrom,derek->invite(*jason)),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),condition(unknownUserInFrom,derek->digestRespond()))),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/403,from(proxy),WaitForResponse,derek->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteForgedHostInFrom()
   {
      WarningLog(<<"*!testInviteForgedHostInFrom!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
            
      Seq
      (
         condition(unknownHostInFrom,derek->invite(*jason)),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),condition(unknownHostInFrom,derek->digestRespond()))),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/403,from(proxy),WaitForResponse,derek->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteLoopingRedirect()
   {
      WarningLog(<<"*!testInviteLoopingRedirect!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
            
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,from(proxy),WaitFor100,jason->send300(jason->getDefaultContacts())),
         And
         (
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE/480,from(proxy),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
//*******************Forking INVITES, parallel******************//


//*************Sunny Day***************//       
   void testInviteForkOneAnswers()
   {
      WarningLog(<<"*!testInviteForkOneAnswers!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))
            ),
            Sub
            (
               jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->ring(), jason2->pause(PauseTime), jason2->answer())),
               derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction()),
               derek->expect(INVITE/200, contact(jason2), WaitForPause, derek->ack()),
               jason2->expect(ACK, from(derek), WaitForResponse, jason2->noAction())
            ),
            Sub
            (
               jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
               derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()),
               jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487())),
               jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteForkRedirect()
   {
      WarningLog(<<"*!testInviteForkRedirect!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      set<NameAddr> contacts=jason1->getDefaultContacts();
      set<NameAddr>::const_iterator i;
      for(i=jason2->getDefaultContacts().begin();i!=jason2->getDefaultContacts().end();i++)
      {
         contacts.insert(*i);
      }
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/302,from(proxy),WaitForCommand,chain(new CheckContacts(contacts,60),derek->ack())),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteForkOneBusy()
   {
      WarningLog(<<"*!testInviteForkOneBusy!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
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
                          jason2->expect(ACK, from(derek), WaitForResponse, jason2->noAction())),
                      Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->send486()),
                          jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction()))))),
          WaitForEndOfTest);
      
      ExecuteSequences();  
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteAllBusyContacts()
   {
      WarningLog(<<"*!testInviteAllBusyContacts!*");

      Seq(derek->registerUser(60,mergeContacts(*david,*cullen,*enlai)),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),

          And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
              Sub(david->expect(INVITE, contact(jason), WaitForCommand, chain(david->send100(),david->send503())),
                  david->expect(ACK,from(proxy),WaitForCommand,david->noAction())),
              Sub(cullen->expect(INVITE, contact(jason), WaitForCommand, cullen->send503()),
                  cullen->expect(ACK,from(proxy),WaitForCommand,cullen->noAction())),
              Sub(enlai->expect(INVITE, contact(jason), WaitForCommand, enlai->send503()),
                  enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())),
               Sub(jason->expect(INVITE/480, from(proxy), 3*WaitForCommand, jason->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteForkThreeCallerCancels()
   {
      WarningLog(<<"*!testInviteForkThreeCallerCancels!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason3->registerUser(60, jason3->getDefaultContacts()),
          jason3->expect(REGISTER/401, from(proxy), WaitForResponse, jason3->digestRespond()),
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
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteForkCallerHangsUp()
   {
      WarningLog(<<"*!testInviteForkCallerHangsUp!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))
            ),
            Sub
            (
               jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->ring(), jason2->pause(PauseTime), jason2->answer())),
               derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction()),
               derek->expect(INVITE/200, contact(jason2), WaitForPause, derek->ack()),
               jason2->expect(ACK, from(derek), WaitForResponse, chain(jason2->noAction(),derek->pause(1000),derek->bye())),
               jason2->expect(BYE,contact(derek),2000,jason2->ok()),
               derek->expect(BYE/200,contact(jason2),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
               derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()),
               jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487())),
               jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  
      
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteForkCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteForkCalleeHangsUp!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))
            ),
            Sub
            (
               jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->ring(), jason2->pause(PauseTime), jason2->answer())),
               derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction()),
               derek->expect(INVITE/200, contact(jason2), WaitForPause, derek->ack()),
               jason2->expect(ACK, from(derek), WaitForResponse, chain(jason2->pause(1000),jason2->bye())),
               derek->expect(BYE,contact(jason2),2000,derek->ok()),
               jason2->expect(BYE/200,contact(derek),WaitForResponse,jason2->noAction())
            ),
            Sub
            (
               jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
               derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()),
               jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487())),
               jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  
      
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }





//*************Cloudy Day***************//

   void testInviteForkThenSpiral()
   {
      WarningLog(<<"*!testInviteForkThenSpiral!*");
      
      Seq(derek->registerUser(60,derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();      

      Seq(cullen->registerUser(60, proxy->makeUrl("jason")),
          cullen->expect(REGISTER/401, from(proxy), WaitForResponse, cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForRegistration, cullen->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUser(60, proxy->makeUrl("enlai")),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(enlai->registerUser(60, proxy->makeUrl("david")),
          enlai->expect(REGISTER/401, from(proxy), WaitForResponse, enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForRegistration, enlai->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(david->registerUser(60, david->getDefaultContacts()),
          david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();


      
      set<NameAddr> contacts;
      contacts.insert(proxy->makeUrl("derek"));
      contacts.insert(proxy->makeUrl("cullen"));
      
      
      Seq
      (
         robert->registerUser(60,contacts),
         robert->expect(REGISTER/401, from(proxy),WaitForResponse,robert->digestRespond()),
         robert->expect(REGISTER/200,from(proxy),WaitForResponse,robert->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         adam->invite(*robert),
         optional(adam->expect(INVITE/100,from(proxy),WaitFor100,adam->noAction())),
         adam->expect(INVITE/407,from(proxy),WaitForResponse,chain(adam->ack(),adam->digestRespond())),
         
         And
         (
            Sub
            (
               optional(adam->expect(INVITE/100,from(proxy),WaitFor100,adam->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,contact(adam),WaitForCommand,chain(derek->ring(),derek->pause(1000),derek->answer())),
               adam->expect(INVITE/180,contact(derek),WaitForResponse,adam->noAction()),
               adam->expect(INVITE/200,contact(derek),WaitForResponse+1000,adam->ack()),
               derek->expect(ACK,contact(adam),WaitForCommand,derek->noAction())
            ),
            Sub
            (
               david->expect(INVITE,contact(adam),WaitForCommand,david->ring()),
               adam->expect(INVITE/180,contact(david),WaitForResponse,adam->noAction()),
               david->expect(CANCEL,from(proxy),3000,chain(david->ok(), david->send487())),
               david->expect(ACK,from(proxy),WaitForResponse,david->noAction())
            )
         ),
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();

      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(david->registerUser(0, all ),
          david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(cullen->registerUser(0, all ),
          cullen->expect(REGISTER/401,from(proxy),WaitForResponse,cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(robert->registerUser(0, all ),
          robert->expect(REGISTER/401,from(proxy),WaitForResponse,robert->digestRespond()),
          robert->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteSpiralThenFork()
   {
      WarningLog(<<"*!testInviteSpiralThenFork!*");
      
      Seq(derek->registerUser(60,proxy->makeUrl("cullen")),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();      

      Seq(cullen->registerUser(60, proxy->makeUrl("jason")),
          cullen->expect(REGISTER/401, from(proxy), WaitForResponse, cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForRegistration, cullen->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUser(60, proxy->makeUrl("enlai")),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(enlai->registerUser(60, proxy->makeUrl("david")),
          enlai->expect(REGISTER/401, from(proxy), WaitForResponse, enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForRegistration, enlai->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();      

      set<NameAddr> contacts = mergeContacts(*robert,*adam,*ben);
      
      Seq(david->registerUser(60, contacts),
          david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq
      (
         ajay->invite(*derek),
         optional(ajay->expect(INVITE/100,from(proxy),WaitFor100,ajay->noAction())),
         ajay->expect(INVITE/407,from(proxy),WaitForResponse,chain(ajay->ack(),ajay->digestRespond())),
         
         And
         (
            Sub
            (
               optional(ajay->expect(INVITE/100,from(proxy),WaitFor100,ajay->noAction()))
            ),
            Sub
            (
               robert->expect(INVITE,contact(ajay),WaitForCommand,robert->ring()),
               ajay->expect(INVITE/180,contact(robert),WaitForResponse,ajay->noAction()),
               robert->expect(CANCEL,from(proxy),3000,chain(robert->ok(), robert->send487())),
               robert->expect(ACK,from(proxy),WaitForResponse,robert->noAction())
            ),
            Sub
            (
               adam->expect(INVITE,contact(ajay),WaitForCommand,adam->ring()),
               ajay->expect(INVITE/180,contact(adam),WaitForResponse,ajay->noAction()),
               adam->expect(CANCEL,from(proxy),3000,chain(adam->ok(), adam->send487())),
               adam->expect(ACK,from(proxy),WaitForResponse,adam->noAction())
            ),
            Sub
            (
               ben->expect(INVITE,contact(ajay),WaitForCommand,chain(ben->ring(),ben->pause(1000),ben->answer())),
               ajay->expect(INVITE/180,contact(ben),WaitForResponse,ajay->noAction()),
               ajay->expect(INVITE/200,contact(ben),WaitForResponse+1000,ajay->ack()),
               ben->expect(ACK,contact(ajay),WaitForCommand,ben->noAction())
            )
         ),
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(david->registerUser(0, all ),
          david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(cullen->registerUser(0, all ),
          cullen->expect(REGISTER/401,from(proxy),WaitForResponse,cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


   void testInviteForkAll4xxResponses()
   {
      WarningLog(<<"*!testInviteForkAll4xxResponses!*");

      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send403()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send404()),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send480()),
               And
               (
                  Sub
                  (
                     enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
                  ),
                  Sub
                  (
                     david->expect(INVITE/480,from(proxy),WaitForResponse,david->ack())
                  )
               )
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      Seq(jason->registerUser(0, all ),
          jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(david->registerUser(0, all ),
          david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(derek->registerUser(0, all ),
          derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(cullen->registerUser(0, all ),
          cullen->expect(REGISTER/401,from(proxy),WaitForResponse,cullen->digestRespond()),
          cullen->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();

   }
   
   
   void testInviteFork200And4xx()
   {
      WarningLog(<<"*!testInviteFork200And4xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send403()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send404()),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->pause(1000),enlai->answer())),
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse+1000,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testInviteForkAll5xxResponses()
   {
      WarningLog(<<"*!testInviteForkAll5xxResponses!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send500()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send503()),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send513()),
               And
               (
                  Sub
                  (
                     enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
                  ),
                  Sub
                  (
                     david->expect(INVITE/513,from(proxy),WaitForResponse,david->ack())
                  )
               )
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   
   void testInviteFork200And5xx()
   {
      WarningLog(<<"*!testInviteFork200And5xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send500()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send503()),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->pause(1000),enlai->answer())),
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse+1000,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteForkAll6xxResponses()
   {
      WarningLog(<<"*!testInviteForkAll6xxResponses!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,chain(jason->pause(200),jason->send600())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send603()),            
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     david->expect(INVITE/603,contact(derek),WaitForResponse,david->ack())
                  )
               )
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->pause(200),enlai->send604())),
               enlai->expect(ACK,from(proxy),WaitForResponse,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteFork6xxBeats200()
   {
      WarningLog(<<"*!testInviteFork6xxBeats200!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send600()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send603()),            
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->pause(200),enlai->answer())),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForResponse,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
      

   void testInviteFork200Beats6xx()
   {
      WarningLog(<<"*!testInviteFork200Beats6xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,chain(jason->pause(200),jason->send600())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,chain(derek->pause(200),derek->send603())),            
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->answer())),
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForResponse,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
      
   void testInviteFork4xxAnd5xx()
   {
      WarningLog(<<"*!testInviteFork4xxAnd5xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
               david->expect(INVITE/403,from(proxy),WaitForResponse,david->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send503()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send403()),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send404()),
               enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testInviteFork4xx5xx6xx()
   {
      WarningLog(<<"*!testInviteFork4xx5xx6xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->send600()),
               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     david->expect(INVITE/600,contact(jason),WaitForResponse,david->ack())
                  )
               )
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send503()),            
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send404()),
               enlai->expect(ACK,from(proxy),WaitForResponse,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
         
   void testInviteForkAllAnswerNo1xx()
   {
      WarningLog(<<"*!testInviteForkAllAnswerNo1xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*enlai),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->answer()),
               david->expect(INVITE/200,contact(jason),WaitForResponse,david->ack()),
               jason->expect(ACK,contact(david),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->answer()),
               david->expect(INVITE/200,contact(derek),WaitForResponse,david->ack()),
               derek->expect(ACK,contact(david),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->answer()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
      Seq(enlai->registerUser(0, all ),
          enlai->expect(REGISTER/401,from(proxy),WaitForResponse,enlai->digestRespond()),
          enlai->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

      void testNonInviteClientRetransmissionsWithRecovery()
      {
         WarningLog(<<"*!testNonInviteClientRetransmissionsWithRecovery!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testNonInviteClientRetransmissionsWithTimeout()
      {
         WarningLog(<<"*!testNonInviteClientRetransmissionsWithTimeout!*");
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         boost::shared_ptr<SipMessage> infoMsg;

         Seq
         (
            derek->info(jason),
            derek->expect(INFO/407, from(proxy), 1000, infoMsg <= derek->digestRespond()),
            jason->expect(INFO, from(derek), 700, jason->noAction()),
            jason->expect(INFO, from(derek), 700, derek->retransmit(infoMsg)),
            jason->expect(INFO, from(derek), 1200, derek->retransmit(infoMsg)),
            jason->expect(INFO, from(derek), 2200, derek->retransmit(infoMsg)),
            And
            (
               Sub
               (
                  derek->expect(INFO/100, from(proxy), 500, derek->noAction())
               ),
               Sub
               (
                  jason->expect(INFO, from(derek), 4200, jason->noAction()),
                  jason->expect(INFO, from(derek), 4200, jason->noAction()),
                  jason->expect(INFO, from(derek), 4200, jason->noAction()),
                  jason->expect(INFO, from(derek), 4200, jason->noAction()),
                  jason->expect(INFO, from(derek), 4200, jason->noAction()),
                  jason->expect(INFO, from(derek), 4200, jason->noAction())
               )
            ),
            And
            (
               Sub
               (
                  derek->expect(INFO/480, from(proxy), WaitForResponse, derek->noAction())
               ),
               Sub
               (
                  optional(jason->expect(INFO, from(derek), 4200, jason->noAction())),
                  optional(jason->expect(INFO, from(derek), 4200, jason->noAction()))
               )
            ),
             32000);// Wait extra long for blacklist to expire
         ExecuteSequences();

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }
      
      void testNonInviteServerRetransmission()
      {
         WarningLog(<<"*!testNonInviteServerRetransmission!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }


      void testBasic302()
      {
         WarningLog(<<"*!testBasic302!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(david->registerUser(0, all ),
             david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testInviteNoAnswerCancel()
      {
         WarningLog(<<"*!testInviteNoAnswerCancel!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
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


      void testInviteClientMissedAck2()
      {
         WarningLog(<<"*!testInviteClientMissedAck!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
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
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         // This assumes that receive and send port are the same
         // otherwise, could just check for existence of rport parameter
         CheckRport checkRport(derek->getPort());
         Seq(condition(addRport, derek->invite(*jason)),
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->checkEchoName(checkRport)))),
                 Sub(jason->expect(INVITE, contact(derek), 1000, jason->ring()),
                     derek->expect(INVITE/180, from(jason), 1000, jason->answer()),
                     derek->expect(INVITE/200, contact(jason), 1000, derek->ack()),
                     jason->expect(ACK, from(derek), 1000, jason->noAction()))),
             WaitForEndOfSeq);
         
         ExecuteSequences();

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }
      
      void testAttendedExtensionToExtensionTransfer()
      {
         WarningLog(<<"*!testAttendedExtensionToExtensionTransfer!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
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
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

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
             optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
             derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

             And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                 Sub(david->expect(INVITE, contact(derek), WaitForCommand, david->answer()),
                     derek->expect(INVITE/200, contact(david), WaitForResponse, derek->ack()),
                     david->expect(ACK, from(derek), WaitForResponse, david->noAction()))),
             1000);
         
         ExecuteSequences();

         //102 refers 103 to 104

         Seq(derek->referReplaces(jason->getContact().uri(), david->getAddressOfRecord()),
               derek->expect(REFER/407,from(proxy),WaitForResponse,derek->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(david->registerUser(0, all ),
             david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testBlindTransferExtensionToExtensionHangupImmediately()
      {
         WarningLog(<<"*!testBlindTransferExtensionToExtensionHangupImmediately!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(david->registerUser(0, all ),
             david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }


      void testConferenceConferencorHangsUp()
      {
         WarningLog(<<"*!testConferenceConferencorHangsUp!*");

         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfTest);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/401, from(proxy), WaitForResponse, david->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(david->registerUser(0, all ),
             david->expect(REGISTER/401,from(proxy),WaitForResponse,david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }



      void testForkedInviteClientLateAck()
      {
         WarningLog(<<"*!testForkedInviteClientLateAck!*");

         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }



      void testInviteForkBothAnswerNoProvisional()
      {
         WarningLog(<<"*!testInviteForkBothAnswerNoProvisional!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
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
                     jason1->expect(ACK, contact(derek), 1000, jason1->noAction()))),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }
      
      void testInviteForkBothBusy()
      {
         WarningLog(<<"*!testInviteForkBothBusy!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }
                     

      void testInviteForkCallerCancels()
      {
         WarningLog(<<"*!testInviteForkCallerCancels!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfTest);
         Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfTest);
         Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
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
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
             1000);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testDigestInviteBasic()
      {
         WarningLog(<<"*!testDigestInviteBasic!*");

         // second time, it may have the credentials already
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
             1000);
         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
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

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }
      
      void test2Serial()
      {
         WarningLog(<<"*!test2Serial!*");

         Seq(jason->registerUser(61, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(jason->getDefaultContacts(), 61)),
             500);
         ExecuteSequences();

         Seq(derek->registerUser(62, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(derek->getDefaultContacts(), 62)),
             500);
         ExecuteSequences();


         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void test2Parallel()
      {
         WarningLog(<<"*!test2Parallel!*");

         Seq(jason->registerUser(63, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(jason->getDefaultContacts(), 63)),
             500);
         Seq(derek->registerUser(64, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(derek->getDefaultContacts(), 64)),
             500);

         ExecuteSequences();


         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testMultiContactSerial()
      {
         WarningLog(<<"*!testMultiContactSerial!*");

         Seq(jason1->registerUser(65, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/401, from(proxy), 1000, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), 3000, new CheckContacts(jason1->getDefaultContacts(), 65)),
             500);
         ExecuteSequences();         

         Seq(jason2->registerUser(66, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/401, from(proxy), 1000, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), 3000, new CheckFetchedContacts(mergeContacts(*jason2, *jason1))),
             500);
         ExecuteSequences();

         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
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
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 69)),
             500);
         
         ExecuteSequences();


         Seq(jason->registerUser(0, all ),
             jason->expect(REGISTER/401,from(proxy),WaitForResponse,jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }
      

      void testSetTwoRemoveOne()
      {
         WarningLog(<<"*!testSetTwoRemoveOne!*");
         
         Seq(derek->registerUser(80, mergeContacts(*jason, *derek)),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(mergeContacts(*jason, *derek), 80)),
             500);
         ExecuteSequences();
         
         NameAddr tmpCon = *jason->getDefaultContacts().begin();
         tmpCon.param(p_expires) = 0;
         set<NameAddr> contacts;
         contacts.insert(tmpCon);
         
            Seq(derek->registerUser(0, contacts),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(derek->getDefaultContacts())),
             500);
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testSimulSetAndRemove()
      {
         WarningLog(<<"*!testSimulSetAndRemove!*");

         Seq(derek->registerUser(81, mergeContacts(*jason, *derek)),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(mergeContacts(*jason, *derek), 81)),
             500);
         ExecuteSequences();

         set<NameAddr> contacts;
   
         NameAddr tmpCon = *jason->getDefaultContacts().begin();
         tmpCon.param(p_expires) = 0;
         contacts.insert(tmpCon);
         contacts.insert(*david->getDefaultContacts().begin());

         Seq(derek->registerUser(60, contacts),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(mergeContacts(*derek, *david))),
             500);
         ExecuteSequences();

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      //this doesn't work right now...removes have to be done specifically.
      //removes should be symmetric with refreshes
      void testSetThenRemoveByHeader()
      {
         WarningLog(<<"*!testSetThenRemoveByHeader!*");

         Seq(derek->registerUser(82, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 82)),
             500);
         ExecuteSequences();

         //CPPUNIT_ASSERT(LocationServer::Instance().exists(derek->getAddressOfRecordString()));

            Seq(derek->registerUser(0, derek->getDefaultContacts()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();
         
         //CPPUNIT_ASSERT(LocationServer::Instance().count(derek->getAddressOfRecordString()) == 0);

         Seq(derek->registerUser(0, all ),
             derek->expect(REGISTER/401,from(proxy),WaitForResponse,derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }

      void testSetThenRemoveAll()
      {
         WarningLog(<<"*!testSetThenRemoveAll!*");

         Seq(derek->registerUser(83, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         //CPPUNIT_ASSERT(LocationServer::Instance().count(derek->getAddressOfRecordString()) == 1);
         
            Seq(derek->removeRegistrationBindings(),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();
         
         sleepSeconds(1);
         //CPPUNIT_ASSERT(LocationServer::Instance().count(derek->getAddressOfRecordString()) == 0);
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
             jason->expect(REGISTER/401, from(proxy), 1000, jason->digestRespond()),
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
             jason->expect(REGISTER/401, from(proxy), 1000, condition(sseq1, jason->digestRespond())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         Seq(condition(compose(stripAuths, sseq, scallid), jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/401, from(proxy), 1000, condition(sseq1, jason->digestRespond())),
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
             jason->expect(REGISTER/401, from(proxy), 1000, condition(sseq1, jason->digestRespond())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);
         ExecuteSequences();

         // new callid, same sequence
         Seq(condition(compose(stripAuths, sseq, scallid), jason->registerUser(83, jason->getDefaultContacts())),
             jason->expect(REGISTER/401, from(proxy), 1000, condition(compose(sseq1, scallid), jason->digestRespond())),
             jason->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(jason->getDefaultContacts(), 83)),
             500);

         ExecuteSequences();
      }
      
      
      
       void testTimerCInterference()
      {
         WarningLog(<<"*!testTimerCInterference!*");
         Seq(jason->registerUser(600, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             1000);
         Seq(derek->registerUser(600, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             1000);
         ExecuteSequences();
         
         Seq
         (
            jason->invite(*derek),
            optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
            jason->expect(INVITE/407, from(proxy), WaitForResponse,chain(jason->ack(),jason->digestRespond())),

            optional(jason->expect(INVITE/100, from(proxy),WaitFor100,jason->noAction())),
            derek->expect(INVITE,contact(jason),WaitForResponse,chain(derek->ring(),derek->pause(50),derek->answer())),
            jason->expect(INVITE/180,contact(derek),WaitForResponse,jason->noAction()),
            jason->expect(INVITE/200,contact(derek),WaitForResponse,jason->ack()),
            derek->expect(ACK,from(proxy),WaitForResponse,chain(jason->pause(200000),jason->bye())),
            derek->expect(BYE,contact(jason),220000,derek->ok()),
            jason->expect(BYE/200,contact(derek),WaitForResponse,jason->noAction()),
            WaitForEndOfTest
         );
         ExecuteSequences();
      }
     
      
      void testTimerCForked()
      {
         WarningLog(<<"*!testTimerCForked!*");

         Seq(derek->registerUser(600, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         Seq(jason1->registerUser(600, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/401, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfSeq);
         Seq(jason2->registerUser(600, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/401, from(proxy), WaitForResponse, jason2->digestRespond()),
             jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         CountDown count487(2, "count487");         

         Seq
         (
            derek->invite(*jason),
            optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
            derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
            optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),

            And
            (
               Sub
               (
                  jason2->expect(INVITE, contact(derek), WaitForCommand, jason2->ring()),
                  derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction()),
                  jason2->expect(CANCEL,from(proxy),200000,chain(jason2->ok(), jason2->send487(),count487.dec())),
                  jason2->expect(ACK,from(proxy),WaitForResponse,jason2->noAction())
               ),
               Sub
               (
                  jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
                  derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()),
                  jason1->expect(CANCEL,from(proxy),200000,chain(jason1->ok(), jason1->send487(),count487.dec())),
		  jason1->expect(ACK,from(proxy),WaitForResponse,jason1->noAction())
               )
            ),

            derek->expect(INVITE/487,from(proxy),count487,WaitForResponse,derek->ack()),

            
            WaitForEndOfTest);
         
         ExecuteSequences();  
      }

      /* Test that the routing logic can make decisions based on the method
       * and (for SUBSCRIBEs) the event type.
       */
      void testRoutingBasic()
      {
         WarningLog(<<"*!testRoutingBasic!*");

	 // INVITEs to u@.* are routed to derek.
         RouteGuard dGuard1(*proxy, "sip:u@.*", "sip:derek@localhost",
                            "INVITE");
	 // SUBSCRIBEs for dialog u@.* are routed to david.
         RouteGuard dGuard2(*proxy, "sip:u@.*", "sip:david@localhost",
                            "SUBSCRIBE", "dialog");
	 // SUBSCRIBEs for status u@.* are routed to enlai.
         RouteGuard dGuard3(*proxy, "sip:u@.*", "sip:enlai@localhost",
                            "SUBSCRIBE", "status");
	 // Everything else is routed to jason.
	 // Give weight 2 for the route to give this route a lower priority
	 // than the ones above.
         RouteGuard dGuard4(*proxy, "", "sip:jason@localhost", Data::Empty,
			    Data::Empty, 2);

	 // Register the users.

         Seq(derek->registerUser(600, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse,
	                   derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           derek->noAction()),
	     WaitForEndOfSeq);

         Seq(david->registerUser(600, david->getDefaultContacts()),
             david->expect(REGISTER/401, from(proxy), WaitForResponse,
	                   david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           david->noAction()),
	     WaitForEndOfSeq);

         Seq(enlai->registerUser(600, enlai->getDefaultContacts()),
             enlai->expect(REGISTER/401, from(proxy), WaitForResponse,
	                   enlai->digestRespond()),
             enlai->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           enlai->noAction()),
	     WaitForEndOfSeq);

         Seq(jason->registerUser(600, jason->getDefaultContacts()),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse,
	                   jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           jason->noAction()),

             WaitForEndOfSeq);

         ExecuteSequences();  

	 // Send the requests.

	 // cullen sends INVITE to u@.* which should be routed to derek.
	 Seq(cullen->invite(proxy->makeUrl("u").uri()),
	     optional(cullen->expect(INVITE/100, from(proxy), WaitFor100,
				     cullen->noAction())),
             cullen->expect(INVITE/407, from(proxy), WaitForResponse,
			    chain(cullen->ack(),
				  cullen->digestRespond())),
	     And(Sub(optional(cullen->expect(INVITE/100, from(proxy), WaitFor100,
					     cullen->noAction()))),
		 Sub(derek->expect(INVITE, contact(cullen), WaitForCommand,
				   derek->answer()),
		     cullen->expect(INVITE/200, contact(derek), WaitForResponse,
				    cullen->ack()),
		     derek->expect(ACK, from(cullen), WaitForResponse,
				   cullen->noAction()))),
	     WaitForEndOfTest);

         ExecuteSequences();  
      }

      void testEarlyMedia()
      {
         InfoLog(<< "*!testEarlyMedia!*");

         Seq(derek->registerUser(60, derek->getDefaultContacts()),
             derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, derek->invite(*jason)),
             optional(derek->expect(INVITE/100, from(proxy), WaitForResponse, derek->noAction())),
             derek->expect(INVITE/407, from(proxy),  WaitForResponse, chain(derek->ack(), derek->digestRespond())),
             optional(derek->expect(INVITE/100, from(proxy), WaitForResponse, derek->noAction())),
             jason->expect(INVITE, from(proxy), WaitForResponse, jason->ring183()),
             derek->expect(INVITE/183, from(proxy), WaitForResponse, chain(derek->pause(5000), jason->answer())),
             derek->expect(INVITE/200, from(proxy), WaitForResponse+5000, derek->ack()),
             jason->expect(ACK, from(proxy), WaitForResponse, jason->bye(*derek)),
             derek->expect(BYE, from(proxy), WaitForResponse, derek->ok()),
             jason->expect(BYE/200, from(proxy), WaitForResponse, derek->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      // provisioning here(automatic cleanup)
      static void createStatic()
      {
      }

//*************MESSAGE scenarios************************************//

//*************Sunny Day***************//   
    
   void testMessageBasic()
   {
      WarningLog(<<"*!testMessageBasic!*");

      //Registering Derek
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/401, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
        //Declaring data variable type for use as text message
	Data text("Hi there");

      //Jason sends a MESSAGE to Derek
      Seq(jason->message(*derek, text),

          //Jason gets a 407 and responds appropriately
          jason->expect(MESSAGE/407, from(proxy), 1000, jason->digestRespond()),

	  //Derek receives a MESSAGE 
          derek->expect(MESSAGE, from(proxy), 1000, derek->ok()),
		  
	  //Jason gets a 200 from Derek
	  jason->expect(MESSAGE/200, contact(derek), 1000, jason->noAction()),
		   
      WaitForEndOfTest);
      ExecuteSequences();  
   }	  

};

#define TEST(_method) \
   suiteOfTests->addTest(new CppUnit::TestCaller<TestHolder>(#_method, &TestHolder::_method))

// TODO:  Need to categorize tests that are failing into two categories.
// Tests that fail because there are problems in the test case implementation
// should be labeled as BADTEST.  Tests that fail because they identify a
// known bug in rePro should be labeled as BUGTEST.
#define BADTEST(_method) \
   if(sRunTestsKnownToFail) \
   { \
      printf("running: " #_method " not known to pass, issues with test\n"); \
      TEST(_method); \
   } \
   else \
   { \
      printf("badtest: " #_method " not run\n"); \
   } 

#define BUGTEST(_method) \
   if(sRunTestsKnownToFail) \
   { \
      printf("running: " #_method " not known to pass, known bug\n"); \
      TEST(_method); \
   } \
   else \
   { \
      printf("bugtest: " #_method " not run\n"); \
   }


class MyTestCase
{
   public:
      static CppUnit::Test* suite(bool redirectServer)
      {
         CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "Suite1" );

#if 1 
         if(redirectServer)
         {
            TEST(testInviteBasicRedirect);
            TEST(testInviteForkRedirect);
         }
         else
         {
// MESSAGE tests
         TEST(testMessageBasic);
//Registrar tests

         TEST(testRegisterBasic);
         TEST(testMultiple1);
         TEST(testMixedExpires);
         TEST(testThirdPartyRegistration);
         TEST(testDetailsQValue); 
                   
         TEST(testDetailsExpires);
         TEST(testRegister407Dropped);
         TEST(testRegisterBogusAuth);
         //TEST(testRegisterLateDigestResponse); //Extremely long test (around 50 minutes).
         TEST(testRegisterClientRetransmits);
         //TEST(testRegisterNoUserInTo); //repro doesn't complain about this yet
         //TEST(testRegisterUserInReqUri); //repro doesn't complain about this yet
         //TEST(testRegisterUnknownAorHost); //repro doesn't complain about this yet
         //TEST(testRegisterUnknownAorUser); //repro doesn't complain about this yet
         //TEST(testOversizeCallIdRegister); //message is getting dropped. Why?
         //TEST(testOversizeContactRegister); //message is getting dropped. Why?
         TEST(testRefresh);
         TEST(testRefreshMulti);
         TEST(testRefreshThirdParty);
         TEST(testChangeQValue); 
         
         TEST(testSetThenRemoveSpecific);
         TEST(testUnregisterMulti);
         TEST(testUnregisterExpired);
         TEST(testUnregisterAll);
         TEST(testUnregisterAllBadExpires);
         //TEST(testUnregisterNonExistent);//repro doesn't complain about this yet
         TEST(testFetch);
         TEST(testExpiryCleanup);
         //TEST(testFetchNonExistent); //repro doesn't complain about this yet


//Proxy tests

         TEST(testInviteBasic);
         TEST(testInviteBasicTls);
         TEST(testInviteCallerHangsUp);
         TEST(testInviteCalleeHangsUp);
         TEST(testInviteCallerCancels);
         TEST(testInviteBusy);

         TEST(testSpiral);
         TEST(testSpiralWithCancel);

         TEST(testInviteCallerCancelsWithRace);
         TEST(testInvite200BeatsCancelClientSide);
         TEST(testInvite486BeatsCancelServerSide);
         TEST(testInvite486BeatsCancelClientSide);
         TEST(testInvite503BeatsCancelServerSide);
         TEST(testInviteNotFound);
         TEST(testInvite488Response);
         TEST(testInvite480Response);
         TEST(testInvite500Response);
         TEST(testInvite503Response);
         TEST(testInvite600Response);
         TEST(testInvite603Response);
         TEST(testInviteServerSpams180);
         TEST(testInviteBogusAuth);
         
         //Transport switches (double record-route tests)
         TEST(testInviteUDPToTCPCallerHangsUp);
         TEST(testInviteUDPToTCPCalleeHangsUp);
         TEST(testInviteTCPToUDPCallerHangsUp);
         TEST(testInviteTCPToUDPCalleeHangsUp);
         TEST(testInviteUDPToTLSCallerHangsUp);
         TEST(testInviteUDPToTLSCalleeHangsUp);
         TEST(testInviteTCPToTLSCallerHangsUp);
         TEST(testInviteTCPToTLSCalleeHangsUp);
         TEST(testInviteTLSToUDPCallerHangsUp);
         TEST(testInviteTLSToUDPCalleeHangsUp);
         TEST(testInviteTLSToTCPCallerHangsUp);
         TEST(testInviteTLSToTCPCalleeHangsUp);
         
         BUGTEST(testInviteRecursiveRedirect);
         TEST(testInvite407Dropped);
         //TEST(testInviteAck407Dropped); //tfm is bungling this one
         TEST(testInviteClientRetransmissionsWithRecovery);
         TEST(testInviteClientLateAck);
         TEST(testInvite1xxDropped);
         TEST(testInviteClientRetransmitsAfter200);
         TEST(testInviteClientMissedAck);
         TEST(testTimerC); //This test takes a long time
         TEST(testInviteServerSpams200);
         TEST(testInviteServerSends180After200);
         //TEST(testInviteClientSpamsInvite); //tfm is asserting on this test, will look into
         TEST(testInviteClientSpamsAck407);
         BADTEST(testInviteClientSpamsAck200); // Race in the test
         TEST(testInviteCallerCancelsNo487);
         TEST(testInviteServerRetransmits486);
         TEST(testInviteServerRetransmits503);
         TEST(testInviteServerRetransmits603);
         TEST(testInviteNoDNS);
         TEST(testInviteNoDNSTcp);
         //TEST(testInviteTransportFailure); //tfm asserts
         TEST(testInviteClientDiesAfterFirstInvite);
         TEST(testInviteClientDiesAfterSecondInvite);
         TEST(testInviteServerDead);

         BADTEST(testInviteLoop); 

         TEST(testInviteForgedUserInFrom);
         //TEST(testInviteForgedHostInFrom); 

         TEST(testInviteForkOneAnswers);
         TEST(testInviteForkOneBusy);
         TEST(testInviteAllBusyContacts);
         TEST(testInviteForkThreeCallerCancels);
         TEST(testInviteForkCallerHangsUp);
         TEST(testInviteForkCalleeHangsUp);

         TEST(testInviteForkThenSpiral);
         TEST(testInviteSpiralThenFork);

         TEST(testInviteForkAll4xxResponses);
         TEST(testInviteFork200And4xx);
         TEST(testInviteForkAll5xxResponses);
         TEST(testInviteFork200And5xx);
         TEST(testInviteForkAll6xxResponses);
         TEST(testInviteFork6xxBeats200);
         TEST(testInviteFork200Beats6xx);
         TEST(testInviteFork4xxAnd5xx);
         TEST(testInviteFork4xx5xx6xx);
         //TEST(testInviteForkAllAnswerNo1xx); //tfm is messing this one up
         
         TEST(testInviteClientRetransmitsAfter200);
         TEST(testNonInviteClientRetransmissionsWithRecovery);
         TEST(testNonInviteClientRetransmissionsWithTimeout);
         TEST(testNonInviteServerRetransmission);
         BADTEST(testBasic302);
         TEST(testInviteNoAnswerCancel);
         TEST(testInviteNotFoundServerRetransmits);
         TEST(testInviteClientMissedAck2);
         TEST(testInviteForRport);
         BADTEST(testInviteForkBothAnswerNoProvisional);
         BADTEST(testAttendedExtensionToExtensionTransfer); // reINVITEs are problematic
         BADTEST(testBlindTransferExtensionToExtensionHangupImmediately); // reINVITEs are problematic
         BADTEST(testConferenceConferencorHangsUp); // reINVITEs are problematic
         BADTEST(testForkedInviteClientLateAck);
         TEST(testInviteForkBothBusy);
         TEST(testEarlyMedia);

	 // Tests of the routing pattern matching logic.
         BADTEST(testRoutingBasic);

#else
         TEST(testInviteAllBusyContacts);
         
#endif   
         }
         return suiteOfTests;
      }
};

int main(int argc, char** argv)
{
#ifndef _WIN32
   if ( signal( SIGPIPE, SIG_IGN) == SIG_ERR)
   {
      cerr << "Couldn't install signal handler for SIGPIPE" << endl;
      exit(-1);
   }
#endif

   all.setAllContacts();
   initNetwork();
   try
   {
      CommandLineParser args(argc, argv);

      std::map<resip::Data, log4cplus::LogLevel> logLevelMap;
   
      logLevelMap[resip::Data("TRACE")] = log4cplus::TRACE_LOG_LEVEL;
      logLevelMap[resip::Data("DEBUG")] = log4cplus::DEBUG_LOG_LEVEL;
      logLevelMap[resip::Data("INFO")] = log4cplus::INFO_LOG_LEVEL;
      logLevelMap[resip::Data("WARN")] = log4cplus::WARN_LOG_LEVEL;
      logLevelMap[resip::Data("ERROR")] = log4cplus::ERROR_LOG_LEVEL;
      logLevelMap[resip::Data("FATAL")] = log4cplus::FATAL_LOG_LEVEL;
   
      log4cplus::LogLevel rootLevel;
      
      if(logLevelMap.count(args.mLogLevel))
      {
         rootLevel = logLevelMap[args.mLogLevel];
      }
      else
      {
         rootLevel = log4cplus::INFO_LOG_LEVEL;
      }
      //This ends up initializing two different logging systems.
      //One is a new logging system found in EsLogger.hxx
      //The other is the old resip logging system found in rutil/Logger.hxx (EsLogger 
      estacado::EsLogger::init("reprolog.conf",argc,argv,rootLevel);

      resip::Timer::T100 = 0;
      
      TestHolder::createStatic();
      ProxyFixture::initialize(argc, argv);
      
      CppUnit::TextUi::TestRunner runner;

      runner.addTest( MyTestCase::suite(args.mRedirectServer) );
      runner.run();
      DebugLog(<< "Finished");

      ProxyFixture::destroyStatic();
   }
   catch (BaseException& e)
   {
      cerr << "Fatal error: " << e << endl;
      exit(-1);
   }

   return 0;
}

/* Copyright 2007 Estacado Systems */

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
