#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

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
      inviteCSeq(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(resip::h_CSeq).method()=resip::INVITE;
         msg->header(resip::h_CSeq).unknownMethodName()="INVITE";         
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      cancelCSeq(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(resip::h_CSeq).method()=resip::CANCEL;
         msg->header(resip::h_CSeq).unknownMethodName()="CANCEL";         
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      ackCSeq(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(resip::h_CSeq).method()=resip::ACK;
         msg->header(resip::h_CSeq).unknownMethodName()="ACK";         
         return msg;
      }
      
      static boost::shared_ptr<SipMessage>
      unknownCSeq(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(resip::h_CSeq).method()=resip::UNKNOWN;
         msg->header(resip::h_CSeq).unknownMethodName()="blargagsaqq";         
         return msg;
      }
      
      

      static resip::Data
      doubleSend(const resip::Data& msg)
      {
         resip::Data result=msg+msg;
         return result;
      }

///***************************************** tests start here ********************************//

//*****************************Registrar tests********************************//

//***********************New Registration Cases******************//

//*********Sunny day cases**********//



   void testRegisterBasic()
      {
         WarningLog(<<"*!testRegisterBasic!*");
         
         Seq(jason->registerUser(60, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
             WaitForEndOfTest);
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

   void testThirdPartyRegistration()
   {
      WarningLog(<<"*!testThirdPartyRegistration!*");
      set<NameAddr> contacts;
      contacts.insert(*(derek->getDefaultContacts().begin()));
      Seq
      (
         jason->registerUser(3000,contacts),
         jason->expect(REGISTER/407, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckContacts(contacts,3000)),
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

//*********Rainy day cases**********//

   void testRegister407Dropped()
   {
      WarningLog(<<"*!testRegister407Dropped!*");
      
      boost::shared_ptr<SipMessage> reg;
      Seq
      (
         save(reg,jason->registerUser(60,jason->getDefaultContacts())),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->retransmit(reg)),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   
   void testRegisterBogusAuth()
   {
      WarningLog(<<"*!testRegisterBogusAuth!*");
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, condition(bogusAuth,jason->digestRespond())),
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
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, chain(jason->pause(3000000),jason->digestRespond())),
         jason->expect(REGISTER/407, from(proxy), 3000000, jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60) ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
      
   }
   
      void testRegisterClientRetransmits()
      {
         WarningLog(<<"*!testRegisterClientRetransmits!*");

         boost::shared_ptr<SipMessage> reg;
         Seq(save(reg, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->retransmit(reg)),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
             WaitForEndOfTest);
         ExecuteSequences();
      }

   void testRegisterNoUserInTo()
   {
      WarningLog(<<"*!testRegisterNoUserInTo!*");
      
      
      Seq
      (
         jason->registerUser(60,jason->getDefaultContacts()), //There would be a condition noUserInTo here, but that would corrupt the registration message that gets reused in every test. (This is broken behavior on tfm's part)
         jason->expect(REGISTER/407,from(proxy), WaitForResponse, condition(noUserInTo,jason->digestRespond())),
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
         jason->expect(REGISTER/407,from(proxy), WaitForResponse, condition(userInReqUri,jason->digestRespond())),
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
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownHostInTo,jason->digestRespond())),
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
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
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
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
            optional(derek->expect(REGISTER/407,from(proxy),WaitForResponse, derek->digestRespond())),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(contacts, 72)),
             500);
         
         ExecuteSequences();
      }

   void testRefreshMulti()
   {
      WarningLog(<<"*!testRefreshMulti!*");
   
      set<NameAddr> contacts = mergeContacts(*jason, *derek);

      Seq
      (
         jason->registerUser(60,contacts),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(contacts,60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      sleepSeconds(1);
      
      Seq
      (
         jason->registerUser(120,contacts),
         optional(jason->expect(REGISTER/407,from(proxy),WaitForResponse, jason->digestRespond())),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse, new CheckContacts(contacts,120)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   
   }

   void testRefreshThirdParty()
   {
      WarningLog(<<"*!testRefreshThirdParty!*");
      
      Seq
      (
         jason->registerUser(60,derek->getDefaultContacts()),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jason->registerUser(120,derek->getDefaultContacts()),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),120)),
         WaitForEndOfSeq
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
         
         Seq(jason->registerUser(73, contactsBefore),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsBefore, 73)),
             500);
         ExecuteSequences();

         Seq(jason->registerUser(74, contactsAfter),
             jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsAfter, 74)),
             500);
         ExecuteSequences();
      }
                               

//*********Rainy day cases**********//

       
       
//*******************Unregister type tests*******************/

//*********Sunny day cases**********//

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
             derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
             500);
         ExecuteSequences();
      }

   void testUnregisterMulti()
   {
      WarningLog(<<"*!testUnregisterMulti!*");
      
      set<NameAddr> contacts = mergeContacts(*jason, *derek);
      set<NameAddr> emptySet;
      
      Seq(jason->registerUser(70, contacts),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 70)),
          500);

      ExecuteSequences();

      Seq
      (
         jason->registerUser(0,contacts),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(emptySet,0)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   
   }

   void testUnregisterExpired()
   {
      WarningLog(<<"*!testUnregisterExpired!*");
      
      set<NameAddr> emptySet;
      Seq(derek->registerUser(3, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      sleepSeconds(5);
      
      
      Seq
      (
         derek->registerUser(0, derek->getDefaultContacts()),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), 4*Seconds + WaitForResponse, new CheckContacts(emptySet, 0)),
         WaitForEndOfTest
      );
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
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason1->getDefaultContacts(),60 )),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts( mergeContacts(*jason1, *jason2) )),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason1->registerUser(0, all ),
          jason1->expect(REGISTER/407,from(proxy),WaitForResponse,jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts(emptySet)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   

//*********Rainy day cases**********//

   void testUnregisterAllBadExpires()
   {
      WarningLog(<<"*!testUnregisterAllBadExpires!*");
      
      NameAddr na;
      set<NameAddr> all;
      set<NameAddr> emptySet;
      
      na.setAllContacts();
      all.insert( na );
      
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason1->getDefaultContacts(),60)),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts( mergeContacts(*jason1, *jason2) )),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason1->registerUser(10, all ),
          jason1->expect(REGISTER/407,from(proxy),WaitForResponse,jason1->digestRespond()),
          jason1->expect(REGISTER/400, from(proxy), WaitForResponse, jason1->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testUnregisterNonExistent()
   {
      WarningLog(<<"*!testUnregisterNonExistent!*");
      
      
      Seq
      (
         condition(unknownUserInTo,jason->registerUser(0,jason->getDefaultContacts())),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
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
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 75)),
          500);
      ExecuteSequences();
      
      set<NameAddr> nullSet;
      Seq(jason->registerUser(76, nullSet),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(contacts)),
          500);
      ExecuteSequences();
   }

   void testExpiryCleanup()
   {
      WarningLog(<<"*!testExpiryCleanup!*");

      Seq(derek->registerUser(10, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 60)),
          500);

      ExecuteSequences();
      sleepSeconds(12);
      // !jf! cause registration bindings to expire here
      
      set<NameAddr> emptySet;
      Seq(derek->registerUser(78, emptySet),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 78)),
          500);
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
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
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

   void testInviteCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteCalleeHangsUp!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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


//*************Cloudy Day***************//

   void testSpiral()
   {
      WarningLog(<<"*!testSpiral!*");
      RouteGuard dGuard(*proxy, "sip:spiral@.*", "sip:"+david->getAddressOfRecordString());
      RouteGuard dGuard1(*proxy, "sip:1spiral@.*", "sip:spiral@localhost");
      RouteGuard dGuard2(*proxy, "sip:2spiral@.*", "sip:1spiral@localhost");
      RouteGuard dGuard3(*proxy, "sip:3spiral@.*", "sip:2spiral@localhost");
      RouteGuard dGuard4(*proxy, "sip:4spiral@.*", "sip:3spiral@localhost");
      RouteGuard dGuard5(*proxy, "sip:5spiral@.*", "sip:4spiral@localhost");
      RouteGuard dGuard6(*proxy, "sip:6spiral@.*", "sip:5spiral@localhost");
      RouteGuard dGuard7(*proxy, "sip:7spiral@.*", "sip:6spiral@localhost");
      RouteGuard dGuard8(*proxy, "sip:8spiral@.*", "sip:7spiral@localhost");
      RouteGuard dGuard9(*proxy, "sip:9spiral@.*", "sip:8spiral@localhost");
      

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


   void testSpiralWithCancel()
   {
      WarningLog(<<"*!testSpiralWithCancel!*");
      RouteGuard dGuard(*proxy, "sip:spiral@.*", "sip:"+david->getAddressOfRecordString());
      RouteGuard dGuard1(*proxy, "sip:1spiral@.*", "sip:spiral@localhost");
      RouteGuard dGuard2(*proxy, "sip:2spiral@.*", "sip:1spiral@localhost");
      RouteGuard dGuard3(*proxy, "sip:3spiral@.*", "sip:2spiral@localhost");
      RouteGuard dGuard4(*proxy, "sip:4spiral@.*", "sip:3spiral@localhost");
      RouteGuard dGuard5(*proxy, "sip:5spiral@.*", "sip:4spiral@localhost");
      RouteGuard dGuard6(*proxy, "sip:6spiral@.*", "sip:5spiral@localhost");
      RouteGuard dGuard7(*proxy, "sip:7spiral@.*", "sip:6spiral@localhost");
      RouteGuard dGuard8(*proxy, "sip:8spiral@.*", "sip:7spiral@localhost");
      RouteGuard dGuard9(*proxy, "sip:9spiral@.*", "sip:8spiral@localhost");

      Seq(david->registerUser(60, david->getDefaultContacts()),
          david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq
      (
         derek->invite(proxy->makeUrl("9spiral").uri()),
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

   void testInvite200BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite200BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }

   void testInvite486BeatsCancelServerSide()
   {
      WarningLog(<<"*!testInvite486BeatsCancelServerSide!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }


   void testInvite486BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite486BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }



   void testInvite503BeatsCancelServerSide()
   {
      WarningLog(<<"*!testInvite503BeatsCancelServerSide!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }


   void testInvite503BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite503BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
              Sub(jason->expect(INVITE, contact(derek), WaitForCommand, chain(jason->send100(),jason->send488())),
                  jason->expect(ACK, from(proxy), WaitForAck, jason->noAction()),
                  derek->expect(INVITE/488, from(proxy), WaitForResponse, derek->ack()))),
          WaitForEndOfTest);
      
      ExecuteSequences();  
   }
   
   void testInvite480Response()
   {
      WarningLog(<<"*!testInvite480Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }
   
   void testInvite500Response()
   {
      WarningLog(<<"*!testInvite500Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }
   
   void testInvite503Response()
   {
      WarningLog(<<"*!testInvite503Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }
   

   void testInvite600Response()
   {
      WarningLog(<<"*!testInvite600Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }

   void testInvite603Response()
   {
      WarningLog(<<"*!testInvite603Response!*");
   
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }
   
   void testInviteServerSpams180()
   {
      WarningLog(<<"*!testInviteServerSpams180!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();


      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
         derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitForResponse,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->ring(),jason->ring(),jason->answer())),
         derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/200,from(proxy),WaitForResponse,derek->ack()),
         jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteBogusAuth()
   {
      WarningLog(<<"*!testInviteBogusAuth!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }
   
   void testInviteRecursiveRedirect()
   {
      WarningLog(<<"*!testInviteRecursiveRedirect!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }

//*************Rainy Day***************//


   void testInvite407Dropped()
   {
      WarningLog(<<"*!testInvite407Dropped!*");

      boost::shared_ptr<SipMessage> msg;

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   }


   void testInviteAck407Dropped()
   {
      WarningLog(<<"*!testInviteAck407Dropped!*");
      
      
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
   
   void testInvite1xxDropped()
   {
      WarningLog(<<"*!testInvite1xxDropped!*");

      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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

   void testTimerC()
   {
      WarningLog(<<"*!testTimerC!*");
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      Seq(derek->registerUser(600, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          1000);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse,chain(jason->ack(),jason->digestRespond())),

         optional(jason->expect(INVITE/100, from(proxy),WaitFor100,jason->noAction())),
         derek->expect(INVITE,contact(jason),WaitForResponse,derek->ring()),
         jason->expect(INVITE/180,contact(derek),WaitForResponse,jason->noAction()),
         derek->expect(CANCEL,from(proxy),200000,chain(derek->ok(),derek->send487())),
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
   }
      

   void testInviteServerSpams200()
   {
      WarningLog(<<"*!testInviteServerSpams200!*");
      
      boost::shared_ptr<SipMessage> ok;

      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }


   void testInviteServerSends180After200()
   {
      WarningLog(<<"*!testInviteServerSends180After200!*");
      

      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   
   }
   
   
   void testInviteClientSpamsInvite()
   {
      WarningLog(<<"*!testInviteClientSpamsInvite!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
      
   }
   
   void testInviteClientSpamsAck407()
   {
      WarningLog(<<"*!testInviteClientSpamsAck407!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      
      ExecuteSequences();

      boost::shared_ptr<SipMessage> ack;
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(ack <= derek->ack(), derek->retransmit(ack),derek->retransmit(ack),derek->retransmit(ack),derek->digestRespond() )),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())),
         derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }


   void testInviteClientSpamsAck200()
   {
      WarningLog(<<"*!testInviteClientSpamsAck200!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }

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

   void testInviteServerRetransmits486()
   {
      WarningLog(<<"*!testInviteServerRetransmits486!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> busy;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,from(proxy),WaitForCommand,busy <= jason->send486()),
         
         And
         (
            Sub //Server side
            (
               jason->expect(ACK,from(proxy),WaitForResponse,jason->retransmit(busy)),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub //Client side
            (
               derek->expect(INVITE/486,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInviteServerRetransmits503()
   {
      WarningLog(<<"*!testInviteServerRetransmits503!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> error;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,from(proxy),WaitForCommand,error <= jason->send503()),
         
         And
         (
            Sub //Server side
            (
               jason->expect(ACK,from(proxy),WaitForResponse,jason->retransmit(error)),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub //Client side
            (
               derek->expect(INVITE/480,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }


   void testInviteServerRetransmits603()
   {
      WarningLog(<<"*!testInviteServerRetransmits603!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> error;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,from(proxy),WaitForCommand,error <= jason->send603()),
         
         And
         (
            Sub //Server side
            (
               jason->expect(ACK,from(proxy),WaitForResponse,jason->retransmit(error)),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub //Client side
            (
               derek->expect(INVITE/603,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      
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
          jason->expect(INVITE/480, from(proxy), 5000, jason->ack()),
          WaitForEndOfTest);
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
          derek->expect(INVITE/480, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

   void testInviteClientDiesAfterFirstInvite()
   {
      WarningLog(<<"*!testInviteClientDiesAfterFirstInvite!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }
   
   void testInviteClientDiesAfterSecondInvite()
   {
      WarningLog(<<"*!testInviteClientDiesAfterSecondInvite!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      boost::shared_ptr<SipMessage> ok;
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
         derek->expect(INVITE/200,contact(jason),4800,chain(derek->noAction(),jason->pause(4000),jason->retransmit(ok))),      
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteServerDead()
   {
      WarningLog(<<"testInviteServerDead");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
            
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),600,jason->noAction()),
         jason->expect(INVITE,contact(derek),1100,jason->noAction()),
         jason->expect(INVITE,contact(derek),2100,jason->noAction()),
         jason->expect(INVITE,contact(derek),4100,jason->noAction()),
         jason->expect(INVITE,contact(derek),4100,jason->noAction()),
         jason->expect(INVITE,contact(derek),4100,jason->noAction()),
         jason->expect(INVITE,contact(derek),4100,jason->noAction()),
         optional(jason->expect(INVITE,contact(derek),4100,jason->noAction())),
         optional(jason->expect(INVITE,contact(derek),4100,jason->noAction())),
         optional(jason->expect(INVITE,contact(derek),4100,jason->noAction())),
         derek->expect(INVITE/408,from(proxy),5000,derek->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteLoop()
   {
      WarningLog(<<"*!testInviteLoop!*");
      
      RouteGuard dGuard(*proxy, "sip:loop@.*", "sip:loop@localhost");

      Seq
      (
         david->invite(proxy->makeUrl("loop").uri()),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(),david->digestRespond())),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/483,from(proxy),1000,david->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteForgedUserInFrom()
   {
      WarningLog(<<"*!testInviteForgedUserInFrom!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }

   void testInviteForgedHostInFrom()
   {
      WarningLog(<<"*!testInviteForgedHostInFrom!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }


   void testInviteLoopingRedirect()
   {
      WarningLog(<<"*!testInviteLoopingRedirect!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
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
   }

   void testInviteCancelCSeq()
   {
      WarningLog(<<"*!testInviteCancelCSeq!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(condition(cancelCSeq,jason->invite(*derek)),
          optional(jason->expect(CANCEL/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(CANCEL/400, from(proxy), WaitForResponse, jason->ack()),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

   void testInviteUnknownCSeq()
   {
      WarningLog(<<"*!testInviteUnknownCSeq!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(condition(unknownCSeq,jason->invite(*derek)),
          optional(jason->expect(UNKNOWN/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(UNKNOWN/400, from(proxy), WaitForResponse, jason->ack()),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

   void testCancelInviteCSeq()
   {
      WarningLog(<<"*!testCancelInviteCSeq!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          1000);
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          1000);
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
               jason->expect(INVITE, contact(derek), WaitForCommand, jason->ring()),
               derek->expect(INVITE/180, from(jason), WaitFor180, condition(inviteCSeq,derek->cancel())),
               derek->expect(INVITE/400, from(proxy), WaitForResponse, jason->answer()),
               derek->expect(INVITE/200,from(proxy), WaitForResponse, derek->ack())
            )
         ),
         WaitForEndOfTest
      );    

      ExecuteSequences();  
   }


//*******************Forking INVITES, parallel******************//


//*************Sunny Day***************//       
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
                          jason2->expect(ACK, from(derek), WaitForResponse, jason2->noAction())),
                      Sub(jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->send486()),
                          jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction()))))),
          WaitForEndOfTest);
      
      ExecuteSequences();  
   }

   void testInviteAllBusyContacts()
   {
      WarningLog(<<"*!testInviteAllBusyContacts!*");
      Seq(derek->registerUser(60,mergeContacts(*david,*cullen,*enlai)),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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

   void testInviteForkCallerHangsUp()
   {
      WarningLog(<<"*!testInviteForkCallerHangsUp!*");

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
      
   }


   void testInviteForkCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteForkCalleeHangsUp!*");

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
      
   }





//*************Cloudy Day***************//

   void testInviteForkThenSpiral()
   {
      WarningLog(<<"*!testInviteForkThenSpiral!*");
      
      set<NameAddr> contacts=mergeContacts(*derek,*jason);
      NameAddr spiral(Uri("sip:spiral@localhost"));
      contacts.insert(spiral);
      
      RouteGuard enter(*proxy,"sip:spiral@.*","sip:spiral1@localhost");
      RouteGuard spiral1(*proxy,"sip:spiral1@.*","sip:spiral2@localhost");
      RouteGuard spiral2(*proxy,"sip:spiral2@.*","sip:spiral3@localhost");
      RouteGuard spiral3(*proxy,"sip:spiral3@.*","sip:spiral4@localhost");
      RouteGuard spiral4(*proxy,"sip:spiral4@.*","sip:spiral5@localhost");
      RouteGuard spiral5(*proxy,"sip:spiral5@.*","sip:spiral6@localhost");
      RouteGuard spiral6(*proxy,"sip:spiral6@.*","sip:spiral7@localhost");
      RouteGuard spiral7(*proxy,"sip:spiral7@.*","sip:spiral8@localhost");
      RouteGuard spiral8(*proxy,"sip:spiral8@.*","sip:spiral9@localhost");
      RouteGuard spiral9(*proxy,"sip:spiral9@.*","sip:exit@localhost");
      RouteGuard exit(*proxy,"sip:exit@.*","sip:cullen@localhost");
      
      Seq
      (
         cullen->registerUser(60,cullen->getDefaultContacts()),
         cullen->expect(REGISTER/407, from(proxy),WaitForResponse,cullen->digestRespond()),
         cullen->expect(REGISTER/200,from(proxy),WaitForResponse,cullen->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407, from(proxy),WaitForResponse,enlai->digestRespond()),
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
               jason->expect(INVITE,contact(david),WaitForCommand,jason->ring()),
               david->expect(INVITE/180,contact(jason),WaitForResponse,david->noAction()),
               jason->expect(CANCEL,from(proxy),3000,chain(jason->ok(), jason->send487())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitForResponse,david->noAction()),
               derek->expect(CANCEL,from(proxy),3000,chain(derek->ok(), derek->send487())),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               cullen->expect(INVITE,contact(david),WaitForCommand,chain(cullen->ring(),cullen->pause(1000),cullen->answer())),
               david->expect(INVITE/180,contact(cullen),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(cullen),WaitForResponse,david->ack()),
               cullen->expect(ACK,contact(david),WaitForCommand,cullen->noAction())
            )
         ),
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteSpiralThenFork()
   {
      WarningLog(<<"*!testInviteSpiralThenFork!*");
      
      
      RouteGuard enter(*proxy,"sip:spiral@.*","sip:spiral1@localhost");
      RouteGuard spiral1(*proxy,"sip:spiral1@.*","sip:spiral2@localhost");
      RouteGuard spiral2(*proxy,"sip:spiral2@.*","sip:spiral3@localhost");
      RouteGuard spiral3(*proxy,"sip:spiral3@.*","sip:spiral4@localhost");
      RouteGuard spiral4(*proxy,"sip:spiral4@.*","sip:spiral5@localhost");
      RouteGuard spiral5(*proxy,"sip:spiral5@.*","sip:spiral6@localhost");
      RouteGuard spiral6(*proxy,"sip:spiral6@.*","sip:spiral7@localhost");
      RouteGuard spiral7(*proxy,"sip:spiral7@.*","sip:spiral8@localhost");
      RouteGuard spiral8(*proxy,"sip:spiral8@.*","sip:spiral9@localhost");
      RouteGuard spiral9(*proxy,"sip:spiral9@.*","sip:exit@localhost");
      RouteGuard exit(*proxy,"sip:exit@.*","sip:enlai@localhost");

      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(proxy->makeUrl("spiral").uri()),
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
               jason->expect(INVITE,contact(david),WaitForCommand,jason->ring()),
               david->expect(INVITE/180,contact(jason),WaitForResponse,david->noAction()),
               jason->expect(CANCEL,from(proxy),3000,chain(jason->ok(), jason->send487())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitForResponse,david->noAction()),
               derek->expect(CANCEL,from(proxy),3000,chain(derek->ok(), derek->send487())),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->pause(1000),enlai->answer())),
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }


   void testInviteForkAll4xxResponses()
   {
      WarningLog(<<"*!testInviteForkAll4xxResponses!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
            )
         ),
         david->expect(INVITE/480,from(proxy),WaitForResponse,david->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   
   void testInviteFork200And4xx()
   {
      WarningLog(<<"*!testInviteFork200And4xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInviteForkAll5xxResponses()
   {
      WarningLog(<<"*!testInviteForkAll5xxResponses!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
            )
         ),
         david->expect(INVITE/513,from(proxy),WaitForResponse,david->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   
   void testInviteFork200And5xx()
   {
      WarningLog(<<"*!testInviteFork200And5xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteForkAll6xxResponses()
   {
      WarningLog(<<"*!testInviteForkAll6xxResponses!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
   }
   
   void testInviteFork6xxBeats200()
   {
      WarningLog(<<"*!testInviteFork6xxBeats200!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
      
   }
      

   void testInviteFork200Beats6xx()
   {
      WarningLog(<<"*!testInviteFork200Beats6xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
      
   }
      
   void testInviteFork4xxAnd5xx()
   {
      WarningLog(<<"*!testInviteFork4xxAnd5xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
         david->expect(INVITE/403,from(proxy),WaitForResponse,david->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteFork4xx5xx6xx()
   {
      WarningLog(<<"*!testInviteFork4xx5xx6xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
   }
   
      
   void testInviteForkMerges()
   {
      WarningLog(<<"*!testInviteForkMerges!*");
      
      set<NameAddr> contacts;
      
      contacts.insert(NameAddr(Uri("sip:derek@localhost")));
      contacts.insert(NameAddr(Uri("sip:jason@localhost")));
      
      RouteGuard merge1(*proxy,"sip:derek@.*","sip:enlai@localhost");
      RouteGuard merge2(*proxy,"sip:jason@.*","sip:enter@localhost");
      RouteGuard enter(*proxy,"sip:enter@.*","sip:spiral1@localhost");
      RouteGuard spiral1(*proxy,"sip:spiral1@.*","sip:spiral2@localhost");
      RouteGuard spiral2(*proxy,"sip:spiral2@.*","sip:spiral3@localhost");
      RouteGuard spiral3(*proxy,"sip:spiral3@.*","sip:spiral4@localhost");
      RouteGuard spiral4(*proxy,"sip:spiral4@.*","sip:spiral5@localhost");
      RouteGuard spiral5(*proxy,"sip:spiral5@.*","sip:spiral6@localhost");
      RouteGuard spiral6(*proxy,"sip:spiral6@.*","sip:spiral7@localhost");
      RouteGuard spiral7(*proxy,"sip:spiral7@.*","sip:spiral8@localhost");
      RouteGuard spiral8(*proxy,"sip:spiral8@.*","sip:spiral9@localhost");
      RouteGuard spiral9(*proxy,"sip:spiral9@.*","sip:exit@localhost");
      RouteGuard exit(*proxy,"sip:exit@.*","sip:enlai@localhost");
      
      Seq
      (
         cullen->registerUser(60,contacts),
         cullen->expect(REGISTER/407, from(proxy),WaitForResponse,cullen->digestRespond()),
         cullen->expect(REGISTER/200,from(proxy),WaitForResponse,cullen->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         enlai->registerUser(60,enlai->getDefaultContacts()),
         enlai->expect(REGISTER/407, from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(*cullen),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         david->expect(INVITE/407,from(proxy),WaitForResponse,chain(david->ack(),david->digestRespond())),
         optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction())),
         
         enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->answer())),
         And
         (
            Sub
            (
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForResponse,enlai->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->ring()),
               enlai->expect(CANCEL,from(proxy),3000,chain(enlai->ok(), enlai->send487())),
               enlai->expect(ACK,from(proxy),WaitForResponse,enlai->noAction())
            )
         ),
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteForkAllAnswerNo1xx()
   {
      WarningLog(<<"*!testInviteForkAllAnswerNo1xx!*");
      
      set<NameAddr> contacts = mergeContacts(*derek,*jason,*enlai);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
   }

   void testSequentialQValueInvite()
   {
      WarningLog(<<"*!testSequentialQValueInvite!*");
      NameAddr con2 = *(jason->getDefaultContacts().begin());
      con2.param(p_q)=0.2;

      NameAddr con1 = *(derek->getDefaultContacts().begin());
      con1.param(p_q)=0.1;
      
      std::set<resip::NameAddr> contacts;
      contacts.insert(con1);
      contacts.insert(con2);
      
      Seq
      (
         derek->registerUser(60,contacts),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
   
      Seq
      (
         cullen->invite(*derek),
         optional(cullen->expect(INVITE/100, from(proxy), WaitFor100, cullen->noAction())),
         cullen->expect(INVITE/407, from(proxy), WaitForResponse, chain(cullen->ack(),cullen->digestRespond())),
         
         And
         (
            Sub
            (
               optional(cullen->expect(INVITE/100, from(proxy), WaitFor100, cullen->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE, from(cullen), WaitForCommand, jason->ring()),
               cullen->expect(INVITE/180, from(jason), WaitForResponse, cullen->noAction()),
               jason->expect(CANCEL, from(proxy), 3000, chain(jason->ok(), jason->send487())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction()),

               derek->expect(INVITE, from(cullen), WaitForCommand, chain(derek->ring(),derek->ok())),
               cullen->expect(INVITE/180, from(derek), WaitForResponse,cullen->noAction()),
               cullen->expect(INVITE/200, from(derek), WaitForResponse, cullen->ack()),
               derek->expect(ACK, from(cullen),WaitForCommand, derek->noAction())
            )
         ),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
   }


//*************Rainy Day***************//


       
   void testCancelTimeout()
   {
      WarningLog(<<"*!Test Resip Exploit 1!*");
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(cullen->invite(*jason),
          optional(cullen->expect(INVITE/100,from(proxy),WaitFor100,cullen->noAction())),
          cullen->expect(INVITE/407, from(proxy), WaitForResponse, chain(cullen->ack(), cullen->digestRespond())),
          And( Sub(optional(cullen->expect(INVITE/100,from(proxy),1000, cullen->noAction())),
                   cullen->expect(INVITE/180,from(jason), 1000, cullen->cancel()),
                   cullen->expect(CANCEL/200,from(proxy),1000,cullen->noAction())),
               Sub(jason->expect(INVITE,contact(cullen),1000,jason->ring()),
                   jason->expect(CANCEL,from(proxy),1000,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()),
                   jason->expect(CANCEL,from(proxy),4800,jason->noAction()))),
          WaitForEndOfTest);
      ExecuteSequences();
   }

   void testReproExploit1()
   {
      WarningLog(<<"*!Test Repro Exploit 1!*");
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq(  cullen->invite(*jason),
            optional(cullen->expect(INVITE/100,from(proxy),WaitFor100,cullen->noAction())),
            cullen->expect(INVITE/407, from(proxy), WaitForResponse,chain(cullen->ack(),cullen->digestRespond())),
            And( Sub(cullen->expect(INVITE/100,from(proxy),WaitFor100,cullen->cancel()),
                     cullen->expect(CANCEL/200,from(proxy),4000,cullen->noAction())),
                 Sub(jason->expect(INVITE,contact(cullen),4000,jason->noAction()),
                     jason->expect(INVITE,contact(cullen),4000,jason->noAction()),
                     jason->expect(INVITE,contact(cullen),4000,jason->noAction()),
                     jason->expect(INVITE,contact(cullen),4000,chain(jason->send100(), jason->pause(500), jason->ring())),
                     jason->expect(CANCEL,from(proxy),4000,jason->ok()))),
            WaitForEndOfTest);
      ExecuteSequences();
                     
   }
   
//********************Forking INVITE, sequential***************//

   void testInviteSeqForkOneBusy()
   {
      WarningLog(<<"*!testInviteSeqForkOneBusy!*");
      resip::NameAddr contact1=*(jason1->getDefaultContacts().begin());
      contact1.param(p_q)=0.1;
      resip::NameAddr contact2=*(jason2->getDefaultContacts().begin());
      contact2.param(p_q)=0.2;

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
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
               jason2->expect(INVITE, contact(derek), WaitForCommand, jason2->ring()),
               derek->expect(INVITE/180, from(jason2), WaitFor180, jason2->send486()),
               And
               (
                  Sub
                  (
                     jason2->expect(ACK, from(proxy), WaitForResponse, jason2->noAction())
                  ),
                  Sub
                  (
                     jason1->expect(INVITE, contact(derek), WaitForCommand, chain(jason1->ring(), jason1->answer()))
                  )
               ),
               derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()),
               derek->expect(INVITE/200, from(jason1), WaitForCommand, derek->ack()),
               jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  
   }


   void testInviteSeqAllBusyContacts()
   {
      WarningLog(<<"*!testInviteSeqAllBusyContacts!*");
      std::set<resip::NameAddr> contacts;

      resip::NameAddr contact1=*(enlai->getDefaultContacts().begin());
      contact1.param(p_q)=0.1;
      resip::NameAddr contact2=*(cullen->getDefaultContacts().begin());
      contact2.param(p_q)=0.2;
      resip::NameAddr contact3=*(david->getDefaultContacts().begin());
      contact3.param(p_q)=0.3;
      
      contacts.insert(contact1);
      contacts.insert(contact2);
      contacts.insert(contact3);
      
      Seq(derek->registerUser(60,contacts),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
         And
         (
            Sub
            (
                optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))
            ),
            Sub
            (
                 david->expect(INVITE, contact(jason), WaitForCommand, david->send503()),
               And
               (
                  Sub
                  (
                     david->expect(ACK,from(proxy),WaitForCommand,david->noAction())
                  ),
                  Sub
                  (
                     cullen->expect(INVITE, contact(jason), WaitForCommand, cullen->send503())
                  )
               ),
               And
               (
                  Sub
                  (
                     cullen->expect(ACK,from(proxy),WaitForCommand,cullen->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE, contact(jason), WaitForCommand, enlai->send503())
                  )
               ),
               And
               (
                  Sub
                  (
                     enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE/480, from(proxy), 3*WaitForCommand, jason->ack())
                  )
               )
            )
         ),
          WaitForEndOfTest);
      
      ExecuteSequences();  
   }

   void testInviteSeqForkCallerCancels()
   {
      WarningLog(<<"*!testInviteSeqForkThreeCallerCancels!*");
      resip::NameAddr contact1=*(jason1->getDefaultContacts().begin());
      contact1.param(p_q)=0.1;
      resip::NameAddr contact2=*(jason2->getDefaultContacts().begin());
      contact2.param(p_q)=0.2;
      resip::NameAddr contact3=*(jason3->getDefaultContacts().begin());
      contact3.param(p_q)=0.3;
      
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason3->registerUser(60, contact3),
          jason3->expect(REGISTER/407, from(proxy), WaitForResponse, jason3->digestRespond()),
          jason3->expect(REGISTER/200, from(proxy), WaitForRegistration, jason3->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();

      CountDown count487(3, "count487");
      
      Seq(derek->invite(*jason),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
          And
          (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason3->expect(INVITE,contact(derek),WaitForCommand, jason3->ring()),
               derek->expect(INVITE/180,contact(jason3),WaitForResponse,jason3->send486()),
               And
               (
                  Sub
                  (
                     jason3->expect(ACK,from(proxy),WaitForCommand,jason3->noAction())
                  ),
                  Sub
                  (
                     jason2->expect(INVITE,contact(derek),WaitForCommand,jason2->ring())
                  )
               ),
               derek->expect(INVITE/180,contact(jason2),WaitForResponse,derek->cancel()),
               And
               (
                  Sub
                  (
                     derek->expect(CANCEL/200,from(proxy),WaitForCommand,derek->noAction()),
                     derek->expect(INVITE/486,from(proxy),WaitForResponse,derek->ack())
                  ),
                  Sub
                  (
                     jason2->expect(CANCEL,from(proxy),WaitForCommand,chain(jason2->ok(),jason2->send487())),
                     jason2->expect(ACK,from(proxy),WaitForCommand,jason2->noAction())
                  )
               )
            )
          ),
          WaitForEndOfTest);
      
      ExecuteSequences();  
   }

   void testInviteSeqForkCallerHangsUp()
   {
      WarningLog(<<"*!testInviteSeqForkCallerHangsUp!*");
      resip::NameAddr contact1=*(jason1->getDefaultContacts().begin());
      contact1.param(p_q)=0.1;
      resip::NameAddr contact2=*(jason2->getDefaultContacts().begin());
      contact2.param(p_q)=0.2;
      resip::NameAddr contact3=*(jason3->getDefaultContacts().begin());
      contact3.param(p_q)=0.3;

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      Seq(jason3->registerUser(60, contact3),
          jason3->expect(REGISTER/407, from(proxy), WaitForResponse, jason3->digestRespond()),
          jason3->expect(REGISTER/200, from(proxy), WaitForRegistration, jason3->noAction()),
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
               jason3->expect(INVITE,contact(derek),WaitForCommand,jason3->send486()),
               And
               (
                  Sub
                  (
                     jason3->expect(ACK,from(proxy),WaitForCommand,jason3->noAction())
                  ),
                  Sub
                  (
                     jason2->expect(INVITE,contact(derek),WaitForCommand,jason2->ring())
                  )
               ),
               derek->expect(INVITE/180,from(jason2),WaitForResponse,jason2->ok()),
               derek->expect(INVITE/200,from(jason2),WaitForResponse,derek->ack()),
               jason2->expect(ACK,from(proxy),WaitForCommand,chain(derek->pause(2000),derek->bye())),
               jason2->expect(BYE,contact(derek),WaitForCommand,jason2->ok()),
               derek->expect(BYE/200,contact(jason2),WaitForResponse,derek->noAction())                  
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  
      
   }


   void testInviteSeqForkCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteSeqForkCalleeHangsUp!*");

      resip::NameAddr contact1=*(jason1->getDefaultContacts().begin());
      contact1.param(p_q)=0.1;
      resip::NameAddr contact2=*(jason2->getDefaultContacts().begin());
      contact2.param(p_q)=0.2;
      resip::NameAddr contact3=*(jason3->getDefaultContacts().begin());
      contact3.param(p_q)=0.3;

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      Seq(jason3->registerUser(60, contact3),
          jason3->expect(REGISTER/407, from(proxy), WaitForResponse, jason3->digestRespond()),
          jason3->expect(REGISTER/200, from(proxy), WaitForRegistration, jason3->noAction()),
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
               jason3->expect(INVITE,contact(derek),WaitForCommand,jason3->send486()),
               And
               (
                  Sub
                  (
                     jason3->expect(ACK,from(proxy),WaitForCommand,jason3->noAction())
                  ),
                  Sub
                  (
                     jason2->expect(INVITE,contact(derek),WaitForCommand,jason2->ring())
                  )
               ),
               derek->expect(INVITE/180,from(jason2),WaitForResponse,jason2->ok()),
               derek->expect(INVITE/200,from(jason2),WaitForResponse,derek->ack()),
               jason2->expect(ACK,from(proxy),WaitForCommand,chain(jason2->pause(2000),jason2->bye())),
               derek->expect(BYE,contact(jason2),WaitForCommand,derek->ok()),
               jason2->expect(BYE/200,contact(derek),WaitForResponse,jason2->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  
   }


//*************Cloudy Day***************//

   void testInviteSeqForkThenSpiral()
   {
      WarningLog(<<"*!testInviteSeqForkThenSpiral!*");
      resip::NameAddr spiral("sip:spiral@localhost");
      spiral.param(p_q)=0.4;
      resip::NameAddr con3=*(derek->getDefaultContacts().begin());
      con3.param(p_q)=0.3;
      resip::NameAddr con2=*(jason->getDefaultContacts().begin());
      con2.param(p_q)=0.2;
      
      set<resip::NameAddr> contacts;
      contacts.insert(spiral);
      contacts.insert(con3);
      contacts.insert(con2);
      
      RouteGuard enter(*proxy,"sip:spiral@.*","sip:spiral1@localhost");
      RouteGuard spiral1(*proxy,"sip:spiral1@.*","sip:spiral2@localhost");
      RouteGuard spiral2(*proxy,"sip:spiral2@.*","sip:spiral3@localhost");
      RouteGuard spiral3(*proxy,"sip:spiral3@.*","sip:spiral4@localhost");
      RouteGuard spiral4(*proxy,"sip:spiral4@.*","sip:spiral5@localhost");
      RouteGuard spiral5(*proxy,"sip:spiral5@.*","sip:spiral6@localhost");
      RouteGuard spiral6(*proxy,"sip:spiral6@.*","sip:spiral7@localhost");
      RouteGuard spiral7(*proxy,"sip:spiral7@.*","sip:spiral8@localhost");
      RouteGuard spiral8(*proxy,"sip:spiral8@.*","sip:spiral9@localhost");
      RouteGuard spiral9(*proxy,"sip:spiral9@.*","sip:exit@localhost");
      RouteGuard exit(*proxy,"sip:exit@.*","sip:cullen@localhost");
      
      Seq
      (
         cullen->registerUser(60,cullen->getDefaultContacts()),
         cullen->expect(REGISTER/407, from(proxy),WaitForResponse,cullen->digestRespond()),
         cullen->expect(REGISTER/200,from(proxy),WaitForResponse,cullen->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407, from(proxy),WaitForResponse,enlai->digestRespond()),
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
               cullen->expect(INVITE,contact(david),WaitForCommand,cullen->ring()),
               david->expect(INVITE/180,contact(cullen),WaitForResponse,david->noAction()),
               cullen->expect(CANCEL,from(proxy),3000,chain(cullen->ok(),cullen->send487())),
               And
               (
                  Sub
                  (
                     cullen->expect(ACK,from(proxy),WaitForCommand,cullen->noAction())
                  ),
                  Sub
                  (
                     derek->expect(INVITE,contact(david),WaitForCommand,derek->ring())
                  )
               ),

               david->expect(INVITE/180,contact(derek),WaitForResponse,david->noAction()),
               derek->expect(CANCEL,from(proxy),3000,chain(derek->ok(), derek->send487())),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,chain(jason->ring(),jason->answer()))
                  )
               ),

               david->expect(INVITE/180,contact(jason),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(jason),WaitForResponse,david->ack()),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            )
         ),


         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteSpiralThenSeqFork()
   {
      WarningLog(<<"*!testInviteSpiralThenSeqFork!*");
      
      
      RouteGuard enter(*proxy,"sip:spiral@.*","sip:spiral1@localhost");
      RouteGuard spiral1(*proxy,"sip:spiral1@.*","sip:spiral2@localhost");
      RouteGuard spiral2(*proxy,"sip:spiral2@.*","sip:spiral3@localhost");
      RouteGuard spiral3(*proxy,"sip:spiral3@.*","sip:spiral4@localhost");
      RouteGuard spiral4(*proxy,"sip:spiral4@.*","sip:spiral5@localhost");
      RouteGuard spiral5(*proxy,"sip:spiral5@.*","sip:spiral6@localhost");
      RouteGuard spiral6(*proxy,"sip:spiral6@.*","sip:spiral7@localhost");
      RouteGuard spiral7(*proxy,"sip:spiral7@.*","sip:spiral8@localhost");
      RouteGuard spiral8(*proxy,"sip:spiral8@.*","sip:spiral9@localhost");
      RouteGuard spiral9(*proxy,"sip:spiral9@.*","sip:exit@localhost");
      RouteGuard exit(*proxy,"sip:exit@.*","sip:enlai@localhost");

      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
         enlai->expect(REGISTER/200,from(proxy),WaitForResponse,enlai->noAction()),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         david->invite(proxy->makeUrl("spiral").uri()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitForResponse,david->noAction()),
               derek->expect(CANCEL,from(proxy),3000,chain(derek->ok(), derek->send487())),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->ring())
                  )
               ),

               david->expect(INVITE/180,contact(jason),WaitForResponse,david->noAction()),
               jason->expect(CANCEL,from(proxy),3000,chain(jason->ok(), jason->send487())),
               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->answer()))
                  )
               ),

               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())            
            )
         ),
         
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }


   void testInviteSeqForkAll4xxResponses()
   {
      WarningLog(<<"*!testInviteSeqForkAll4xxResponses!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send404()),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send403())
                  )
               ),

               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send480())
                  )
               ),

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
   }
   
   
   void testInviteSeqFork200And4xx()
   {
      WarningLog(<<"*!testInviteSeqFork200And4xx!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send404()),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send403())
                  )
               ),

               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->answer()))
                  )
               ),

               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         

         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInviteSeqForkAll5xxResponses()
   {
      WarningLog(<<"*!testInviteSeqForkAll5xxResponses!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send503()),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send500())
                  )
               ),

               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send513())
                  )
               ),

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
   }
   
   
   void testInviteSeqFork200And5xx()
   {
      WarningLog(<<"*!testInviteSeqFork200And5xx!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send503()),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send500())
                  )
               ),

               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->answer()))
                  )
               ),

               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForCommand,enlai->noAction())
            )
         ),
         

         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteSeqFork6xxResponse()
   {
      WarningLog(<<"*!testInviteSeqFork6xxResponse!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitForResponse,david->noAction()),
               derek->expect(CANCEL,from(proxy),3000,chain(derek->ok(),derek->send487())),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForCommand,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send600())
                  )
               ),

               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction()),
               david->expect(INVITE/600,from(proxy),WaitForResponse,david->ack())
            )
         ),
         
               
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteSeqFork4xxAnd5xx()
   {
      WarningLog(<<"*!testInviteSeqFork4xxAnd5xx!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send403()),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send503())
                  )
               ),

               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send404())
                  )
               ),

               And
               (
                  Sub
                  (
                     enlai->expect(ACK,from(proxy),WaitForCommand,enlai->noAction())
                  ),
                  Sub
                  (
                     david->expect(INVITE/403,from(proxy),WaitForResponse,david->ack())
                  )
               )

            )
         ),
         
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteSeqFork4xx5xx6xx()
   {
      WarningLog(<<"*!testInviteFork4xx5xx6xx!*");
      
      resip::NameAddr derekc=*(derek->getDefaultContacts().begin());
      derekc.param(p_q)=0.3;
      resip::NameAddr jasonc=*(jason->getDefaultContacts().begin());
      jasonc.param(p_q)=0.2;
      resip::NameAddr enlaic=*(enlai->getDefaultContacts().begin());
      enlaic.param(p_q)=0.1;

      set<NameAddr> contacts;
      
      contacts.insert(derekc);
      contacts.insert(jasonc);
      contacts.insert(enlaic);
      
      Seq
      (
         enlai->registerUser(60,contacts),
         enlai->expect(REGISTER/407,from(proxy),WaitForResponse,enlai->digestRespond()),
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
               derek->expect(INVITE,contact(david),WaitForCommand,derek->send503()),            
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommand,jason->send404())
                  )
               ),

               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommand,enlai->send600())
                  )
               ),

               And
               (
                  Sub
                  (
                     enlai->expect(ACK,from(proxy),WaitForResponse,enlai->noAction())
                  ),
                  Sub
                  (
                     david->expect(INVITE/600,contact(enlai),WaitForResponse,david->ack())
                  )
               )

            )
         ),
         


         WaitForEndOfTest
      );
      
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
      
      
      
       void testTimerCInterference()
      {
         WarningLog(<<"*!testTimerCInterference!*");
         Seq(jason->registerUser(600, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
             1000);
         Seq(derek->registerUser(600, derek->getDefaultContacts()),
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
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
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         Seq(jason1->registerUser(600, jason1->getDefaultContacts()),
             jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
             jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
             WaitForEndOfSeq);
         Seq(jason2->registerUser(600, jason2->getDefaultContacts()),
             jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
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
             derek->expect(REGISTER/407, from(proxy), WaitForResponse,
	                   derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           derek->noAction()),
	     WaitForEndOfSeq);

         Seq(david->registerUser(600, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse,
	                   david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           david->noAction()),
	     WaitForEndOfSeq);

         Seq(enlai->registerUser(600, enlai->getDefaultContacts()),
             enlai->expect(REGISTER/407, from(proxy), WaitForResponse,
	                   enlai->digestRespond()),
             enlai->expect(REGISTER/200, from(proxy), WaitForRegistration,
                           enlai->noAction()),
	     WaitForEndOfSeq);

         Seq(jason->registerUser(600, jason->getDefaultContacts()),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse,
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
             derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
             derek->expect(REGISTER/200, from(proxy), WaitForResponse, jason->registerUser(60, jason->getDefaultContacts())),
             jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
             jason->expect(REGISTER/200, from(proxy), WaitForResponse, derek->invite(*jason)),
             optional(derek->expect(INVITE/100, from(proxy), WaitForResponse, derek->noAction())),
             derek->expect(INVITE/407, from(proxy),  WaitForResponse, chain(derek->ack(), derek->digestRespond())),
             And
             (
               Sub
               (
                  optional(derek->expect(INVITE/100, from(proxy), WaitForResponse, derek->noAction()))
               ),
               Sub
               (
                   jason->expect(INVITE, from(proxy), WaitForResponse, jason->ring183())
               )
             ),
            derek->expect(INVITE/183, from(proxy), WaitForResponse, chain(derek->pause(5000), jason->answer())),
            derek->expect(INVITE/200, from(proxy), WaitForResponse+5000, derek->ack()),
            jason->expect(ACK, from(proxy), WaitForResponse, jason->bye(*derek)),
            derek->expect(BYE, from(proxy), WaitForResponse, derek->ok()),
            jason->expect(BYE/200, from(proxy), WaitForResponse, derek->noAction()),
            WaitForEndOfTest);
         ExecuteSequences();
      }

      void testTCPMultiMsg()
      {
         InfoLog(<< "*!testTCPMultiMsg!*");
         Uri server("sip:127.0.0.1:5060");

         Seq(jasonTcp->registerUser(60, jasonTcp->getDefaultContacts()),
             jasonTcp->expect(REGISTER/407, from(proxy), WaitForResponse, jasonTcp->digestRespond()),
             jasonTcp->expect(REGISTER/200, from(proxy), WaitForResponse, jasonTcp->noAction()),
             WaitForEndOfTest);
      
         ExecuteSequences();
         
         boost::shared_ptr<SipMessage> ring;
         
         Seq
         (
            jozsef->invite(*jasonTcp),
            optional(jozsef->expect(INVITE/100, from(proxy),WaitFor100,jozsef->noAction())),
            jozsef->expect(INVITE/407, from(proxy),WaitForResponse, chain(jozsef->ack(),jozsef->digestRespond())),
            And
            (
               Sub
               (
                  optional(jozsef->expect(INVITE/100, from(proxy),WaitFor100,jozsef->noAction()))
               ),
               Sub
               (
                  jasonTcp->expect(INVITE, from(jozsef), WaitForCommand, chain(rawcondition(doubleSend,jasonTcp->ring()),jasonTcp->answer())),
                  jozsef->expect(INVITE/180, from(jasonTcp),WaitForResponse, jozsef->noAction()),
                  jozsef->expect(INVITE/180, from(jasonTcp),WaitForResponse, jozsef->noAction()),
                  jozsef->expect(INVITE/200, from(jasonTcp),WaitForResponse, jozsef->ack()),
                  jasonTcp->expect(ACK, from(proxy),WaitForCommand,jasonTcp->noAction())
               )
            ),
            WaitForEndOfTest
         );
         
         ExecuteSequences();
      }

      void testTCPPreparseError()
      {
         InfoLog(<< "*!testTCPPreparseError!*");

         Random::initialize();

         Uri server;
         server.host() = "127.0.0.1";
         server.port() = 5060;
//         server.host() = proxy->getUri().host();
//         server.port() = proxy->getUri().port();
         Data preparseError = "fMBMTyr0ChZkQM0Ue3DLPSInEQKSMKVUiHRCU1tMKnyGC55/nhZbZNxd5wJRcjFcPNA=";
         Data errMsg = preparseError.base64decode();

         Seq(jozsef->registerUser(60, jozsef->getDefaultContacts()),
             jozsef->expect(REGISTER/407, from(proxy), WaitForResponse, jozsef->digestRespond()),
             jozsef->expect(REGISTER/200, from(proxy), WaitForResponse, chain(jozsef->rawSend(server, errMsg), jozsef->registerUser(60, jozsef->getDefaultContacts()))),
             jozsef->expect(REGISTER/407, from(proxy), WaitForResponse, jozsef->digestRespond()),
             jozsef->expect(REGISTER/200, from(proxy), WaitForResponse, jozsef->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testTCPParseBufferError()
      {
         InfoLog(<< "*!testTCPParseBufferError!*");

         Random::initialize();

         Uri server;
         server.host() = "127.0.0.1";
         server.port() = 5060;
//         server.host() = proxy->getUri().host();
//         server.port() = proxy->getUri().port();
         Data parseBufferError = "NsKkOMmAiiygF/lFP0d2DYWrbQwOZx5X/UG5Eiv0dgoJ++Y8fT+RfM83cg6CEHxnNh8=";
         Data errMsg = parseBufferError.base64decode();

         Seq(jozsef->registerUser(60, jozsef->getDefaultContacts()),
             jozsef->expect(REGISTER/407, from(proxy), WaitForResponse, jozsef->digestRespond()),
             jozsef->expect(REGISTER/200, from(proxy), WaitForResponse, chain(jozsef->rawSend(server, errMsg), jozsef->registerUser(60, jozsef->getDefaultContacts()))),
             jozsef->expect(REGISTER/407, from(proxy), WaitForResponse, jozsef->digestRespond()),
             jozsef->expect(REGISTER/200, from(proxy), WaitForResponse, jozsef->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }


      // provisioning here(automatic cleanup)
      static void createStatic()
      {
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
      static CppUnit::Test* suite()
      {
         CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "Suite1" );
#if 1
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
         //TEST(testRegisterNoUserInTo); //repro doesn't compain about this yet
         //TEST(testRegisterUserInReqUri); //repro doesn't compain about this yet
         //TEST(testRegisterUnknownAorHost); //repro doesn't compain about this yet
         //TEST(testRegisterUnknownAorUser); //repro doesn't compain about this yet
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
         //TEST(testUnregisterNonExistent);//repro doesn't compain about this yet
         TEST(testFetch);
         TEST(testExpiryCleanup);
         //TEST(testFetchNonExistent); //repro doesn't compain about this yet


//Proxy tests

         TEST(testInviteBasic);
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
         TEST(testInvite200BeatsCancelClientSide);
         TEST(testInviteNotFound);
         TEST(testInvite488Response);
         TEST(testInvite480Response);
         TEST(testInvite500Response);
         TEST(testInvite503Response);
         TEST(testInvite600Response);
         TEST(testInvite603Response);
         TEST(testInviteServerSpams180);
         TEST(testInviteBogusAuth);
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
         //TEST(testInviteTransportFailure); //tfm asserts
         TEST(testInviteClientDiesAfterFirstInvite);
         TEST(testInviteClientDiesAfterSecondInvite);
         TEST(testInviteServerDead);
         TEST(testInviteLoop); 
         TEST(testInviteForgedUserInFrom);
         TEST(testInviteCancelCSeq);
         TEST(testInviteUnknownCSeq);
         TEST(testCancelInviteCSeq);
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
         TEST(testInviteForkMerges);
         //TEST(testInviteForkAllAnswerNo1xx); //tfm is messing this one up
         TEST(testSequentialQValueInvite);

         TEST(testInviteSeqForkOneBusy);
         TEST(testInviteSeqAllBusyContacts);
         TEST(testInviteSeqForkCallerCancels);
         TEST(testInviteSeqForkCallerHangsUp);
         TEST(testInviteSeqForkCalleeHangsUp);

         TEST(testInviteSeqForkThenSpiral);
         TEST(testInviteSpiralThenSeqFork);
         TEST(testInviteSeqForkAll4xxResponses);
         TEST(testInviteSeqFork200And4xx);
         TEST(testInviteSeqForkAll5xxResponses);
         TEST(testInviteSeqFork200And5xx);
         TEST(testInviteSeqFork6xxResponse);
         TEST(testInviteSeqFork4xxAnd5xx);
         TEST(testInviteSeqFork4xx5xx6xx);
         
         TEST(testInfo);
         TEST(testInviteClientRetransmitsAfter200);
         TEST(testNonInviteClientRetransmissionsWithRecovery);
         TEST(testNonInviteClientRetransmissionsWithTimeout);
         TEST(testNonInviteServerRetransmission);
         TEST(testBasic302);
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
         TEST(testRoutingBasic);
         TEST(testTCPMultiMsg);
         // TCP send errors 
         TEST(testTCPPreparseError);
         TEST(testTCPParseBufferError);
#else
         TEST(testRegisterBasic);
//         TEST(testMultiple1);
//         TEST(testInviteAllBusyContacts);
#endif         
         return suiteOfTests;
      }
};

int main(int argc, char** argv)
{
   initNetwork();
   try
   {
      CommandLineParser args(argc, argv);
      Log::initialize(args.mLogType, args.mLogLevel, argv[0]);
      resip::Timer::T100 = 0;
      
      TestHolder::createStatic();
      Fixture::initialize(argc, argv);
      
      CppUnit::TextTestRunner runner;

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
