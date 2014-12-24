#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <cppunit/TextTestRunner.h>
#include <cppunit/TextTestResult.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/XmlOutputter.h>

#include <signal.h>
#include "resip/stack/ApiCheckList.hxx"
#include "resip/stack/InteropHelper.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "tfm/repro/CommandLineParser.hxx"
#include "tfm/repro/ReproFixture.hxx"
#include "tfm/RouteGuard.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/SipEvent.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/predicates/ExpectUtils.hxx"
#include "tfm/CppTestSelector.hxx"
#include "tfm/CPTextTestProgressListener.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

#define TEST_RESULT_FILE "./testResult.xml"

using namespace CppUnit;
using namespace resip;

static const int WaitFor100 = 1000;
static const int WaitFor180 = 1000;
static const int WaitFor180Spiral = 5000;  // Give more time since response is going to spiral first
static const int WaitFor487 = 1000;
static const int WaitForAck = 1000;  //immediate ACK for 4xx and CANCEL; not ACK for 200
static const int WaitForCommand = 1000;
static const int WaitForCommandSpiral = 5000;  // Give more time since command is going to spiral first
static const int WaitForResponse = 1000;
static const int WaitForResponseSpiral = 5000; // Give more time since respond is going to spiral first
static const int WaitForResponseLoop = 60000; // Give more time since respond is going to loop first
static const int WaitForRegistration = 1000;
static const int PauseTime = 100;
static const int WaitForPause = 1100;
static const int WaitForEndOfTest = 100;
static const int WaitForEndOfSeq = 100;
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

class DisableDigestAuthGuard 
{
   public:
      DisableDigestAuthGuard(TestProxy& proxy) : mProxy(proxy)
       {
          // Disable digest authentication by treating messages from loopback adaptor as trusted
          mProxy.addTrustedHost("127.0.0.1", UDP, 0, 8, V4);
       }

       ~DisableDigestAuthGuard() 
       { 
          mProxy.deleteTrustedHost("127.0.0.1", UDP, 0, 8, V4);
       }

   private:
      TestProxy& mProxy;
};

class TestHolder : public ReproFixture
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
      badNonce(boost::shared_ptr<SipMessage> msg)
      {
         if(msg->exists(h_ProxyAuthorizations))
         {
            Auths::iterator i = msg->header(h_ProxyAuthorizations).begin();
            for(; i!=msg->header(h_ProxyAuthorizations).end(); ++i)
            {
               i->param(p_nonce)="ffffffffffffffffffffffffffffffff";
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
         resip_assert(msg->exists(h_Contacts) &&
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
      
      static boost::shared_ptr<SipMessage>
      missingTid(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(h_Vias).front().remove(p_branch);
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      unknownProxyRequire(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(h_ProxyRequires).push_back(Token("foobajooba"));
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      supportedProxyRequire(boost::shared_ptr<SipMessage> msg)
      {
         // !bwc! TODO Once we have something we _do_ support, put that here.
         msg->header(h_ProxyRequires).push_back(Token("p-fakeoption"));
         return msg;
      }

      static resip::Data
      doubleSend(const resip::Data& msg)
      {
         resip::Data result=msg+msg;
         return result;
      }

      static resip::Data
      make200Response(const resip::Data& msg)
      {
         // .bwc. Since we are sending this, we can be assured that the start 
         // line all lies on one line.
         size_t endStartLine = msg.find("\r\n");
         Data result("SIP/2.0 200 OK");
         result+=msg.substr(endStartLine);
         return result;
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
               resip_assert(sipEvent);
               boost::shared_ptr<SipMessage> msg = sipEvent->getMessage();
               resip_assert(!msg->exists(h_RecordRoutes));
            }
      };

      static boost::shared_ptr<SipMessage>
      makeMessage(boost::shared_ptr<SipMessage> msg)
      {
         if(msg->isRequest())
         {
            msg->header(h_RequestLine).method()=MESSAGE;
         }
         msg->header(h_CSeq).method()=MESSAGE;
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      makeInfo(boost::shared_ptr<SipMessage> msg)
      {
         if(msg->isRequest())
         {
            msg->header(h_RequestLine).method()=INFO;
         }
         msg->header(h_CSeq).method()=INFO;
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      makeInvite(boost::shared_ptr<SipMessage> msg)
      {
         if(msg->isRequest())
         {
            msg->header(h_RequestLine).method()=INVITE;
         }
         msg->header(h_CSeq).method()=INVITE;
         return msg;
      }
      
      static boost::shared_ptr<SipMessage>
      makeAck(boost::shared_ptr<SipMessage> msg)
      {
         if(msg->isRequest())
         {
            msg->header(h_RequestLine).method()=ACK;
         }
         msg->header(h_CSeq).method()=ACK;
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      fiddleBranchCase(boost::shared_ptr<SipMessage> msg)
      {
         resip_assert(msg->isResponse());
         // Yes, this is evil. However, BranchParameter does not expose any API
         // for doing this evil, evil thing.
         Data& branch = *const_cast<Data*>(&(msg->header(h_Vias).front().param(p_branch).getTransactionId()));
         for(size_t i=0;i<branch.size();++i)
         {
            if(isupper(branch[i]))
            {
               branch[i] = tolower(branch[i]);
            }
            else if(islower(branch[i]))
            {
               branch[i] = toupper(branch[i]);
            }
         }
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      upperBranchCase(boost::shared_ptr<SipMessage> msg)
      {
         // Yes, this is evil. However, BranchParameter does not expose any API
         // for doing this evil, evil thing.
         Data& branch = *const_cast<Data*>(&(msg->header(h_Vias).front().param(p_branch).getTransactionId()));
         for(size_t i=0;i<branch.size();++i)
         {
            branch[i] = toupper(branch[i]);
         }
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      removeProxyVias(boost::shared_ptr<SipMessage> msg)
      {
         resip_assert(msg->header(h_Vias).size()>1);
         resip_assert(msg->isResponse());
         Via top=msg->header(h_Vias).front();
         msg->remove(h_Vias);
         msg->header(h_Vias).push_back(top);
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      addProxyVia(boost::shared_ptr<SipMessage> msg)
      {
         resip_assert(msg->header(h_Vias).size()>1);
         resip_assert(msg->isResponse());
         Via top=msg->header(h_Vias).front();
         msg->header(h_Vias).pop_front();
         msg->header(h_Vias).push_front( Via() );
         msg->header(h_Vias).push_front( top );
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      corruptProxyBranch(boost::shared_ptr<SipMessage> msg)
      {
         resip_assert(msg->header(h_Vias).size()>1);
         resip_assert(msg->isResponse());
         // Yes, this is evil. However, BranchParameter does not expose any API
         // for doing this evil, evil thing.
         Vias::iterator i = msg->header(h_Vias).begin();
         ++i;
         Data& branch = *const_cast<Data*>(&(i->param(p_branch).getTransactionId()));
         char* buf = new char[branch.size()];
         memset(buf,'f',branch.size());
         branch=Data(Data::Take,buf,branch.size());
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      removeTo(boost::shared_ptr<SipMessage> msg)
      {
         msg->remove(h_To);
         return msg;
      }

   void refreshRegistration()
   {
      Seq
      (
         jason->registerUser(120, jason->getDefaultContacts()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 120)),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testProxyAlive()
   {
      WarningLog(<<"*!testProxyAlive!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())),
         derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
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

   /*!
      @brief Registration callflow with Record-Route in REGISTER.
      
      This tests whether the registrar will copy the Record-Route in the
      request into the 200. (This should not happen, see RFC 3261 sec 10.3)
      (Otherwise, this is a basic register callflow with auth)
   */
   void testRegisterWithRecordRoute()
   {
      WarningLog(<<"*!testRegisterWithRecordRoute!*");

      Seq(condition(addRecordRoute,jason->registerUser(60, jason->getDefaultContacts())),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, chain(new AssertNoRecordRoutes,condition(addRecordRoute,jason->digestRespond()))),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new AssertNoRecordRoutes),
          WaitForEndOfTest);
   
      ExecuteSequences();
   }
   
   /*!
      @brief Registration callflow with multiple contacts in the REGISTER.
      
      Testing the case where a UAC sends a REGISTER request with multiple
      contacts, is challenged with a 401, and digest responds. The contacts in
      the 200 are checked.
   */
   void testMultiple1()
   {
      WarningLog(<<"*!testMultiple1!*");

      set<NameAddr> contacts = mergeContacts(*jason, *derek);
         
      Seq(jason->registerUser(70, contacts),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(contacts, 70)),
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
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 3000)),
          WaitForEndOfTest);

      ExecuteSequences();
   }

   void testThirdPartyRegistration()
   {
      WarningLog(<<"*!testThirdPartyRegistration!*");
      set<NameAddr> contacts;
      contacts.insert(*(derek->getDefaultContacts().begin()));
      Seq(jason->registerUser(3000,contacts),
          jason->expect(REGISTER/407, from(proxy),1000, jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),5000, new CheckContacts(contacts,3000)),
          WaitForEndOfTest);

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
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 68)),
          WaitForEndOfTest);

      ExecuteSequences();
   }

   //*********Rainy day cases**********//

   void testRegister407Dropped()
   {
      WarningLog(<<"*!testRegister407Dropped!*");

      boost::shared_ptr<SipMessage> reg;
      Seq(save(reg,jason->registerUser(60,jason->getDefaultContacts())),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->retransmit(reg)),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfTest);

      ExecuteSequences();
   }
   
   void testRegisterBogusAuth()
   {
      WarningLog(<<"*!testRegisterBogusAuth!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, condition(bogusAuth,jason->digestRespond())),
          jason->expect(REGISTER/403, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testRegisterBadNonce()
   {
      WarningLog(<<"*!testRegisterBadNonce!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, condition(badNonce,jason->digestRespond())),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }
   
   void testRegisterLateDigestResponse()
   {
      WarningLog(<<"*!testRegisterLateDigestResponse!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, chain(jason->pause(3000000),jason->digestRespond())),
          jason->expect(REGISTER/407, from(proxy), 3000000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60) ),
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
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfTest);
      ExecuteSequences();
   }

   void testRegisterNoUserInTo()
   {
      WarningLog(<<"*!testRegisterNoUserInTo!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()), //There would be a condition noUserInTo here, but that would corrupt the registration message that gets reused in every test. (This is broken behavior on tfm's part)
          jason->expect(REGISTER/407,from(proxy), WaitForResponse, condition(noUserInTo,jason->digestRespond())),
          jason->expect(REGISTER/403,from(proxy),WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testRegisterUserInReqUri()
   {
      WarningLog(<<"*!testRegisterUserInReqUri!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy), WaitForResponse, condition(userInReqUri,jason->digestRespond())),
          jason->expect(REGISTER/400,from(proxy),WaitForResponse,jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testRegisterUnknownAorHost()
   {
      WarningLog(<<"*!testRegisterUnknownAorHost!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownHostInTo,jason->digestRespond())),
          jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testRegisterUnknownAorUser()
   {
      WarningLog(<<"*!testRegisterUnknownAorUser!*");
      
      Seq(jason->registerUser(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
          jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   void testOversizeCallIdRegister()
   {
      WarningLog(<<"*!testOversizeCallIdRegister!*");
        
      Seq(condition(largeCallId, jason->registerUser(60, jason->getDefaultContacts())),
          WaitForEndOfTest);

      ExecuteSequences();
   }

   void testOversizeContactRegister()
   {
      WarningLog(<<"*!testOversizeContactRegister!*");

      Seq(condition(largeContact, jason->registerUser(60, jason->getDefaultContacts())),
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
          WaitForEndOfTest);
      ExecuteSequences();
         
      sleepSeconds(1);

      NameAddr con = *(derek->getDefaultContacts().begin());
      con.param(p_expires) = 6000;

      set<NameAddr> contacts;
      contacts.insert(con);

      Seq(derek->registerUser(72,contacts),
          optional(derek->expect(REGISTER/407,from(proxy),WaitForResponse, derek->digestRespond())),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(contacts, 72)),
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
      
      Seq(jason->registerUser(60,derek->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),60)),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      Seq(jason->registerUser(120,derek->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),120)),
          WaitForEndOfSeq);
      
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
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsBefore, 73)),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUser(74, contactsAfter),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsAfter, 74)),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUser(74, withoutQ),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(withoutQ, 74)),
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
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 79)),
          WaitForEndOfTest);
      ExecuteSequences();

      NameAddr con = *(derek->getDefaultContacts().begin());
      con.param(p_expires) = 0;

      set<NameAddr> contacts;
      contacts.insert(con);

      set<NameAddr> emptySet;
      Seq(derek->registerUser(0, contacts),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
          WaitForEndOfTest);
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
          WaitForEndOfTest);

      ExecuteSequences();

      Seq(jason->registerUser(0,contacts),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(emptySet,0)),
          WaitForEndOfTest);
      
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
      
      Seq(derek->registerUser(0, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 4*Seconds + WaitForResponse, new CheckContacts(emptySet, 0)),
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
      
      Seq(condition(unknownUserInTo,jason->registerUser(0,jason->getDefaultContacts())),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
          jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
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
          WaitForEndOfTest);
      ExecuteSequences();
      
      set<NameAddr> nullSet;
      Seq(jason->registerUser(76, nullSet),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(contacts)),
          WaitForEndOfTest);
      ExecuteSequences();
   }

   void testExpiryCleanup()
   {
      WarningLog(<<"*!testExpiryCleanup!*");

      Seq(derek->registerUser(10, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 60)),
          WaitForEndOfTest);

      ExecuteSequences();
      sleepSeconds(12);
      // !jf! cause registration bindings to expire here
      
      set<NameAddr> emptySet;
      Seq(derek->registerUser(78, emptySet),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 78)),
          WaitForEndOfTest);
      ExecuteSequences();
   }     


//*********Rainy day cases**********//

   void testFetchNonExistent()
   {         
      WarningLog(<<"*!testFetchNonExistent!*");

      set<NameAddr> nullSet;
      Seq(condition(unknownUserInTo,jason->registerUser(76, nullSet)),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
          jason->expect(REGISTER/404, from(proxy), 10000, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }


//*****************************Registrar tests (with outbound)********************************//

//***********************New Registration Cases******************//

//*********Sunny day cases**********//


   /*!
      @brief Basic registration callflow, with auth.
      
      This tests the callflow where a single UAC sends REGISTER, is challenged
      with a 401, and then responds to the challenge. The contacts returned in
      the 200 are checked.
   */
   void testRegisterBasicWithOutbound()
   {
      WarningLog(<<"*!testRegisterBasicWithOutbound!*");
         
      Seq(jason->registerUserWithOutbound(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfTest);

      ExecuteSequences();
   }

   /*!
      @brief Registration callflow with Record-Route in REGISTER.
      
      This tests whether the registrar will copy the Record-Route in the
      request into the 200. (This should not happen, see RFC 3261 sec 10.3)
      (Otherwise, this is a basic register callflow with auth)
   */
   void testRegisterWithRecordRouteWithOutbound()
   {
      WarningLog(<<"*!testRegisterWithRecordRouteWithOutbound!*");

      Seq(condition(addRecordRoute,jason->registerUserWithOutbound(60, jason->getDefaultContacts())),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, chain(new AssertNoRecordRoutes,condition(addRecordRoute,jason->digestRespond()))),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new AssertNoRecordRoutes),
          WaitForEndOfTest);
   
      ExecuteSequences();
   }
   
   /*!
      @brief Registration callflow with multiple contacts in the REGISTER.
      
      Testing the case where a UAC sends a REGISTER request with multiple
      contacts, is challenged with a 401, and digest responds. The contacts in
      the 200 are checked.
   */
   void testMultiple1WithOutbound()
   {         
      WarningLog(<<"*!testMultiple1WithOutbound!*");

      set<NameAddr> contacts = mergeContacts(*jason, *derek);
         
      Seq(jason->registerUserWithOutbound(70, contacts),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(contacts, 70)),
          WaitForEndOfTest);

      ExecuteSequences();
   }
      
   /*!
      @brief Registration with multiple contacts and expiries.
        
      Callflow with two contacts in REGISTER; one of which has an expires
      parameter of 60000, the other of which has no expires param. The
      REGISTER also includes an Expires header of 3000. This checks to see
      whether the registrar will correctly assign an expiry of 60000 to the
      first contact, and 3000 to the second. This callflow includes auth.
   */
   void testMixedExpiresWithOutbound()
   {
      WarningLog(<<"*!testMixedExpiresWithOutbound!*");

      NameAddr cond = *(jason->getDefaultContacts().begin());
      NameAddr conk = *(derek->getDefaultContacts().begin());
      cond.param(p_expires) = 60000;
        
      set<NameAddr> contacts;
      contacts.insert(cond);
      contacts.insert(conk);

      Seq(jason->registerUserWithOutbound(3000, contacts),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 3000)),
          WaitForEndOfTest);
         
      ExecuteSequences();
   }

   /*!
      @brief Third-party registration callflow.
      
      This callflow features a UAC registering a contact that does not point to
      it. (ie, the address in the contact is not the same as the address the
      UAC is attached to). This callflow features auth.
   */
   void testThirdPartyRegistrationWithOutbound()
   {
      WarningLog(<<"*!testThirdPartyRegistrationWithOutbound!*");
      set<NameAddr> contacts;
      contacts.insert(*(derek->getDefaultContacts().begin()));
      Seq
      (
         jason->registerUserWithOutbound(3000,contacts),
         jason->expect(REGISTER/407, from(proxy),1000, jason->digestRespond()),
         jason->expect(REGISTER/200,from(proxy),5000, new CheckContacts(contacts,3000)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   /*!
      @brief Registration callflow with q-value parameter.
         
      This callflow tests whether the registrar will store a q-value param
      on a contact. (in the 200, we check to make sure the correct q-value
      paramter is present) This callflow features auth.
   */
   void testDetailsQValueWithOutbound()
   {
      WarningLog(<<"*!testDetailsQValueWithOutbound!*");
        
      NameAddr con = *(jason->getDefaultContacts().begin());
         
      set<NameAddr> contacts;
      contacts.insert(con);
                  
      Seq(jason->registerUserWithOutbound(67, con),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(contacts, 67)),
          WaitForEndOfTest);
         
      ExecuteSequences();
   }

   /*!
      @brief Callflow with conflicting expiry values.
         
      The REGISTER includes a single contact with expires param 60000, and
      an Expires header of 68. We test to ensure that the expires param is
      honored, and the Expires header is ignored. This callflow features
      auth.
   */
   void testDetailsExpiresWithOutbound()
   {
      WarningLog(<<"*!testDetailsExpiresWithOutbound!*");

      NameAddr con = *(jason->getDefaultContacts().begin());
      con.param(p_expires) = 60000;
         
      set<NameAddr> contacts;
      contacts.insert(con);
         
      Seq(jason->registerUserWithOutbound(68, con),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 68)),
          WaitForEndOfTest);
         
      ExecuteSequences();
   }

//*********Rainy day cases**********//

   /*!
      @brief Registration callflow where the first 401 is dropped.
      
      This tests the case where the first 401 does not arrive at the UAC,
      prompting the UAC to retransmit.
      This scenario features auth.
   */
   void testRegister407DroppedWithOutbound()
   {
      WarningLog(<<"*!testRegister407DroppedWithOutbound!*");
      
      boost::shared_ptr<SipMessage> reg;
      Seq
      (
         save(reg,jason->registerUserWithOutbound(60,jason->getDefaultContacts())),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->retransmit(reg)),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   /*!
      @brief Registration with bad digest response.
      
      This callflow tests the case where the UAC sends an incorrect response to
      the digest challenge. We expect the registrar to respond with a 403.
      This scenario features auth.
   */
   void testRegisterBogusAuthWithOutbound()
   {
      WarningLog(<<"*!testRegisterBogusAuthWithOutbound!*");
      
      Seq
      (
         jason->registerUserWithOutbound(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, condition(bogusAuth,jason->digestRespond())),
         jason->expect(REGISTER/403, from(proxy), WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   /*!
      @brief Registration with an extremely late digest response.
      
      This tests the case where the UAC does not digest respond until the
      nonce has expired. (We expect the registrar to issue a new challenge)
      This test is not run by default, since it takes a very long time for
      a nonce to expire.
      This scenario features auth.
   */
   void testRegisterLateDigestResponseWithOutbound()
   {
      WarningLog(<<"*!testRegisterLateDigestResponseWithOutbound!*");
      
      Seq
      (
         jason->registerUserWithOutbound(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/407, from(proxy), WaitForResponse, chain(jason->pause(3000000),jason->digestRespond())),
         jason->expect(REGISTER/407, from(proxy), 3000000, jason->digestRespond()),
         jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60) ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   /*!
      @brief Registration scenario with retransmission of digest response.
         
      This tests the scenario where the UAC retransmits its digest response.
      This scenario features auth.
   */
   void testRegisterClientRetransmitsWithOutbound()
   {
      WarningLog(<<"*!testRegisterClientRetransmitsWithOutbound!*");

      boost::shared_ptr<SipMessage> reg;
      Seq(save(reg, jason->registerUserWithOutbound(60, jason->getDefaultContacts())),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->retransmit(reg)),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfTest);
      ExecuteSequences();
   }

   /*!
      @brief Registration scenario with no userpart in the To header uri.
      
      This tests the case where the AOR is of the form sip:hostname. A
      registrar is supposed to reject this with a 403.
      This scenario features auth.
   */
   void testRegisterNoUserInToWithOutbound()
   {
      WarningLog(<<"*!testRegisterNoUserInToWithOutbound!*");
      
      Seq(jason->registerUserWithOutbound(60,jason->getDefaultContacts()), //There would be a condition noUserInTo here, but that would corrupt the registration message that gets reused in every test. (This is broken behavior on tfm's part)
          jason->expect(REGISTER/407,from(proxy), WaitForResponse, condition(noUserInTo,jason->digestRespond())),
          jason->expect(REGISTER/403,from(proxy),WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Registration scenario with a userpart in the Request-Uri.
      
      This tests the case where the Request-Uri in a REGISTER contains a
      userpart. (UACs are not supposed to do this, but there is no language
      forcing registrars to reject this request)
      This scenario features auth.
   */
   void testRegisterUserInReqUriWithOutbound()
   {
      WarningLog(<<"*!testRegisterUserInReqUriWithOutbound!*");
      
      Seq(jason->registerUserWithOutbound(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy), WaitForResponse, condition(userInReqUri,jason->digestRespond())),
          jason->expect(REGISTER/400,from(proxy),WaitForResponse,jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Registration scneario where the AOR contains a host that the
      registrar is not authoritative for.
      
      This tests the case where the To: header (ie, the AOR) contains a
      hostpart that does not refer to the registrar. A registrar is supposed
      to reject this with a 404. (See RFC 3261 sec 10.3 bullet 5)
      This scenario features auth.
   */
   void testRegisterUnknownAorHostWithOutbound()
   {
      WarningLog(<<"*!testRegisterUnknownAorHostWithOutbound!*");
      
      Seq(jason->registerUserWithOutbound(60,jason->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownHostInTo,jason->digestRespond())),
          jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Registration with unknown user in AOR.
      
      This tests the case were the To: header (ie, AOR) contains a user part
      that is not provisioned at the registrar. The registrar is supposed to
      reject this with a 404.
      This scenario features auth.
   */
   void testRegisterUnknownAorUserWithOutbound()
   {
      WarningLog(<<"*!testRegisterUnknownAorUserWithOutbound!*");
      
      Seq(jason->registerUserWithOutbound(60,jason->getDefaultContacts()),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
         jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Registration scenario with an over-large CallId.
      
      This tests the case where the REGISTER request conatins a CallId that
      is 4096 characters long. We expect the registrar to reject this 
      with a 400, although there is no language that mandates this.
      This scenario features auth.
   */
   void testOversizeCallIdRegisterWithOutbound()
   {
      WarningLog(<<"*!testOversizeCallIdRegisterWithOutbound!*");
      
      Seq(condition(largeCallId, jasonTcp->registerUserWithOutbound(60, jasonTcp->getDefaultContacts())),
          WaitForEndOfTest);

      ExecuteSequences();
   }

   /*!
      @brief Registration scenario with an over-large contact.
      
      This tests the case where the REGISTER contains a Contact that is
      4096 characters in length. We expect the regstrar to reject this
      request with a 500, but there is no language mandating this behavior.
      This scenario features auth.
   */
   void testOversizeContactRegisterWithOutbound()
   {
      WarningLog(<<"*!testOversizeContactRegisterWithOutbound!*");

      Seq(condition(largeContact, jasonTcp->registerUserWithOutbound(60, jasonTcp->getDefaultContacts())),
          WaitForEndOfTest);

      ExecuteSequences();
   }

//*******************Refresh type tests*******************/

//*********Sunny day cases**********//


   /*!
      @brief Basic registration refresh scenario.
      
      This tests the case where the UAC is refreshing an existing
      registration. The contacts in the 200 to both the first registration
      and the refresh are checks. This scenario features auth.
   */
   void testRefreshWithOutbound()
   {
      WarningLog(<<"*!testRefreshWithOutbound!*");

      Seq(derek->registerUserWithOutbound(71, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 71)),
          WaitForEndOfTest);
      ExecuteSequences();
      
      sleepSeconds(1);

      NameAddr con = *(derek->getDefaultContacts().begin());
      con.param(p_expires) = 6000;

      set<NameAddr> contacts;
      contacts.insert(con);

      Seq(derek->registerUserWithOutbound(72,contacts),
         optional(derek->expect(REGISTER/407,from(proxy),WaitForResponse, derek->digestRespond())),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(contacts, 72)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Registration refresh scenario with multiple contacts.
      
      This tests the case where the UAC refreshes multiple contacts in the same
      REGISTER request. (These contacts are initially registered in a single
      request as well) The contacts returned in the 200 are checked. This 
      scenario features auth.
   */
   void testRefreshMultiWithOutbound()
   {
      WarningLog(<<"*!testRefreshMultiWithOutbound!*");
   
      set<NameAddr> contacts = mergeContacts(*jason, *derek);

      Seq(jason->registerUserWithOutbound(60,contacts),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(contacts,60)),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      sleepSeconds(1);
      
      Seq(jason->registerUserWithOutbound(120,contacts),
          optional(jason->expect(REGISTER/407,from(proxy),WaitForResponse, jason->digestRespond())),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse, new CheckContacts(contacts,120)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Third-party registration refresh scenario.
      
      This tests the case where a third party refreshes a registration that it
      established earlier. This scenario features auth.
   */
   void testRefreshThirdPartyWithOutbound()
   {
      WarningLog(<<"*!testRefreshThirdPartyWithOutbound!*");
      
      Seq(jason->registerUserWithOutbound(60,derek->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),60)),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      Seq(jason->registerUserWithOutbound(120,derek->getDefaultContacts()),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(derek->getDefaultContacts(),120)),
          WaitForEndOfSeq);
      
      ExecuteSequences();
   }

   /*!
      @brief Registration refresh scenario with a change in q-value.
      
      This tests the case where the q-value of a binding is changed when
      the UAC refreshes. This scenario features auth.
   */
   void testChangeQValueWithOutbound()
   {         
      WarningLog(<<"*!testChangeQValueWithOutbound!*");

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
      
      Seq(jason->registerUserWithOutbound(73, contactsBefore),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsBefore, 73)),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUserWithOutbound(74, contactsAfter),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contactsAfter, 74)),
          WaitForEndOfTest);
      ExecuteSequences();

      Seq(jason->registerUserWithOutbound(74, withoutQ),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(withoutQ, 74)),
          WaitForEndOfTest);
      ExecuteSequences();
   }

//*********Rainy day cases**********//

//*******************Unregister type tests*******************/

//*********Sunny day cases**********//

   /*!
      @brief Basic unregister scenario.
      
      This tests the case where a UAC unregisters a specific contact. This
      scenario features auth.
   */
   void testSetThenRemoveSpecificWithOutbound()
   {
      WarningLog(<<"*!testSetThenRemoveSpecificWithOutbound!*");

      Seq(derek->registerUserWithOutbound(79, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 79)),
          WaitForEndOfTest);
      ExecuteSequences();

      NameAddr con = *(derek->getDefaultContacts().begin());
      con.param(p_expires) = 0;

      set<NameAddr> contacts;
      contacts.insert(con);

      set<NameAddr> emptySet;
      Seq(derek->registerUserWithOutbound(0, contacts),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 60)),
          WaitForEndOfTest);
      ExecuteSequences();
   }

   /*!
      @brief Unregister scenario with multiple contacts.
      
      This tests the case where the UAC unregisters multiple contacts. This
      is not done with the Contact: * style de-registration. This scenario
      features auth.
   */
   void testUnregisterMultiWithOutbound()
   {
      WarningLog(<<"*!testUnregisterMultiWithOutbound!*");
      
      set<NameAddr> contacts = mergeContacts(*jason, *derek);
      set<NameAddr> emptySet;
      
      Seq(jason->registerUserWithOutbound(70, contacts),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(contacts, 70)),
          WaitForEndOfTest);

      ExecuteSequences();

      Seq(jason->registerUserWithOutbound(0,contacts),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,jason->digestRespond()),
          jason->expect(REGISTER/200,from(proxy),WaitForResponse,new CheckContacts(emptySet,0)),
          WaitForEndOfTest);
      
      ExecuteSequences();
   
   }

   /*!
      @brief Unregistration of a contact that has already expired.
      
      This tests the case where the UAC unregisters a contact that has already
      expired. We expect a 200 from the registrar. This scenario features auth.
   */
   void testUnregisterExpiredWithOutbound()
   {
      WarningLog(<<"*!testUnregisterExpiredWithOutbound!*");
      
      set<NameAddr> emptySet;
      Seq(derek->registerUserWithOutbound(5, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      sleepSeconds(7);
      
      Seq(derek->registerUserWithOutbound(0, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 4*Seconds + WaitForResponse, new CheckContacts(emptySet, 0)),
          WaitForEndOfTest);
          
      ExecuteSequences();
   }

//*********Rainy day cases**********//

   /*!
      @brief Unregister all (Contact: *) with non-zero Expires header.
      
      This tests the case where the UAC sends a REGISTER request with a
      Contact: *, but a non-zero Expires header. The registrar is expected to
      reject with a 400. This scenario features auth.
   */
   void testUnregisterAllBadExpiresWithOutbound()
   {
      WarningLog(<<"*!testUnregisterAllBadExpiresWithOutbound!*");
      
      NameAddr na;
      set<NameAddr> all;
      set<NameAddr> emptySet;
      
      na.setAllContacts();
      all.insert( na );
      
      Seq(jason1->registerUserWithOutbound(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckContacts(jason1->getDefaultContacts(),60)),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason2->registerUserWithOutbound(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForResponse, new CheckFetchedContacts( mergeContacts(*jason1, *jason2) )),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq(jason1->registerUserWithOutbound(10, all ),
          jason1->expect(REGISTER/407,from(proxy),WaitForResponse,jason1->digestRespond()),
          jason1->expect(REGISTER/400, from(proxy), WaitForResponse, jason1->noAction()),
          WaitForEndOfTest);
      
      ExecuteSequences();
   }

   /*!
      @brief Unregistration of a non-existent AOR.
      
      This tests the case where a UAC attempts to unregister a contact for an
      AOR that doesn't exist. The registrar is expected to reject with a 404.
      This scenario features auth.
   */
   void testUnregisterNonExistentWithOutbound()
   {
      WarningLog(<<"*!testUnregisterNonExistentWithOutbound!*");
      
      Seq
      (
         condition(unknownUserInTo,jason->registerUserWithOutbound(0,jason->getDefaultContacts())),
         jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
         jason->expect(REGISTER/404,from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
       
       
//*******************Fetch type tests*******************/

//*********Sunny day cases**********//

   /*!
      @brief Basic registration fetch scenario.
      
      Tests the case where the UAC fetches the current registrations on an AOR.
      This scenario features auth.
   */
   void testFetchWithOutbound()
   {         
      WarningLog(<<"*!testFetchWithOutbound!*");
      set<NameAddr> contacts = mergeContacts(*jason, *derek);
      
      Seq(jason->registerUserWithOutbound(75, contacts),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 5000, new CheckContacts(contacts, 75)),
          WaitForEndOfTest);
      ExecuteSequences();
      
      set<NameAddr> nullSet;
      Seq(jason->registerUserWithOutbound(76, nullSet),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 10000, new CheckFetchedContacts(contacts)),
          WaitForEndOfTest);
      ExecuteSequences();
   }

   /*!
      @brief Registration fetch of expired contact.
      
      This tests that contacts are expiring as they should be, by fetching
      a contact after waiting long enough for it to expire. This scenario
      features auth.
   */
   void testExpiryCleanupWithOutbound()
   {
      WarningLog(<<"*!testExpiryCleanupWithOutbound!*");

      Seq(derek->registerUserWithOutbound(10, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(derek->getDefaultContacts(), 60)),
          WaitForEndOfTest);

      ExecuteSequences();
      sleepSeconds(12);
      // !jf! cause registration bindings to expire here
      
      set<NameAddr> emptySet;
      Seq(derek->registerUserWithOutbound(78, emptySet),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 10000, new CheckContacts(emptySet, 78)),
          WaitForEndOfTest);
      ExecuteSequences();
   }     

//*********Rainy day cases**********//

   /*!
      @brief Registration fetch on non-existent AOR.
      
      This tests the case where the UAC attempts to fetch contacts for an AOR
      that does not exist. This scenario features auth.
   */
   void testFetchNonExistentWithOutbound()
   {         
      WarningLog(<<"*!testFetchNonExistentWithOutbound!*");

      set<NameAddr> nullSet;
      Seq(condition(unknownUserInTo,jason->registerUserWithOutbound(76, nullSet)),
          jason->expect(REGISTER/407,from(proxy),WaitForResponse,condition(unknownUserInTo,jason->digestRespond())),
          jason->expect(REGISTER/404, from(proxy), 10000, jason->noAction()),
          WaitForEndOfTest);
      
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

   void testInviteBasicUpperCaseBranch()
   {
      WarningLog(<<"*!testInviteBasicUpperCaseBranch!*");
      Seq(condition(upperBranchCase,derek->registerUser(60, derek->getDefaultContacts())),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, condition(upperBranchCase,derek->digestRespond())),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(condition(upperBranchCase,jason->invite(*derek)),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), condition(upperBranchCase,jason->digestRespond()))),
          And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
              Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                  jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
                  jason->expect(INVITE/200, contact(derek), WaitForResponse, condition(upperBranchCase,jason->ack())),
                  derek->expect(ACK, from(jason), WaitForResponse, jason->noAction()))),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

   void testInviteBasicTls()
   {
      WarningLog(<<"*!testInviteBasicTls!*");
      
      Seq(derekTls->registerUserWithOutbound(60, derekTls->getDefaultContacts()),
          derekTls->expect(REGISTER/407, from(proxy), WaitForResponse, derekTls->digestRespond()),
          derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, derekTls->noAction()),
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      Seq(jasonTls->inviteWithOutbound(*derekTls),
          optional(jasonTls->expect(INVITE/100, from(proxy), WaitFor100, jasonTls->noAction())),
          jasonTls->expect(INVITE/407, from(proxy), WaitForResponse, chain(jasonTls->ack(), jasonTls->digestRespond())),
          And(Sub(optional(jasonTls->expect(INVITE/100, from(proxy), WaitFor100, jasonTls->noAction()))),
              Sub(derekTls->expect(INVITE, contact(jasonTls), WaitForCommand, chain(derekTls->ring(), derekTls->answer())),
                  jasonTls->expect(INVITE/180, from(derekTls), WaitFor100, jasonTls->noAction()),
                  jasonTls->expect(INVITE/200, contact(derekTls), WaitForResponse, jasonTls->ack()),
                  derekTls->expect(ACK, from(jasonTls), WaitForResponse, jasonTls->noAction()))),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInviteBasicRedirect()
   {
      WarningLog(<<"*!testInviteBasicRedirect!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/302,from(proxy),WaitForResponse,chain(new CheckContacts(derek->getDefaultContacts(),0),jason->ack())),
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

      ExecuteSequences();

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
      ExecuteSequences();

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
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
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
      ExecuteSequences();

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
                  And(Sub(jason->expect(ACK, from(proxy), WaitForAck, jason->noAction())),
                      Sub(derek->expect(INVITE/486, from(proxy), WaitForResponse, derek->ack()))))),
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
              Sub(david->expect(INVITE, contact(derek), WaitForCommandSpiral, chain(david->ring(), david->answer())),
                  derek->expect(INVITE/180, from(david), WaitFor180Spiral, derek->noAction()),
                  derek->expect(INVITE/200, contact(david), WaitForResponseSpiral, derek->ack()),
                  david->expect(ACK, from(derek), WaitForResponseSpiral, derek->noAction()))),
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

      Seq(derek->invite(proxy->makeUrl("9spiral").uri()),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),derek->digestRespond())),
          And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                  derek->expect(INVITE/180, from(david), WaitFor180Spiral, derek->cancel()),
                  derek->expect(CANCEL/200, from(proxy),WaitForResponseSpiral, derek->noAction()),
                  derek->expect(INVITE/487, from(david),WaitForResponseSpiral, derek->ack())),
              Sub(david->expect(INVITE, contact(derek), WaitForCommandSpiral, david->ring()),
                  david->expect(CANCEL, from(proxy), WaitForResponseSpiral, chain(david->ok(),david->send487())),
                  david->expect(ACK, from(proxy),WaitForResponse,david->noAction()))),
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
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq(david->invite(*derek),
          optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
          david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
          
          And(Sub(optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction()))),
              Sub(derek->expect(INVITE, contact(david), WaitForCommand, derek->ring()),
                  david->expect(INVITE/180, from(derek), WaitFor180, chain(derek->ok(), derek->pause(25), david->cancel())),
                  And(Sub(david->expect(CANCEL/200, from(proxy), WaitForCommand, derek->noAction())),
                      Sub(david->expect(INVITE/200, contact(derek), WaitForCommand, david->ack()),
                          derek->expect(ACK, contact(david), WaitForAck, derek->noAction()))))),
          WaitForEndOfTest);    
      
      ExecuteSequences();  
   }

   void testInvite200BeatsCancelClientSide()
   {
      WarningLog(<<"*!testInvite200BeatsCancelClientSide!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
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
         WaitForEndOfSeq
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
          WaitForEndOfSeq);
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
               jason->expect(INVITE/180,contact(derek),WaitForResponse+500,jason->noAction()),
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
         WaitForEndOfSeq
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

   void testInviteCancelBefore1xx()
   {
      WarningLog(<<"*!testInviteCancelBefore1xx!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
         WaitForEndOfSeq
      );
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
               jason->expect(INVITE/100,from(proxy),WaitForResponse, chain(jason->noAction(), jason->pause(50),jason->cancel())),
               jason->expect(CANCEL/200,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,chain(derek->pause(1200), derek->send100()))
            )
         ),

         derek->expect(CANCEL,from(proxy),WaitForCommand+1200,chain(derek->ok(),derek->send487())),
         And
         (
            Sub
            (
               derek->expect(ACK, from(proxy),WaitForCommand,derek->noAction())
            ),
            Sub
            (
               jason->expect(INVITE/487,contact(derek),WaitForResponse,jason->ack())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInviteCancelBefore1xxNo487()
   {
      WarningLog(<<"*!testInviteCancelBefore1xxNo487!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
         WaitForEndOfSeq
      );
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
               jason->expect(INVITE/100,from(proxy),WaitForResponse,chain(jason->pause(100),jason->cancel())),
               jason->expect(CANCEL/200,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,chain(derek->pause(1200),derek->send100()))
            )
         ),

         derek->expect(CANCEL,from(proxy),WaitForCommand+1200,chain(derek->ok(),derek->noAction())),
         jason->expect(INVITE/408,from(proxy),64100,jason->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInviteCancelBefore1xxNo487or1xx()
   {
      WarningLog(<<"*!testInviteCancelBefore1xxNo487or1xx!*");

      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
         derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
         WaitForEndOfSeq
      );
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
               jason->expect(INVITE/100,from(proxy),WaitForResponse,chain(jason->pause(50),jason->cancel())),
               jason->expect(CANCEL/200,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(jason),WaitForCommand,derek->noAction())
            )
         ),

         jason->expect(INVITE/408,from(proxy),32100,jason->ack()),
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
          WaitForEndOfSeq);
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
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitForResponse,derek->noAction())),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,from(proxy),WaitForResponse,derek->ack()),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->ring(),jason->ring(),jason->answer()))
            )
         ),
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
   
   void testInviteBadNonce()
   {
      WarningLog(<<"*!testInviteBadNonce!*");
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), condition(badNonce,jason->digestRespond()))),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/407,from(proxy),WaitForResponse,chain(jason->ack(), jason->digestRespond())),
          And
          (
            Sub
            (
               optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
               jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
               jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
               derek->expect(ACK, from(jason), WaitForResponse, jason->noAction())
            )
         ),
         WaitForEndOfTest
      
      );
      ExecuteSequences();  
   }
   
   void testInviteUDPToTCPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTCPCallerHangsUp!*");
      Seq
      (
         jasonTcp->registerUserWithOutbound(60, jasonTcp->getDefaultContacts()),
         jasonTcp->expect(REGISTER/407,from(proxy),1000,jasonTcp->digestRespond()),
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
   }

   void testInviteUDPToTCPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTCPCalleeHangsUp!*");
      Seq
      (
         jasonTcp->registerUserWithOutbound(60, jasonTcp->getDefaultContacts()),
         jasonTcp->expect(REGISTER/407,from(proxy),1000,jasonTcp->digestRespond()),
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
   }

   void testInviteTCPToUDPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToUDPCallerHangsUp!*");
      Seq
      (
         derek->registerUserWithOutbound(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407,from(proxy),1000,derek->digestRespond()),
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
   }
   
   void testInviteTCPToUDPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToUDPCalleeHangsUp!*");
      Seq
      (
         derek->registerUserWithOutbound(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTcp->inviteWithOutbound(*derek),
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
   }
   
   void testInviteUDPToTLSCallerHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTLSCallerHangsUp!*");
      Seq
      (
         derekTls->registerUserWithOutbound(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/407,from(proxy),1000,derekTls->digestRespond()),
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
   }
   
   void testInviteUDPToTLSCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteUDPToTLSCalleeHangsUp!*");
      Seq
      (
         derekTls->registerUserWithOutbound(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/407,from(proxy),1000,derekTls->digestRespond()),
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
   }
   
   void testInviteTCPToTLSCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToTLSCallerHangsUp!*");
      Seq
      (
         derekTls->registerUserWithOutbound(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/407,from(proxy),1000,derekTls->digestRespond()),
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
   }
   
   void testInviteTCPToTLSCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTCPToTLSCalleeHangsUp!*");
      Seq
      (
         derekTls->registerUserWithOutbound(60, derekTls->getDefaultContacts()),
         derekTls->expect(REGISTER/407,from(proxy),1000,derekTls->digestRespond()),
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
   }
   
   void testInviteTLSToUDPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToUDPCallerHangsUp!*");
      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->inviteWithOutbound(*derek),
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
   }
   
   void testInviteTLSToUDPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToUDPCalleeHangsUp!*");
      Seq
      (
         derek->registerUser(60, derek->getDefaultContacts()),
         derek->expect(REGISTER/407,from(proxy),1000,derek->digestRespond()),
         derek->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derek->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->inviteWithOutbound(*derek),
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
      
   }
   
   void testInviteTLSToTCPCallerHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToTCPCallerHangsUp!*");
      Seq
      (
         derekTcp->registerUser(60, derekTcp->getDefaultContacts()),
         derekTcp->expect(REGISTER/407,from(proxy),1000,derekTcp->digestRespond()),
         derekTcp->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTcp->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->inviteWithOutbound(*derekTcp),
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
   }
   
   void testInviteTLSToTCPCalleeHangsUp()
   {
      WarningLog(<<"*!testInviteTLSToTCPCalleeHangsUp!*");
      Seq
      (
         derekTcp->registerUser(60, derekTcp->getDefaultContacts()),
         derekTcp->expect(REGISTER/407,from(proxy),1000,derekTcp->digestRespond()),
         derekTcp->expect(REGISTER/200,from(proxy),1000,new CheckContacts(derekTcp->getDefaultContacts(),60)),
         WaitForEndOfSeq
      );
      
      ExecuteSequences();
      
      Seq
      (
         jasonTls->inviteWithOutbound(*derekTcp),
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

   void testUnsupportedProxyRequire()
   {
      WarningLog(<<"*!testUnsupportedProxyRequire!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         condition(unknownProxyRequire,jason->invite(*derek)),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/420, from(proxy), WaitForResponse,jason->ack()),
         WaitForEndOfTest
      );
      ExecuteSequences();  
   }

   void testSupportedProxyRequire()
   {
      WarningLog(<<"*!testSupportedProxyRequire!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         condition(supportedProxyRequire,jason->invite(*derek)),
         optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
         jason->expect(INVITE/407, from(proxy), WaitForResponse,chain(jason->ack(), condition(supportedProxyRequire, jason->digestRespond()))),
          And
          (
            Sub
            (
               optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
               jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
               jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
               derek->expect(ACK, from(jason), WaitForResponse, jason->noAction())
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
          WaitForEndOfSeq);
      ExecuteSequences();
      Seq(derek->registerUser(600, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
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
          WaitForEndOfSeq);
      
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
                     jason->expect(ACK,contact(derek),WaitForCommand+1000,jason->noAction())                  
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
          WaitForEndOfSeq);
      
      ExecuteSequences();

      boost::shared_ptr<SipMessage> inv;
      
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),save(inv,derek->digestRespond()),derek->retransmit(inv),
         derek->retransmit(inv),
         derek->retransmit(inv))),
               
               And
               (
                  Sub //The meat of the call
                  (
                     jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())),
                     derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
                     derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
                     jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
                  ),
                  Sub //Soak up 100s
                  (
                     optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
                     optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
                     optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
          WaitForEndOfSeq);
      
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
   }

   void testInviteClientSpamsAck200()
   {
      WarningLog(<<"*!testInviteClientSpamsAck200!*");
      
      Seq(jason->registerUser(600, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfSeq);
      
      ExecuteSequences();

      boost::shared_ptr<SipMessage> ack;
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())))),
         derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack <= derek->ack(),derek->pause(200),derek->retransmit(ack),derek->pause(200),derek->retransmit(ack),derek->pause(200),derek->retransmit(ack))),
         // pauses above are required or we may trigger the following code block in TransactionState:
         // .bwc. While the resolver was attempting to find a target, another
         // request came down from the TU. This could be a bug in the TU, or 
         // could be a retransmission of an ACK/200. Either way, we cannot
         // expect to ever be able to send this request (nowhere to store it
         // temporarily).
         // DebugLog(<< "Received a second request from the TU for a transaction"
         //             " that already existed, before the DNS subsystem was done "
         //             "resolving the target for the first request. Either the TU"
         //             " has messed up, or it is retransmitting ACK/200 (the only"
         //             " valid case for this to happen)");
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
          WaitForEndOfSeq);
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
          WaitForEndOfSeq);
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
   }

   void testInviteServerRetransmits503()
   {
      WarningLog(<<"*!testInviteServerRetransmits503!*");
      
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForRegistration, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> error;

      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,from(proxy),WaitForCommand,error <= jason->send503()))),
         
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
          WaitForEndOfSeq);
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

   void testInviteNoDNSTcp()
   {
      WarningLog(<<"*!testInviteNoDNSTcp!*");
      
      RouteGuard dGuard(*proxy, "sip:.*@.*", "sip:foobar@dfkaslkfdas.com;transport=TCP");
      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/480, from(proxy), 5000, jason->ack()),
          WaitForEndOfTest);
      ExecuteSequences();  
   }
   
   void testInviteNoSuchUser()
   {
      WarningLog(<<"*!testInviteNoSuchUser!*");
      
      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), condition(userInReqUri,jason->digestRespond()))),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/404, from(proxy), WaitForResponse, jason->ack()),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

   void testInviteTransportFailure()
   {
      WarningLog(<<"*!testInviteTransportFailure!*");

      Seq(jasonTcp->registerUser(60, jasonTcp->getDefaultContacts()),
          jasonTcp->expect(REGISTER/407, from(proxy), WaitForResponse, jasonTcp->digestRespond()),
          jasonTcp->expect(REGISTER/200, from(proxy), WaitForResponse, jasonTcp->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq(chain(jasonTcp->closeTransport(), derek->invite(*jasonTcp)),
          optional(derek->expect(INVITE/100, from(proxy), 300+WaitFor100, derek->noAction())),
          derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),
                                                                        derek->digestRespond())),
          optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
          // Note:  Under windows when using WSAPoll - there is a bug in WSAPoll where it won't properly notify of
          //        TCP connection errors.  The implemented work around is to check the socket on the next send.
          //        This means that this test case will fail.
          derek->expect(INVITE/480, from(proxy), WaitForResponseSpiral, derek->ack()),  // Give more time than just WaitForResponse - TCP connection failure detection can take some time
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
         david->expect(INVITE/483,from(proxy),WaitForResponseLoop,david->ack()),
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
          WaitForEndOfSeq);
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
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
               derek->expect(INVITE/200,from(proxy), WaitForResponse, derek->ack()),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );    

      ExecuteSequences();  
   }

   void testInviteBadAckTid1()
   {
      WarningLog(<<"*!testInviteBadAckTid1!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
   
      ExecuteSequences();

      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
              Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                  jason->expect(INVITE/180, from(derek), WaitFor100, jason->noAction()),
                  jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ackOldTid()),
                  derek->expect(ACK, from(jason), WaitForResponse, jason->noAction()))),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

   void testInviteBadAckTid2()
   {
      WarningLog(<<"*!testInviteBadAckTid2!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
   
      ExecuteSequences();

      Seq(jason->invite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          And
          (
            Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
            Sub
            (
               derek->expect(INVITE, contact(jason), WaitForCommand, derek->ring()),
               And
               (
                  Sub
                  (
                     jason->expect(INVITE/180, from(derek), WaitFor100, derek->send486()),
                     jason->expect(INVITE/486, contact(derek), WaitForResponse, jason->ackNewTid()),
                     jason->expect(INVITE/486, contact(derek), 1100, jason->ackNewTid()),
                     jason->expect(INVITE/486, contact(derek), 2100, jason->ackNewTid()),
                     jason->expect(INVITE/486, contact(derek), 4100, jason->ackNewTid()),
                     jason->expect(INVITE/486, contact(derek), 4100, jason->ack())
                  ),
                  Sub
                  (
                     derek->expect(ACK, from(proxy), WaitForResponse, jason->noAction())
                  )
               )
            )
         ),
          WaitForEndOfTest);
      ExecuteSequences();  
   }

//****************** Oddball UAS ********************//

   void testInvite2543Tid()
   {
      WarningLog(<<"*!testInvite2543Tid!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      
      Seq
      (
         jason->invite(*derek),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100, jason->noAction())),
         jason->expect(INVITE/407,from(proxy),WaitForResponse,chain(jason->ack(),condition( missingTid,jason->digestRespond()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),WaitFor100, jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE,from(proxy),WaitForCommand,chain(derek->ring(), derek->ok())),
               jason->expect(INVITE/180,from(proxy),WaitForResponse,jason->noAction()),
               jason->expect(INVITE/200,from(proxy),WaitForResponse, jason->ack()),
               derek->expect(ACK,from(jason),WaitForCommand,derek->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNit2543Tid()
   {
      WarningLog(<<"*!testNit2543Tid!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         condition(missingTid,derek->message(*jason,"Ping")),
         jason->expect(MESSAGE, from(proxy), WaitForCommand, jason->send486()),
         derek->expect(MESSAGE/486, from(proxy),WaitForResponse, derek->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testAck2543Tid()
   {
      WarningLog(<<"*!testAck2543Tid!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,condition(missingTid,derek->ack())),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNonInvite2543Tid()
   {
      WarningLog(<<"*!testNonInvite2543Tid!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, condition(missingTid,jason->digestRespond())),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send486()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testInvite2543TidWithDigest()
   {
      WarningLog(<<"*!testInvite2543TidWithDigest!*");

      //Registering Derek
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), WaitForResponse, jason->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100, derek->noAction())),
         derek->expect(INVITE/407,from(proxy),WaitForResponse,chain(derek->ack(),derek->digestRespond())),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100, derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,from(proxy),WaitForCommand,chain(jason->ring(), jason->ok())),
               derek->expect(INVITE/180,from(proxy),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,from(proxy),WaitForResponse, derek->ack()),
               jason->expect(ACK,from(derek),WaitForCommand,jason->noAction())
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses forking, and tfm doesn't get the Record-Routing logic correct when forking occurs.");
         return;
      }

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
               jason2->expect(ACK, contact(derek), WaitForResponse, jason2->noAction())
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

   void testInviteForkRedirect()
   {
      WarningLog(<<"*!testInviteForkRedirect!*");

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
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
         derek->expect(INVITE/300,from(proxy),WaitForCommand,chain(new CheckContacts(contacts,0),derek->ack())),
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
      ExecuteSequences();
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
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
      ExecuteSequences();
      Seq(jason2->registerUser(60, jason2->getDefaultContacts()),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses forking, and tfm doesn't get the Record-Routing logic correct when forking occurs.");
         return;
      }

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
               jason2->expect(ACK, contact(derek), WaitForResponse, chain(jason2->noAction(),derek->pause(1000),derek->bye())),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses forking, and tfm doesn't get the Record-Routing logic correct when forking occurs.");
         return;
      }

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, jason1->getDefaultContacts()),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
               jason2->expect(ACK, contact(derek), WaitForResponse, chain(jason2->pause(1000),jason2->bye())),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
               david->expect(INVITE/180,contact(jason),WaitFor180Spiral,david->noAction()),
               jason->expect(CANCEL,from(proxy),WaitForResponseSpiral+2000,chain(jason->ok(), jason->send487())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitFor180Spiral,david->noAction()),
               derek->expect(CANCEL,from(proxy),WaitForResponseSpiral+2000,chain(derek->ok(), derek->send487())),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               cullen->expect(INVITE,contact(david),WaitForCommandSpiral,chain(cullen->ring(),cullen->pause(100),cullen->answer())),
               david->expect(INVITE/180,contact(cullen),WaitFor180Spiral,david->noAction()),
               david->expect(INVITE/200,contact(cullen),WaitForResponseSpiral,david->ack()),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
      RouteGuard enter(*proxy,"sip:spiral@.*","sip:spiral1@localhost");
      RouteGuard spiral1(*proxy,"sip:spiral1@.*","sip:spiral2@localhost");
      RouteGuard spiral2(*proxy,"sip:spiral2@.*","sip:spiral3@localhost");
      RouteGuard spiral3(*proxy,"sip:spiral3@.*","sip:spiral4@localhost");
      RouteGuard spiral4(*proxy,"sip:spiral4@.*","sip:exit@localhost");
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
               jason->expect(INVITE,contact(david),WaitForCommandSpiral,jason->ring()),
               david->expect(INVITE/180,contact(jason),WaitFor180Spiral,david->noAction()),
               jason->expect(CANCEL,from(proxy),WaitForResponseSpiral,chain(jason->ok(), jason->send487())),
               jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommandSpiral,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitFor180Spiral,david->noAction()),
               derek->expect(CANCEL,from(proxy),WaitForResponseSpiral,chain(derek->ok(), derek->send487())),
               derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommandSpiral,chain(enlai->ring(),enlai->pause(2000),enlai->answer())),
               david->expect(INVITE/180,contact(enlai),WaitFor180Spiral,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponseSpiral,david->ack()),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

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
   
   void testInviteFork200And4xx()
   {
      WarningLog(<<"*!testInviteFork200And4xx!*");
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
               enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->pause(100),enlai->answer())),
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse+1000,david->ack()),
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
      
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

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
   
   void testInviteFork200And5xx()
   {
      WarningLog(<<"*!testInviteFork200And5xx!*");
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
               david->expect(INVITE/200,contact(enlai),WaitForResponse+1000,david->ack()),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
   }
   
   void testInviteFork4xx5xx6xx()
   {
      WarningLog(<<"*!testInviteFork4xx5xx6xx!*");
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
         
         enlai->expect(INVITE,contact(david),WaitForCommand,chain(enlai->ring(),enlai->pause(150),enlai->answer())),
         And
         (
            Sub
            (
               david->expect(INVITE/180,contact(enlai),WaitForResponse+300,david->noAction()),
               david->expect(INVITE/180,contact(enlai),WaitForResponse,david->noAction()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->ack()),
               enlai->expect(ACK,contact(david),WaitForResponse,enlai->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand+300,chain(enlai->pause(150),enlai->ring())),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
         
         // .bwc. We don't do the ACKs in this test-case because TestSipEndPoint 
         // doesn't handle the multiple 200s case very well. (If we do ACK each
         // 200, we get three ACKs hitting the same endpoint instead of one ACK
         // each)
         And
         (
            Sub
            (
               optional(david->expect(INVITE/100,from(proxy),WaitFor100,david->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(david),WaitForCommand,jason->answer()),
               david->expect(INVITE/200,contact(jason),WaitForResponse,david->noAction())
            ),
            Sub
            (
               derek->expect(INVITE,contact(david),WaitForCommand,derek->answer()),
               david->expect(INVITE/200,contact(derek),WaitForResponse,david->noAction())
            ),
            Sub
            (
               enlai->expect(INVITE,contact(david),WaitForCommand,enlai->answer()),
               david->expect(INVITE/200,contact(enlai),WaitForResponse,david->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testSequentialQValueInvite()
   {
      WarningLog(<<"*!testSequentialQValueInvite!*");
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

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
               jason->expect(CANCEL, from(proxy), 6000, chain(jason->ok(), jason->send487())),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

      resip::NameAddr contact1=*(jason1->getDefaultContacts().begin());
      contact1.param(p_q)=0.1;
      resip::NameAddr contact2=*(jason2->getDefaultContacts().begin());
      contact2.param(p_q)=0.2;

      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
               jason1->expect(ACK, contact(derek), WaitForAck, jason1->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();  
   }

   void testInviteSeqAllBusyContacts()
   {
      WarningLog(<<"*!testInviteSeqAllBusyContacts!*");
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

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
      ExecuteSequences();
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
      ExecuteSequences();
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
               jason2->expect(ACK,contact(derek),WaitForCommand,chain(derek->pause(2000),derek->bye())),
               jason2->expect(BYE,contact(derek),WaitForCommand+2000,jason2->ok()),
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
      ExecuteSequences();
      Seq(jason1->registerUser(60, contact1),
          jason1->expect(REGISTER/407, from(proxy), WaitForResponse, jason1->digestRespond()),
          jason1->expect(REGISTER/200, from(proxy), WaitForRegistration, jason1->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
      Seq(jason2->registerUser(60, contact2),
          jason2->expect(REGISTER/407, from(proxy), WaitForResponse, jason2->digestRespond()),
          jason2->expect(REGISTER/200, from(proxy), WaitForRegistration, jason2->noAction()),
          WaitForEndOfTest);
      ExecuteSequences();
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
               jason2->expect(ACK,contact(derek),WaitForCommand,chain(jason2->pause(2000),jason2->bye())),
               derek->expect(BYE,contact(jason2),WaitForCommand+2000,derek->ok()),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }

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
               cullen->expect(INVITE,contact(david),WaitForCommandSpiral,cullen->ring()),
               david->expect(INVITE/180,contact(cullen),WaitFor180Spiral,david->noAction()),
               cullen->expect(CANCEL,from(proxy),WaitForCommandSpiral,chain(cullen->ok(),cullen->send487())),
               And
               (
                  Sub
                  (
                     cullen->expect(ACK,from(proxy),WaitForCommand,cullen->noAction())
                  ),
                  Sub
                  (
                     derek->expect(INVITE,contact(david),WaitForCommandSpiral,derek->ring())
                  )
               ),

               david->expect(INVITE/180,contact(derek),WaitFor180Spiral,david->noAction()),
               derek->expect(CANCEL,from(proxy),WaitForCommandSpiral,chain(derek->ok(), derek->send487())),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommandSpiral,chain(jason->ring(),jason->answer()))
                  )
               ),

               david->expect(INVITE/180,contact(jason),WaitFor180Spiral,david->noAction()),
               david->expect(INVITE/200,contact(jason),WaitForResponseSpiral,david->ack()),
               jason->expect(ACK,contact(david),WaitForResponse,jason->noAction())
            )
         ),


         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testInviteSpiralThenSeqFork()
   {
      WarningLog(<<"*!testInviteSpiralThenSeqFork!*");
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
      
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
               derek->expect(INVITE,contact(david),WaitForCommandSpiral,derek->ring()),
               david->expect(INVITE/180,contact(derek),WaitFor180Spiral,david->noAction()),
               derek->expect(CANCEL,from(proxy),WaitForCommandSpiral,chain(derek->ok(), derek->send487())),
               And
               (
                  Sub
                  (
                     derek->expect(ACK,from(proxy),WaitForResponse,derek->noAction())
                  ),
                  Sub
                  (
                     jason->expect(INVITE,contact(david),WaitForCommandSpiral,jason->ring())
                  )
               ),

               david->expect(INVITE/180,contact(jason),WaitFor180Spiral,david->noAction()),
               jason->expect(CANCEL,from(proxy),WaitForCommandSpiral,chain(jason->ok(), jason->send487())),
               And
               (
                  Sub
                  (
                     jason->expect(ACK,from(proxy),WaitForResponse,jason->noAction())
                  ),
                  Sub
                  (
                     enlai->expect(INVITE,contact(david),WaitForCommandSpiral,chain(enlai->ring(),enlai->answer()))
                  )
               ),

               david->expect(INVITE/180,contact(enlai),WaitFor180Spiral,david->noAction()),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
               derek->expect(CANCEL,from(proxy),6000,chain(derek->ok(),derek->send487())),
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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
      if(resip::InteropHelper::getRRTokenHackEnabled())
      {
         WarningLog(<<"This test uses third-party registrations, and will not work with the flow-token hack enabled.");
         return;
      }
      
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



//*************non-INVITE scenarios************************************//
  
  //*************Sunny Day***************//   

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

   void testNonInviteBusy()
   {
      WarningLog(<<"*!testNonInviteBusy!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send486()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteTLS()
   {
      WarningLog(<<"*!testNonInviteTLS!*");
      
      Seq(derekTls->registerUserWithOutbound(60, derekTls->getDefaultContacts()),
          derekTls->expect(REGISTER/407, from(proxy), WaitForResponse, derekTls->digestRespond()),
          derekTls->expect(REGISTER/200, from(proxy), WaitForResponse, derekTls->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jasonTls->message(*derekTls,"Ping"),
         jasonTls->expect(MESSAGE/407,from(proxy),WaitForResponse,jasonTls->digestRespond()),
         derekTls->expect(MESSAGE,from(proxy),WaitForCommand,derekTls->ok()),
         jasonTls->expect(MESSAGE/200,from(proxy),WaitForResponse, jasonTls->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteSpiral()
   {
      WarningLog(<<"*!testNonInviteSpiral!*");
      
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
         derek->message(proxy->makeUrl("9spiral").uri(),"Ping"),
         derek->expect(MESSAGE/407,from(proxy),WaitForResponse,derek->digestRespond()),
         david->expect(MESSAGE, from(proxy),2*WaitForCommand,david->ok()),
         derek->expect(MESSAGE/200,from(proxy),WaitForResponse, derek->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteNoContacts()
   {
      WarningLog(<<"*!testNonInviteNoContacts!*");
      
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         jason->expect(MESSAGE/480, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite4xxResponse()
   {
      WarningLog(<<"*!testNonInvite4xxResponse!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send400()),
         jason->expect(MESSAGE/400, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite5xxResponse()
   {
      WarningLog(<<"*!testNonInvite5xxResponse!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send500()),
         jason->expect(MESSAGE/500, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite6xxResponse()
   {
      WarningLog(<<"*!testNonInvite6xxResponse!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send600()),
         jason->expect(MESSAGE/600, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteBadAuth()
   {
      WarningLog(<<"*!testNonInviteBadAuth!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, condition(bogusAuth,jason->digestRespond())),
         jason->expect(MESSAGE/403, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteUDPtoTCP()
   {
      WarningLog(<<"*!testNonInviteUDPtoTCP!*");
      
      Seq(derekTcp->registerUser(60, derekTcp->getDefaultContacts()),
          derekTcp->expect(REGISTER/407, from(proxy), WaitForResponse, derekTcp->digestRespond()),
          derekTcp->expect(REGISTER/200, from(proxy), WaitForResponse, derekTcp->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derekTcp,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derekTcp->expect(MESSAGE, from(proxy), WaitForCommand, derekTcp->ok()),
         jason->expect(MESSAGE/200, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteTCPtoUDP()
   {
      WarningLog(<<"*!testNonInviteTCPtoUDP!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jasonTcp->message(*derek,"Ping"),
         jasonTcp->expect(MESSAGE/407, from(proxy), WaitForResponse, jasonTcp->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->ok()),
         jasonTcp->expect(MESSAGE/200, from(proxy),WaitForResponse, jasonTcp->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteWith180andCancel()
   {
      WarningLog(<<"*!testNonInviteWith180andCancel!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(derek->ring(),derek->send486())),
         jason->expect(MESSAGE/180, from(proxy), WaitForResponse, jason->cancel()),
         And
         (
            Sub
            (
               jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction())
            ),
            Sub
            (
               jason->expect(CANCEL/200, from(proxy), WaitForResponse, derek->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite200ThenCancel()
   {
      WarningLog(<<"*!testNonInvite200ThenCancel!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->ok()),
         jason->expect(MESSAGE/200, from(proxy), WaitForResponse, jason->cancel()),
         jason->expect(CANCEL/200, from(proxy), WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite400ThenCancel()
   {
      WarningLog(<<"*!testNonInvite400ThenCancel!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send400()),
         jason->expect(MESSAGE/400, from(proxy), WaitForResponse, jason->cancel()),
         jason->expect(CANCEL/200, from(proxy), WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite500ThenCancel()
   {
      WarningLog(<<"*!testNonInvite500ThenCancel!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send500()),
         jason->expect(MESSAGE/500, from(proxy), WaitForResponse, jason->cancel()),
         jason->expect(CANCEL/200, from(proxy), WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite407Dropped()
   {
      WarningLog(<<"*!testNonInvite407Dropped!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         save(msg, jason->message(*derek,"Ping")),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, chain(jason->pause(500),jason->retransmit(msg))),
         jason->expect(MESSAGE/407, from(proxy), 1100, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send486()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteDropped()
   {
      WarningLog(<<"*!testNonInviteDropped!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->noAction()),
         derek->expect(MESSAGE, from(proxy), 550, derek->send486()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteWith180()
   {
      WarningLog(<<"*!testNonInviteWith180!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(derek->ring(),derek->send486())),
         jason->expect(MESSAGE/180, from(proxy),WaitForResponse, jason->noAction()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteSpam180()
   {
      WarningLog(<<"*!testNonInviteSpam180!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(derek->ring(),derek->ring(),derek->ring(),derek->ring(),derek->send486())),
         And
         (
            Sub
            (
               optional(derek->expect(MESSAGE, from(proxy), WaitForCommand,derek->noAction()))
            ),
            Sub
            (
               jason->expect(MESSAGE/180, from(proxy),WaitForResponse, jason->noAction()),
               jason->expect(MESSAGE/180, from(proxy),WaitForResponse, jason->noAction()),
               jason->expect(MESSAGE/180, from(proxy),WaitForResponse, jason->noAction()),
               jason->expect(MESSAGE/180, from(proxy),WaitForResponse, jason->noAction()),
               jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteWithAck200()
   {
      WarningLog(<<"*!testNonInviteWithAck200!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->subscribe(*derek,Token("fake")),
         jason->expect(SUBSCRIBE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(SUBSCRIBE, from(proxy), WaitForCommand, derek->ok()),
         jason->expect(SUBSCRIBE/200, from(proxy),WaitForResponse, jason->ack()),
         derek->expect(ACK, from(jason),WaitForAck,derek->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteWithAckFailure()
   {
      WarningLog(<<"*!testNonInviteWithAckFailure!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send486()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->ack()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite200Dropped()
   {
      WarningLog(<<"*!testNonInvite200Dropped!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, save(msg,jason->digestRespond())),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->ok()),
         jason->expect(MESSAGE/200, from(proxy),WaitForResponse, jason->retransmit(msg)),
         jason->expect(MESSAGE/200, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteSpam200()
   {
      WarningLog(<<"*!testNonInviteSpam200!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();
      boost::shared_ptr<SipMessage> resp;
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(save(resp,derek->ok()),derek->retransmit(resp),derek->retransmit(resp),derek->retransmit(resp))),
         jason->expect(MESSAGE/200, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInvite200then180()
   {
      WarningLog(<<"*!testNonInvite200then180!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, jason->digestRespond()),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(derek->ok(),derek->ring())),
         jason->expect(MESSAGE/200, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteSpamRequestBeforeResponse()
   {
      WarningLog(<<"*!testNonInviteSpamRequestBeforeResponse!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, save(msg,jason->digestRespond())),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(jason->retransmit(msg), jason->retransmit(msg), jason->retransmit(msg),derek->pause(200),derek->send486())),
         optional(jason->expect(MESSAGE/100,from(proxy),WaitForResponse+200,jason->noAction())),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteSpamRequestAfterResponse()
   {
      WarningLog(<<"*!testNonInviteSpamRequestAfterResponse!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, save(msg,jason->digestRespond())),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, derek->send486()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse, chain(jason->retransmit(msg),jason->retransmit(msg),jason->retransmit(msg))),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse,jason->noAction()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse,jason->noAction()),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }
   
   void testNonInviteWithInviteCollision()
   {
      WarningLog(<<"*!testNonInviteWithInviteCollision!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, save(msg,jason->digestRespond())),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(condition(makeInvite,jason->retransmit(msg)),derek->send486())),
         jason->expect(MESSAGE/486, from(proxy),WaitForResponse,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testNonInviteWithWrongMethodResponse()
   {
      WarningLog(<<"*!testNonInviteWithWrongMethodResponse!*");
      
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForResponse, derek->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         jason->message(*derek,"Ping"),
         jason->expect(MESSAGE/407, from(proxy), WaitForResponse, save(msg,jason->digestRespond())),
         derek->expect(MESSAGE, from(proxy), WaitForCommand, chain(condition(makeInvite,jason->retransmit(msg)),condition(unknownCSeq,derek->send200()))),
         derek->expect(MESSAGE, from(proxy), 1000, chain(condition(makeInvite,jason->retransmit(msg)),derek->send200())),
         jason->expect(MESSAGE/200, from(proxy),WaitForResponse,jason->noAction()),
         WaitForEndOfTest
      );
      
      ExecuteSequences();
   }

   void testReflectedInvite()
   {
      WarningLog(<<"*!testReflectedInvite!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();

      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->reflect(*derek)),
                 optional(jason->expect(INVITE/100,from(proxy), WaitFor100, jason->noAction())),
                 jason->expect(INVITE/480,from(proxy),WaitForResponse,jason->ack()),
                 jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/408,from(proxy),32000, derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteReflectedAsNonInvite()
   {
      WarningLog(<<"*!testInviteReflectedAsNonInvite!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->reflect(*derek,MESSAGE)),
                 jason->expect(MESSAGE/480,from(proxy),WaitForResponse,jason->noAction()),
                 jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/408,from(proxy),32000, derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteReflectedAsAck()
   {
      WarningLog(<<"*!testInviteReflectedAsAck!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->reflect(*derek,ACK)),
                 jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/408,from(proxy),32000, derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteNonInviteResponse()
   {
      WarningLog(<<"*!testInviteNonInviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(makeMessage,jason->ok())),
                 jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/408,from(proxy),32000, derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteAckResponse()
   {
      WarningLog(<<"*!testInviteAckResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(makeAck,jason->ok())),
                 jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/408,from(proxy),32000, derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


// ******************** non-INVITE ********************//

   void testNitReflectedAsInvite()
   {
      WarningLog(<<"*!testNitReflectedAsInvite!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->reflect(*derek,INVITE)),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
               jason->expect(INVITE/480,from(proxy),WaitForResponse,jason->ack())
            ),
            Sub
            (
               jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitReflected()
   {
      WarningLog(<<"*!testNitReflected!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->reflect(*derek)),
         And
         (
            Sub
            (
               jason->expect(MESSAGE/480,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitReflectedAsDifferentNit()
   {
      WarningLog(<<"*!testNitReflectedAsDifferentNit!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->reflect(*derek,INFO)),
         And
         (
            Sub
            (
               jason->expect(INFO/480,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
               optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitReflectedAsAck()
   {
      WarningLog(<<"*!testNitReflectedAsAck!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->reflect(*derek,ACK)),
         jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitInviteResponse()
   {
      WarningLog(<<"*!testNitInviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(makeInvite,jason->ok())),
         jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitDifferentNitResponse()
   {
      WarningLog(<<"*!testNitDifferentNitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(makeInfo,jason->ok())),
         jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAckResponse()
   {
      WarningLog(<<"*!testNitAckResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(makeAck,jason->ok())),
         jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


// ******************** ACK ********************//

   void testAck200ReflectedAsInvite()
   {
      WarningLog(<<"*!testAck200ReflectedAsInvite!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->reflect(*derek,INVITE)),
         optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
         jason->expect(INVITE/480,from(proxy),WaitForResponse,jason->ack()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200ReflectedAsNit()
   {
      WarningLog(<<"*!testAck200ReflectedAsNit!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->reflect(*derek,MESSAGE)),
         jason->expect(MESSAGE/480,from(proxy),WaitForResponse,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200Reflected()
   {
      WarningLog(<<"*!testAck200Reflected!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->reflect(*derek)),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureReflectedAsInvite()
   {
      WarningLog(<<"*!testAckFailureReflectedAsInvite!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()))),
         And
         (
            Sub
            (
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->reflect(*derek,INVITE)),
               optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
               jason->expect(INVITE/480,from(proxy),WaitForResponse,jason->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureReflectedAsNit()
   {
      WarningLog(<<"*!testAckFailureReflectedAsNit!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()))),
         And
         (
            Sub
            (
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->reflect(*derek,MESSAGE)),
               jason->expect(MESSAGE/480,from(proxy),WaitForResponse,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureReflected()
   {
      WarningLog(<<"*!testAckFailureReflected!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()))),
         And
         (
            Sub
            (
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->reflect(*derek))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200InviteResponse()
   {
      WarningLog(<<"*!testAck200InviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand, condition(makeInvite,jason->send200())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200NitResponse()
   {
      WarningLog(<<"*!testAck200NitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand, condition(makeInfo,jason->send200())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureInviteResponse()
   {
      WarningLog(<<"*!testAckFailureInviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()))),
         And
         (
            Sub
            (
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, condition(makeInvite,jason->send200()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureNitResponse()
   {
      WarningLog(<<"*!testAckFailureNitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()))),
         And
         (
            Sub
            (
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, condition(makeInfo,jason->send200()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


//****************** Oddball UAS ********************//

   void testInviteBranchCaseAltered()
   {
      WarningLog(<<"*!testInviteBranchCaseAltered!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(fiddleBranchCase,jason->ok())))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitBranchCaseAltered()
   {
      WarningLog(<<"*!testNitBranchCaseAltered!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(fiddleBranchCase,jason->ok())),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInvite6xxThen2xx()
   {
      WarningLog(<<"*!testInvite6xxThen2xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send600(),jason->ok())))),
         And
         (
            Sub
            (
               derek->expect(INVITE/600,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInvite2xxThen6xx()
   {
      WarningLog(<<"*!testInvite2xxThen6xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ok(),jason->send600())))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


//****************** Misbehaving UAS ********************//

   void testInviteUASRemovesProxyVia()
   {
      WarningLog(<<"*!testInviteUASRemovesProxyVia!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(removeProxyVias,jason->ok())))),
         derek->expect(INVITE/500,from(proxy),WaitForResponse,derek->ack()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteUASAddsVia()
   {
      WarningLog(<<"*!testInviteUASAddsVia!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(addProxyVia,jason->ok())))),
         derek->expect(INVITE/500,from(proxy),WaitForResponse,derek->ack()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteUASChangesProxyBranch()
   {
      WarningLog(<<"*!testInviteUASChangesProxyBranch!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(corruptProxyBranch,jason->ok())))),
         derek->expect(INVITE/500,from(proxy),WaitForResponse,derek->ack()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInvite2xxThen1xx()
   {
      WarningLog(<<"*!testInvite2xxThen1xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ok(),jason->ring())))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInvite4xxThen1xx()
   {
      WarningLog(<<"*!testInvite4xxThen1xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send400(),jason->ring())))),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInvite2xxThen4xx()
   {
      WarningLog(<<"*!testInvite2xxThen4xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ok(),jason->send400())))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInvite4xxThen2xx()
   {
      WarningLog(<<"*!testInvite4xxThen2xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send400(),jason->ok())))),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteMultiple4xx()
   {
      WarningLog(<<"*!testInviteMultiple4xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send400(),jason->send487(),jason->send400())))),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction()),
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction()),
               jason->expect(ACK,from(proxy),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteMalformed1xxWithTimeout()
   {
      WarningLog(<<"*!testInviteMalformed1xxWithTimeout!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,condition(removeTo,jason->ring())),
                 jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
                 jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()))),
         And
         (
            Sub
            (
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               derek->expect(INVITE/408,from(proxy),32000, derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200WithResponse()
   {
      WarningLog(<<"*!testAck200WithResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()))),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->send200()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureWithResponse()
   {
      WarningLog(<<"*!testAckFailureWithResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
             Sub(jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()))),
         And
         (
            Sub
            (
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand,jason->send200())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitUASRemovesProxyVia()
   {
      WarningLog(<<"*!testNitUASRemovesProxyVia!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(removeProxyVias,jason->ok())),
         derek->expect(MESSAGE/500,from(proxy),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitUASAddsVia()
   {
      WarningLog(<<"*!testNitUASAddsVia!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(addProxyVia,jason->ok())),
         derek->expect(MESSAGE/500,from(proxy),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitUASChangesProxyBranch()
   {
      WarningLog(<<"*!testNitUASChangesProxyBranch!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(corruptProxyBranch,jason->ok())),
         derek->expect(MESSAGE/500,from(proxy),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNit2xxThen1xx()
   {
      WarningLog(<<"*!testNit2xxThen1xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,chain(jason->ok(),jason->ring())),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNit4xxThen1xx()
   {
      WarningLog(<<"*!testNit4xxThen1xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,chain(jason->send487(),jason->ring())),
         derek->expect(MESSAGE/487,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNit2xxThen4xx()
   {
      WarningLog(<<"*!testNit2xxThen4xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,chain(jason->ok(),jason->send487())),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNit4xxThen2xx()
   {
      WarningLog(<<"*!testNit4xxThen2xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,chain(jason->send487(),jason->ok())),
         derek->expect(MESSAGE/487,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitMultiple4xx()
   {
      WarningLog(<<"*!testNitMultiple4xx!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,chain(jason->send487(),jason->send400(),jason->send403())),
         derek->expect(MESSAGE/487,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitMalformed1xxWithTimeout()
   {
      WarningLog(<<"*!testNitMalformed1xxWithTimeout!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      Seq
      (
         derek->message(*jason,"ping"),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,condition(removeTo,jason->send100())),
         jason->expect(MESSAGE,contact(derek),resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         jason->expect(MESSAGE,contact(derek),4100,jason->noAction()),
         optional(jason->expect(MESSAGE,contact(derek),4100,jason->noAction())),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


//***************************** UAS tests ********************************//

//****************** Transaction collision ********************//

// ******************** INVITE ********************//

   void testInviteAndNitCollide()
   {
      WarningLog(<<"*!testInviteAndNitCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> inv;
      Seq
      (
         chain(inv<=derek->invite(*jason),condition(makeMessage,derek->sendSip(inv,*jason))),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->answer())
            )
         ),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteAndAckCollide()
   {
      WarningLog(<<"*!testInviteAndAckCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> inv;
      Seq
      (
         chain(inv<=derek->invite(*jason),condition(makeAck,derek->sendSip(inv,*jason))),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->answer())
            )
         ),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteAndResponse()
   {
      WarningLog(<<"*!testInviteAndResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> inv;
      Seq
      (
         chain(inv<=derek->invite(*jason),rawcondition(make200Response,derek->sendSip(inv,*jason))),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->answer())
            )
         ),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteAndNitResponse()
   {
      WarningLog(<<"*!testInviteAndNitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> inv;
      Seq
      (
         chain(inv<=derek->invite(*jason),rawcondition(make200Response,condition(makeInfo,derek->sendSip(inv,*jason)))),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->answer())
            )
         ),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testInviteAndAckResponse()
   {
      WarningLog(<<"*!testInviteAndAckResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> inv;
      Seq
      (
         chain(inv<=derek->invite(*jason),rawcondition(make200Response,condition(makeAck,derek->sendSip(inv,*jason)))),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->answer())
            )
         ),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack()),
         jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


// ******************** non-INVITE ********************//

   void testNitAndInviteCollide()
   {
      WarningLog(<<"*!testNitAndInviteCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),condition(makeInvite,derek->sendSip(msg,*jason))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAndDifferentNitCollide()
   {
      WarningLog(<<"*!testNitAndDifferentNitCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),condition(makeInfo,derek->sendSip(msg,*jason))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAndAckCollide()
   {
      WarningLog(<<"*!testNitAndAckCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),condition(makeAck,derek->sendSip(msg,*jason))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAndInviteResponse()
   {
      WarningLog(<<"*!testNitAndInviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),rawcondition(make200Response,condition(makeInvite,derek->sendSip(msg,*jason)))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAndResponse()
   {
      WarningLog(<<"*!testNitAndResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),rawcondition(make200Response,derek->sendSip(msg,*jason))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAndDifferentNitResponse()
   {
      WarningLog(<<"*!testNitAndDifferentNitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),rawcondition(make200Response,condition(makeInfo,derek->sendSip(msg,*jason)))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testNitAndAckResponse()
   {
      WarningLog(<<"*!testNitAndAckResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> msg;
      Seq
      (
         chain(msg<=derek->message(*jason,"ping"),rawcondition(make200Response,condition(makeAck,derek->sendSip(msg,*jason)))),
         jason->expect(MESSAGE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(MESSAGE/200,contact(jason),WaitForResponse,derek->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


// ******************** ACK ********************//

   void testAck200AndInviteCollide()
   {
      WarningLog(<<"*!testAck200AndInviteCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeInvite,derek->retransmit(ack)))),
         And
         (
            Sub
            (
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,from(proxy),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200AndNitCollide()
   {
      WarningLog(<<"*!testAck200AndNitCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeMessage,derek->retransmit(ack)))),
         And
         (
            Sub
            (
               derek->expect(MESSAGE/400,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200AndInviteResponse()
   {
      WarningLog(<<"*!testAck200AndInviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeInvite,derek->retransmit(ack))))),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200AndNitResponse()
   {
      WarningLog(<<"*!testAck200AndNitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeMessage,derek->retransmit(ack))))),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAck200AndAckResponse()
   {
      WarningLog(<<"*!testAck200AndAckResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
         derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,derek->retransmit(ack)))),
         jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction()),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureAndInviteCollide()
   {
      WarningLog(<<"*!testAckFailureAndInviteCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeInvite,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureAndNitCollide()
   {
      WarningLog(<<"*!testAckFailureAndNitCollide!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeMessage,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureAndInviteResponse()
   {
      WarningLog(<<"*!testAckFailureAndInviteResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeInvite,derek->retransmit(ack)))))
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureAndNitResponse()
   {
      WarningLog(<<"*!testAckFailureAndNitResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeMessage,derek->retransmit(ack)))))
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

   void testAckFailureAndAckResponse()
   {
      WarningLog(<<"*!testAckFailureAndAckResponse!*");

      // Disable digest authentication for this test
      DisableDigestAuthGuard daGuard(*proxy);

      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
         jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
         And
         (
            Sub
            (
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(ACK,from(proxy),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
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

      Seq
      (
         derek->info(jason),
         derek->expect(INFO/407, from(proxy), 1000, derek->digestRespond()),
         jason->expect(INFO, from(derek), 500, jason->noAction()),
         jason->expect(INFO, from(derek), 600, jason->noAction()),
         jason->expect(INFO, from(derek), 1100, jason->noAction()),
         And
         (
            Sub
            (
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction()),
               jason->expect(INFO, from(derek), 4800, jason->noAction())
            ),
            Sub
            (
               derek->expect(INFO/100, from(proxy), 1000, derek->noAction())
            )
         ),
          // note: 408 to NIT are not forwarded by repro
          //derek->expect(INFO/408, from(proxy), 4800, jason->noAction()),
          32000);// Wait extra long for blacklist to expire
      ExecuteSequences();
   }
   
   void testNonInviteServerRetransmission()
   {
      WarningLog(<<"*!testNonInviteServerRetransmission!*");

      Seq(david->registerUser(60, david->getDefaultContacts()),
          david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
          david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
          WaitForEndOfSeq);
      ExecuteSequences();

      boost::shared_ptr<SipMessage> infoMsg;
      Seq(save(infoMsg, derek->info(david)),
          derek->expect(INFO/407, from(proxy), 1000, derek->digestRespond()),
          david->expect(INFO, from(derek), 1000, david->noAction()),
          david->expect(INFO, from(derek), 1000, david->ok()),
          derek->expect(INFO/200, from(david), 1000, derek->retransmit(infoMsg)),
          derek->expect(INFO/200, from(david), 1000, derek->noAction()),
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
            resip_assert(msgEvent);
            boost::shared_ptr<resip::SipMessage> msg = msgEvent->getMessage();
            resip_assert(msg.get());

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
      resip_assert(!msg->header(h_Vias).empty());
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

          And(Sub(optional(derek->expect(INVITE/100, from(proxy), 1000, derek->checkEchoName(checkRport)))),
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
          WaitForEndOfSeq);
      
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
                      Sub(jason->expect(NOTIFY/407, from(proxy), WaitForResponse, jason->digestRespond()),
                          derek->expect(NOTIFY, from(jason), WaitForCommand, derek->ok()),
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
                  optional(jason->expect(INVITE/100, from(proxy), WaitForPause, jason->noAction())),
                  jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
                  And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitForPause, jason->noAction()))),
                      Sub(derek->expect(INVITE, contact(jason), WaitForPause, chain(derek->ring(), derek->answer())),
                          jason->expect(INVITE/180, from(derek), WaitFor180, jason->noAction()),
                          jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                          derek->expect(ACK, from(jason), WaitForResponse, derek->noAction()))))),
          WaitForEndOfTest);
      
      ExecuteSequences();
      
      //102 refers 103 to 104
      Seq(jason->refer(derek->getContact().uri(), david->getAddressOfRecord()),
          jason->expect(REFER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          derek->expect(REFER, from(jason), WaitForCommand, chain(derek->send202(), derek->inviteReferredBy())),
          And(Sub(jason->expect(REFER/202, from(derek), WaitForResponse, jason->noAction())),
              Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                  derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(), derek->digestRespond())),
                  And(Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))),
                      Sub(david->expect(INVITE, contact(derek), WaitForCommand, chain(david->send100(), david->answer())),
                          derek->expect(INVITE/200, contact(david), WaitForResponse, chain(derek->ack(), derek->notify200(*jason))),
                          And(Sub(derek->expect(NOTIFY/407, from(proxy), WaitForResponse, derek->digestRespond()),
                                  jason->expect(NOTIFY, from(derek), WaitForCommand, chain(jason->ok(), jason->note("sending bye"), jason->bye())),
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
                  optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
                  jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
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
          WaitForEndOfSeq);
      
      ExecuteSequences();
      
      //102 takes 103 off hold
      Seq(jason->reInvite(*derek),
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
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
          optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction())),
          jason->expect(INVITE/407, from(proxy), WaitForResponse, chain(jason->ack(), jason->digestRespond())),
          And(Sub(optional(jason->expect(INVITE/100, from(proxy), WaitFor100, jason->noAction()))),
              Sub(derek->expect(INVITE, contact(jason), WaitForCommand, chain(derek->ring(), derek->answer())),
                  jason->expect(INVITE/180, from(derek), WaitFor180, jason->noAction()),
                  jason->expect(INVITE/200, contact(derek), WaitForResponse, jason->ack()),
                  derek->expect(ACK, from(jason), WaitForResponse, derek->noAction()))),
          WaitForEndOfTest);
      
      ExecuteSequences();

      //102 refers with replaces 103 to 104

      Seq(jason->referReplaces(derek->getContact().uri(), david->getAddressOfRecord()),
          jason->expect(REFER/407, from(proxy), WaitForResponse, jason->digestRespond()),
          derek->expect(REFER, from(jason), WaitForCommand, chain(derek->send202(), derek->inviteReferReplaces())),
          And(Sub(jason->expect(REFER/202, from(derek), WaitForResponse, jason->noAction())),
              Sub(optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                  derek->expect(INVITE/407, from(proxy), WaitForResponse, chain(derek->ack(),derek->digestRespond())),
                  optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction())),
                  david->expect(INVITE, contact(derek), WaitForCommand, chain(david->send100(), david->ok())),
                  derek->expect(INVITE/200, contact(david), WaitForResponse, chain(derek->ack(), derek->notify200(*jason))),
                  And(Sub(david->expect(ACK, from(derek), WaitForResponse, david->noAction())),
                      Sub(derek->expect(NOTIFY/407, from(proxy), WaitForResponse, derek->digestRespond()),
                          jason->expect(NOTIFY, from(derek), WaitForCommand, jason->ok()),
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

          And
          (
            Sub
            (
               optional(derek->expect(INVITE/100, from(proxy), WaitFor100, derek->noAction()))
            ),
            Sub
            (
               jason2->expect(INVITE, contact(derek), WaitForCommand, chain(jason2->ring(), jason2->pause(PauseTime), ok <= jason2->answer())),
               derek->expect(INVITE/180, from(jason2), WaitFor180, derek->noAction()),
               derek->expect(INVITE/200, contact(jason2), WaitForPause, chain(jason2->pause(500),jason2->retransmit(ok))),
               derek->expect(INVITE/200, contact(jason2), WaitForPause+500, derek->ack()),
               jason2->expect(ACK, from(derek), WaitForResponse, chain(jason2->pause(PauseTime), jason2->bye())),
               derek->expect(BYE, from(jason2), WaitForResponse+PauseTime, derek->ok()),
               jason2->expect(BYE/200, from(derek), WaitForResponse, jason2->noAction())
            ),
            Sub
            (
               jason1->expect(INVITE, contact(derek), WaitForCommand, jason1->ring()),
               derek->expect(INVITE/180, from(jason1), WaitFor180, derek->noAction()),
               jason1->expect(CANCEL, from(proxy), WaitForCommand, chain(jason1->ok(), jason1->send487())),
               jason1->expect(ACK, from(proxy), WaitForAck, jason1->noAction())
            )
         ),
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
          WaitForEndOfSeq);
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(derek->getDefaultContacts(), 60)),
          WaitForEndOfSeq);
      ExecuteSequences();

      // second time, it has the credentials already
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfSeq);
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(derek->getDefaultContacts(), 60)),
          WaitForEndOfSeq);
      ExecuteSequences();
   }

   void testDigestInviteBasic()
   {
      WarningLog(<<"*!testDigestInviteBasic!*");

      // second time, it may have the credentials already
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/407, from(proxy), 1000, jason->digestRespond()),
          jason->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(jason->getDefaultContacts(), 60)),
          WaitForEndOfSeq);
      Seq(derek->registerUser(60, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), 1000, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), 1000, new CheckContacts(derek->getDefaultContacts(), 60)),
          WaitForEndOfSeq);
      ExecuteSequences();

      Seq(david->invite(*jason),
          optional(david->expect(INVITE/100, from(proxy), 2000, david->noAction())),
          david->expect(INVITE/407, from(proxy), 2000, chain(david->ack(), david->pause(2000), david->digestRespond())),
          And( Sub(jason->expect(INVITE, contact(david), 2000, chain(jason->ring(), jason->answer())),
                   david->expect(INVITE/180, from(jason), 2000, david->noAction()),
                   david->expect(INVITE/200, contact(jason), 2000, david->ack()),
                   jason->expect(ACK, from(david), 2000, jason->noAction())),
               Sub(optional(david->expect(INVITE/100, from(proxy), 2000, david->noAction())))),
          WaitForEndOfSeq);
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
          WaitForEndOfSeq);
      Seq(derek->registerUser(600, derek->getDefaultContacts()),
          derek->expect(REGISTER/407, from(proxy), WaitForResponse, derek->digestRespond()),
          derek->expect(REGISTER/200, from(proxy), WaitForRegistration, derek->noAction()),
          WaitForEndOfSeq);
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
      RouteGuard dGuard1(*proxy, "sip:u@.*", "sip:derek@localhost", "INVITE");
      // SUBSCRIBEs for dialog u@.* are routed to david.
      RouteGuard dGuard2(*proxy, "sip:u@.*", "sip:david@localhost", "SUBSCRIBE", "dialog");
      // SUBSCRIBEs for status u@.* are routed to enlai.
      RouteGuard dGuard3(*proxy, "sip:u@.*", "sip:enlai@localhost", "SUBSCRIBE", "status");
      // Everything else is routed to jason.
      // Give weight 2 for the route to give this route a lower priority
      // than the ones above.
      RouteGuard dGuard4(*proxy, "", "sip:jason@localhost", Data::Empty, Data::Empty, 2);

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
          optional(cullen->expect(INVITE/100, from(proxy), WaitFor100, cullen->noAction())),
          cullen->expect(INVITE/407, from(proxy), WaitForResponse, chain(cullen->ack(), cullen->digestRespond())),
          And(Sub(optional(cullen->expect(INVITE/100, from(proxy), WaitFor100, cullen->noAction()))),
              Sub(derek->expect(INVITE, contact(cullen), WaitForCommand, derek->answer()),
                  cullen->expect(INVITE/200, contact(derek), WaitForResponse, cullen->ack()),
                  derek->expect(ACK, from(cullen), WaitForResponse, cullen->noAction()))),
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
         jason->expect(ACK, contact(derek), WaitForResponse, jason->bye(*derek)),
         derek->expect(BYE, contact(jason), WaitForResponse, derek->ok()),
         jason->expect(BYE/200, contact(derek), WaitForResponse, derek->noAction()),
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
          jozsef->expect(REGISTER/200, from(proxy), WaitForResponse, chain(jozsef->rawSend(server, errMsg), jozsef->pause(200), jozsef->registerUser(60, jozsef->getDefaultContacts()))),
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
          jozsef->expect(REGISTER/200, from(proxy), WaitForResponse, chain(jozsef->rawSend(server, errMsg), jozsef->pause(50), jozsef->registerUser(60, jozsef->getDefaultContacts()))),
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

         TEST(testReflectedInvite);
         TEST(testInviteReflectedAsNonInvite);
         TEST(testInviteReflectedAsAck);
         TEST(testInviteNonInviteResponse);
         TEST(testInviteAckResponse);

      // ******************** non-INVITE ********************//
         TEST(testNitReflectedAsInvite);
         TEST(testNitReflected);
         TEST(testNitReflectedAsDifferentNit);
         TEST(testNitReflectedAsAck);
         TEST(testNitInviteResponse);
         TEST(testNitDifferentNitResponse);
         TEST(testNitAckResponse);

      // ******************** ACK ********************//
         TEST(testAck200ReflectedAsInvite);
         TEST(testAck200ReflectedAsNit);
         TEST(testAck200Reflected);
         TEST(testAckFailureReflectedAsInvite);
         TEST(testAckFailureReflectedAsNit);
         TEST(testAckFailureReflected);
         TEST(testAck200InviteResponse);
         TEST(testAck200NitResponse);
         TEST(testAckFailureInviteResponse);
         TEST(testAckFailureNitResponse);
      
      //****************** Oddball UAS ********************//
         TEST(testInviteBranchCaseAltered);
         TEST(testNitBranchCaseAltered);
         TEST(testInvite6xxThen2xx);
         TEST(testInvite2xxThen6xx);
      
      //****************** Misbehaving UAS ********************//
         TEST(testInviteUASRemovesProxyVia);
         TEST(testInviteUASAddsVia);
         TEST(testInviteUASChangesProxyBranch);
         TEST(testInvite2xxThen1xx);
         TEST(testInvite4xxThen1xx);
         TEST(testInvite2xxThen4xx);
         TEST(testInvite4xxThen2xx);
         TEST(testInviteMultiple4xx);
         TEST(testInviteMalformed1xxWithTimeout);
         TEST(testAck200WithResponse);
         TEST(testAckFailureWithResponse);
         TEST(testNitUASRemovesProxyVia);
         TEST(testNitUASAddsVia);
         TEST(testNitUASChangesProxyBranch);
         TEST(testNit2xxThen1xx);
         TEST(testNit4xxThen1xx);
         TEST(testNit2xxThen4xx);
         TEST(testNit4xxThen2xx);
         TEST(testNitMultiple4xx);
         TEST(testNitMalformed1xxWithTimeout);

//***************************** UAS tests ********************************//
      //****************** Transaction collision ********************//
      // ******************** INVITE ********************//
         TEST(testInviteAndNitCollide);
         TEST(testInviteAndAckCollide);
         TEST(testInviteAndResponse);
         TEST(testInviteAndNitResponse);
         TEST(testInviteAndAckResponse);
      
      // ******************** non-INVITE ********************//
         TEST(testNitAndInviteCollide);
         TEST(testNitAndDifferentNitCollide);
         TEST(testNitAndAckCollide);
         TEST(testNitAndInviteResponse);
         TEST(testNitAndResponse);
         TEST(testNitAndDifferentNitResponse);
         TEST(testNitAndAckResponse);
      
      // ******************** ACK ********************//
         TEST(testAck200AndInviteCollide);
         TEST(testAck200AndNitCollide);
         TEST(testAck200AndInviteResponse);
         TEST(testAck200AndNitResponse);
         TEST(testAck200AndAckResponse);
         TEST(testAckFailureAndInviteCollide);
         TEST(testAckFailureAndNitCollide);
         TEST(testAckFailureAndInviteResponse);
         TEST(testAckFailureAndNitResponse);
         TEST(testAckFailureAndAckResponse);
      
      //****************** Oddball UAS ********************//
         TEST(testInvite2543Tid);
         TEST(testNit2543Tid);
         TEST(testAck2543Tid);

// Non-invite tests
         TEST(testNonInviteWithInviteCollision);
         TEST(testNonInviteBusy);
#ifdef USE_SSL
         TEST(testNonInviteTLS);
#endif
         TEST(testNonInviteSpiral);
         TEST(testNonInviteNoContacts);
         TEST(testNonInvite4xxResponse);
         TEST(testNonInvite5xxResponse);
         TEST(testNonInvite6xxResponse);
         TEST(testNonInviteBadAuth);
         TEST(testNonInviteUDPtoTCP);
         TEST(testNonInviteTCPtoUDP);
         TEST(testNonInvite2543Tid);
         TEST(testNonInviteWith180andCancel);
         TEST(testNonInvite200ThenCancel);
         TEST(testNonInvite400ThenCancel);
         TEST(testNonInvite500ThenCancel);
         TEST(testNonInvite407Dropped);
         TEST(testNonInviteDropped);
         TEST(testNonInviteWith180);
         TEST(testNonInviteSpam180);
         TEST(testNonInviteWithAck200);
         TEST(testNonInviteWithAckFailure);
         TEST(testNonInvite200Dropped);
         TEST(testNonInviteSpam200);
         TEST(testNonInvite200then180);
         TEST(testNonInviteSpamRequestBeforeResponse);
         TEST(testNonInviteSpamRequestAfterResponse);
         TEST(testNonInviteWithWrongMethodResponse);
         TEST(testNonInviteWithInviteCollision);
         TEST(testNonInviteClientRetransmissionsWithRecovery);
         TEST(testNonInviteClientRetransmissionsWithTimeout);
         TEST(testNonInviteServerRetransmission);
         TEST(testInfo);
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
         TEST(testOversizeCallIdRegister); //message is getting dropped. Why?
         TEST(testOversizeContactRegister); //message is getting dropped. Why?
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

         TEST(testRegisterBasicWithOutbound);
         TEST(testMultiple1WithOutbound);
         TEST(testMixedExpiresWithOutbound);
         TEST(testThirdPartyRegistrationWithOutbound);
         TEST(testDetailsQValueWithOutbound); 
                   
         TEST(testDetailsExpiresWithOutbound);
         TEST(testRegister407DroppedWithOutbound);
         TEST(testRegisterBogusAuthWithOutbound);
         //TEST(testRegisterLateDigestResponseWithOutbound); //Extremely long test (around 50 minutes).
         TEST(testRegisterClientRetransmitsWithOutbound);
         //TEST(testRegisterNoUserInToWithOutbound); //repro doesn't complain about this yet
         //TEST(testRegisterUserInReqUriWithOutbound); //repro doesn't complain about this yet
         //TEST(testRegisterUnknownAorHostWithOutbound); //repro doesn't complain about this yet
         //TEST(testRegisterUnknownAorUserWithOutbound); //repro doesn't complain about this yet
         TEST(testOversizeCallIdRegisterWithOutbound); //message is getting dropped. Why?
         TEST(testOversizeContactRegisterWithOutbound); //message is getting dropped. Why?
         TEST(testRefreshWithOutbound);
         TEST(testRefreshMultiWithOutbound);
         TEST(testRefreshThirdPartyWithOutbound);
         TEST(testChangeQValueWithOutbound); 
         
         TEST(testSetThenRemoveSpecificWithOutbound);
         TEST(testUnregisterMultiWithOutbound);
         TEST(testUnregisterExpiredWithOutbound);
         TEST(testUnregisterAllBadExpiresWithOutbound);
         //TEST(testUnregisterNonExistentWithOutbound);//repro doesn't complain about this yet
         TEST(testFetchWithOutbound);
         TEST(testExpiryCleanupWithOutbound);
         //TEST(testFetchNonExistentWithOutbound); //repro doesn't complain about this yet

//Proxy tests

         TEST(testInviteBasic);
         TEST(testInviteBasicUpperCaseBranch);
#ifdef USE_SSL
         TEST(testInviteBasicTls);
#endif
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
         TEST(testInviteCancelBefore1xx);
         TEST(testInviteCancelBefore1xxNo487);
         TEST(testInviteCancelBefore1xxNo487or1xx);
         TEST(testInviteNotFound);
         TEST(testInvite488Response);
         TEST(testInvite480Response);
         TEST(testInvite500Response);
         TEST(testInvite503Response);
         TEST(testInvite600Response);
         TEST(testInvite603Response);
         TEST(testInviteServerSpams180);
         TEST(testInviteBogusAuth);

         // Double record-routing tests
         TEST(testInviteUDPToTCPCallerHangsUp);
         TEST(testInviteUDPToTCPCalleeHangsUp);
         TEST(testInviteTCPToUDPCallerHangsUp);
         TEST(testInviteTCPToUDPCalleeHangsUp);

#ifdef USE_SSL
         TEST(testInviteUDPToTLSCallerHangsUp);
         TEST(testInviteUDPToTLSCalleeHangsUp);
         TEST(testInviteTCPToTLSCallerHangsUp);
         TEST(testInviteTCPToTLSCalleeHangsUp);
         TEST(testInviteTLSToUDPCallerHangsUp);
         TEST(testInviteTLSToUDPCalleeHangsUp);
         TEST(testInviteTLSToTCPCallerHangsUp);
         TEST(testInviteTLSToTCPCalleeHangsUp);
#endif

         BUGTEST(testInviteRecursiveRedirect);  // needs to be enabled in proxy settings
         TEST(testSupportedProxyRequire);
         TEST(testUnsupportedProxyRequire);

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
         TEST(testInviteClientSpamsInvite); //tfm is asserting on this test, will look into
         TEST(testInviteClientSpamsAck407);
         TEST(testInviteClientSpamsAck200); // Race in the test
         TEST(testInviteCallerCancelsNo487);
         TEST(testInviteServerRetransmits486);
         TEST(testInviteServerRetransmits503);
         TEST(testInviteServerRetransmits603);
         TEST(testInviteNoDNS); // this requires your dns server to return no answer for unknown lookups
         TEST(testInviteNoDNSTcp);  // this requires your dns server to return no answer for unknown lookups
         TEST(testInviteNoSuchUser);
         TEST(testInviteClientDiesAfterFirstInvite);
         TEST(testInviteClientDiesAfterSecondInvite);
         TEST(testInviteServerDead);
         BADTEST(testInviteLoop);   // This test doesn't work when record routing is enabled because it drivers the message size over our max UDP size of 8k - so we end up getting a 408 response and not a 483
         TEST(testInviteForgedUserInFrom);
         TEST(testInviteCancelCSeq);
         TEST(testInviteUnknownCSeq);
         TEST(testCancelInviteCSeq);
         TEST(testInviteBadAckTid1);
         TEST(testInviteBadAckTid2);
         TEST(testInvite2543Tid);
         TEST(testInvite2543TidWithDigest);
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
         BADTEST(testInviteForkMerges);
         TEST(testInviteForkAllAnswerNo1xx); //tfm is messing this one up; the same test endpoint needs to ACK in three different dialogs, but tfm is unable to handle multiple dialogs resulting from the same request (so, it sends the same ACK three times)
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
         
         TEST(testInviteClientRetransmitsAfter200);
         TEST(testBasic302);
         TEST(testInviteNoAnswerCancel);
         TEST(testInviteNotFoundServerRetransmits);
         TEST(testInviteClientMissedAck2);
         TEST(testInviteForRport);
         TEST(testInviteForkBothAnswerNoProvisional);
         TEST(testAttendedExtensionToExtensionTransfer); // NOTIFY get challenged
         TEST(testBlindTransferExtensionToExtensionHangupImmediately); // reINVITEs get challenged
         TEST(testConferenceConferencorHangsUp); // reINVITEs get challenged
         TEST(testForkedInviteClientLateAck);
         TEST(testInviteForkBothBusy);
         TEST(testEarlyMedia);

         // Tests of the routing pattern matching logic.
         TEST(testRoutingBasic);
         TEST(testTCPMultiMsg);
         // TCP send errors 
         TEST(testTCPPreparseError);
         TEST(testTCPParseBufferError);

         // .bwc. This needs to come last, since it tears down one of the test-
         // user's transports.
         // Note:  Under windows when using WSAPoll - there is a bug in WSAPoll where it won't properly notify of
         //        TCP connection errors.  The implemented work around is to check the socket on the next send.
         //        This means that this test case will fail.
         TEST(testInviteTransportFailure);

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

   initNetwork();
   bool interactive = false;
   try
   {
      CommandLineParser args(argc, argv);
      Log::initialize(args.mLogType, args.mLogLevel, argv[0]);
      resip::Timer::T100 = 0;
      
      if(args.mInteractive)
      {
         interactive = true;
         CommandLineSelector testSelector;
         CppUnit::TextTestRunner testrunner;

         // informs test-listener about testresults
         CPPUNIT_NS::TestResult testresult;
         // register listener for collecting the test-results
         CPPUNIT_NS::TestResultCollector collectedresults;
         testresult.addListener (&collectedresults);
         // Add a listener that displays test progres
         CPTextTestProgressListener progress;
         testresult.addListener( &progress );   

         int numRepetitions = 1;
         if(CppTestSelector::SelectTests(MyTestCase::suite(), testrunner, testSelector, numRepetitions) > 0)
         {
            ReproFixture::initialize(args);

            for(int x=0; x<numRepetitions; x++)
            {
               testrunner.run (testresult);
            }

            ReproFixture::destroyStatic();

            // output results in text-format
            //TextOutputter (TestResultCollector *result, OStream &stream)
            CPPUNIT_NS::TextOutputter textoutputter (&collectedresults, std::cerr);
            textoutputter.write ();
            textoutputter.printStatistics();

            // output results in xml-format
            ofstream testResult(TEST_RESULT_FILE);
            CPPUNIT_NS :: XmlOutputter xmloutputter (&collectedresults, testResult);
            xmloutputter.write ();
            testResult.close();
         }

         DebugLog(<< "Finished");
      }
      else
      {
         TestHolder::createStatic();
         ReproFixture::initialize(args);
      
         CppUnit::TextTestRunner runner;

         runner.addTest( MyTestCase::suite() );
         runner.run();
         DebugLog(<< "Finished: waiting for all transactions to die.");
      
         sleepSeconds(32);

         ReproFixture::destroyStatic();
      } 
   }
   catch (BaseException& e)
   {
      cerr << "Fatal error: " << e << endl;
      exit(-1);
   }

   if(interactive)
   {
      char ch;
      std::cout <<"Press <enter> to exit: ";
      std::cin >>ch;
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
