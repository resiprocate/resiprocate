#include "cppunit/TextTestRunner.h"
#include "cppunit/TextTestResult.h"

#include <signal.h>
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
#include "rutil/Timer.hxx"

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
         assert(msg->isResponse());
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
      removeProxyVias(boost::shared_ptr<SipMessage> msg)
      {
         assert(msg->header(h_Vias).size()>1);
         assert(msg->isResponse());
         Via bottom=msg->header(h_Vias).back();
         Via top=msg->header(h_Vias).front();
         msg->remove(h_Vias);
         msg->header(h_Vias).push_back(top);
         msg->header(h_Vias).push_back(bottom);
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      addProxyVia(boost::shared_ptr<SipMessage> msg)
      {
         assert(msg->header(h_Vias).size()>1);
         assert(msg->isResponse());
         Via top=msg->header(h_Vias).front();
         msg->header(h_Vias).pop_front();
         msg->header(h_Vias).push_front( Via() );
         msg->header(h_Vias).push_front( top );
         return msg;
      }

      static boost::shared_ptr<SipMessage>
      corruptProxyBranch(boost::shared_ptr<SipMessage> msg)
      {
         assert(msg->isResponse());
         // Yes, this is evil. However, BranchParameter does not expose any API
         // for doing this evil, evil thing.
         Data& branch = *const_cast<Data*>(&(msg->header(h_Vias).front().param(p_branch).getTransactionId()));
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

      static boost::shared_ptr<SipMessage>
      missingTid(boost::shared_ptr<SipMessage> msg)
      {
         msg->header(h_Vias).front().remove(p_branch);
         return msg;
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
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/180,contact(jason),WaitForResponse,derek->noAction()),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ring(),jason->answer())),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }

///***************************************** tests start here ********************************//

//***************************** UAC tests ********************************//

//****************** Transaction collision ********************//

// ******************** INVITE ********************//

   void testReflectedInvite()
   {
      WarningLog(<<"*!testReflectedInvite!*");
      refreshRegistration();

      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->reflect(*derek)),
               optional(jason->expect(INVITE/100,from(proxy), WaitFor100, jason->noAction())),
               jason->expect(INVITE/480,from(proxy),WaitForResponse,jason->ack()),
               jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->reflect(*derek,MESSAGE)),
               jason->expect(MESSAGE/480,from(proxy),WaitForResponse,jason->noAction()),
               jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->reflect(*derek,ACK)),
               jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(makeMessage,jason->ok())),
               jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(makeAck,jason->ok())),
               jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->reflect(*derek,INVITE)),
               optional(jason->expect(INVITE/100,from(proxy),WaitFor100,jason->noAction())),
               jason->expect(INVITE/480,from(proxy),WaitForResponse,jason->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAck200ReflectedAsNit()
   {
      WarningLog(<<"*!testAck200ReflectedAsNit!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->reflect(*derek,MESSAGE)),
               jason->expect(MESSAGE/480,from(proxy),WaitForResponse,jason->noAction())
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAck200Reflected()
   {
      WarningLog(<<"*!testAck200Reflected!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->reflect(*derek))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAckFailureReflectedAsInvite()
   {
      WarningLog(<<"*!testAckFailureReflectedAsInvite!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, condition(makeInvite,jason->send200()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAck200NitResponse()
   {
      WarningLog(<<"*!testAck200NitResponse!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, condition(makeInfo,jason->send200()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAckFailureInviteResponse()
   {
      WarningLog(<<"*!testAckFailureInviteResponse!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(fiddleBranchCase,jason->ok())),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testNitBranchCaseAltered()
   {
      WarningLog(<<"*!testNitBranchCaseAltered!*");
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/600,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send600(),jason->ok())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ok(),jason->send600())),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }



//****************** Misbehaving UAS ********************//

   void testInviteUASRemovesProxyVia()
   {
      WarningLog(<<"*!testInviteUASRemovesProxyVia!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/500,from(proxy),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(removeProxyVias,jason->ok()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testInviteUASAddsVia()
   {
      WarningLog(<<"*!testInviteUASAddsVia!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/500,from(proxy),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(addProxyVia,jason->ok()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testInviteUASChangesProxyBranch()
   {
      WarningLog(<<"*!testInviteUASChangesProxyBranch!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/500,from(proxy),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(corruptProxyBranch,jason->ok()))
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testInvite2xxThen1xx()
   {
      WarningLog(<<"*!testInvite2xxThen1xx!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ok(),jason->ring())),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testInvite4xxThen1xx()
   {
      WarningLog(<<"*!testInvite4xxThen1xx!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send400(),jason->ring())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->ok(),jason->send400())),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testInvite4xxThen2xx()
   {
      WarningLog(<<"*!testInvite4xxThen2xx!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send400(),jason->ok())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,chain(jason->send400(),jason->send487(),jason->send400())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,condition(removeTo,jason->ring())),
               jason->expect(INVITE,contact(derek),resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),2*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),4*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),8*resip::Timer::T1+100,jason->noAction()),
               jason->expect(INVITE,contact(derek),16*resip::Timer::T1+100,jason->noAction()),
               optional(jason->expect(INVITE,contact(derek),32*resip::Timer::T1+100,jason->noAction()))
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand,jason->send200())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAckFailureWithResponse()
   {
      WarningLog(<<"*!testAckFailureWithResponse!*");
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/487,contact(jason),WaitForResponse,derek->ack())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send487()),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            ),
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeInvite,derek->retransmit(ack)))),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeMessage,derek->retransmit(ack)))),
               derek->expect(MESSAGE/400,from(proxy),WaitForResponse,derek->noAction())
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeInvite,derek->retransmit(ack)))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAck200AndNitResponse()
   {
      WarningLog(<<"*!testAck200AndNitResponse!*");
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeMessage,derek->retransmit(ack)))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAck200AndAckResponse()
   {
      WarningLog(<<"*!testAck200AndAckResponse!*");
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }


   void testAckFailureAndInviteCollide()
   {
      WarningLog(<<"*!testAckFailureAndInviteCollide!*");
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeInvite,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),condition(makeMessage,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeInvite,derek->retransmit(ack)))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,condition(makeMessage,derek->retransmit(ack)))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
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
      refreshRegistration();
      boost::shared_ptr<SipMessage> ack;
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/400,contact(jason),WaitForResponse,chain(ack<=derek->ack(),rawcondition(make200Response,derek->retransmit(ack))))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->send400()),
               jason->expect(ACK,from(proxy),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
      ExecuteSequences();
   }



//****************** Oddball UAS ********************//

   void testInvite2543TidWithDigest()
   {
      WarningLog(<<"*!testInvite2543TidWithDigest!*");

      //Registering Derek
      Seq(jason->registerUser(60, jason->getDefaultContacts()),
          jason->expect(REGISTER/401, from(proxy), WaitForResponse, jason->digestRespond()),
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


   void testInvite2543Tid()
   {
      WarningLog(<<"*!testInvite2543Tid!*");
      refreshRegistration();
      
      Seq
      (
         derek->invite(*jason),
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


   void testNit2543Tid()
   {
      WarningLog(<<"*!testNit2543Tid!*");
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
      refreshRegistration();
      Seq
      (
         derek->invite(*jason),
         And
         (
            Sub
            (
               optional(derek->expect(INVITE/100,from(proxy),WaitFor100,derek->noAction())),
               derek->expect(INVITE/200,contact(jason),WaitForResponse,condition(missingTid,derek->ack()))
            ),
            Sub
            (
               jason->expect(INVITE,contact(derek),WaitForCommand,jason->ok()),
               jason->expect(ACK,contact(derek),WaitForCommand, jason->noAction())
            )
         ),
         WaitForEndOfTest
      );
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
      static CppUnit::Test* suite(bool redirectServer)
      {
         CppUnit::TestSuite *suiteOfTests = new CppUnit::TestSuite( "Suite1" );

         TEST(testReflectedInvite);
         TEST(testProxyAlive);
      
      
         TEST(testInviteReflectedAsNonInvite);
         TEST(testProxyAlive);
      
      
         TEST(testInviteReflectedAsAck);
         TEST(testProxyAlive);
      
      
         TEST(testInviteNonInviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testInviteAckResponse);
         TEST(testProxyAlive);
      
      
      
      // ******************** non-INVITE ********************//
      
         TEST(testNitReflectedAsInvite);
         TEST(testProxyAlive);
      
      
         TEST(testNitReflected);
         TEST(testProxyAlive);
      
      
         TEST(testNitReflectedAsDifferentNit);
         TEST(testProxyAlive);
      
      
         TEST(testNitReflectedAsAck);
         TEST(testProxyAlive);
      
      
         TEST(testNitInviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testNitDifferentNitResponse);
         TEST(testProxyAlive);
      
      
         TEST(testNitAckResponse);
         TEST(testProxyAlive);
      
      
      
      // ******************** ACK ********************//
      
         TEST(testAck200ReflectedAsInvite);
         TEST(testProxyAlive);
      
      
         TEST(testAck200ReflectedAsNit);
         TEST(testProxyAlive);
      
      
         TEST(testAck200Reflected);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureReflectedAsInvite);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureReflectedAsNit);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureReflected);
         TEST(testProxyAlive);
      
      
         TEST(testAck200InviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAck200NitResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureInviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureNitResponse);
         TEST(testProxyAlive);
      
      
      //****************** Oddball UAS ********************//
      
         TEST(testInviteBranchCaseAltered);
         TEST(testProxyAlive);
      
      
         TEST(testNitBranchCaseAltered);
         TEST(testProxyAlive);
      
      
         TEST(testInvite6xxThen2xx);
         TEST(testProxyAlive);
      
      
         TEST(testInvite2xxThen6xx);
         TEST(testProxyAlive);
      
      
      
      //****************** Misbehaving UAS ********************//
      
         TEST(testInviteUASRemovesProxyVia);
         TEST(testProxyAlive);
      
      
         TEST(testInviteUASAddsVia);
         TEST(testProxyAlive);
      
      
         TEST(testInviteUASChangesProxyBranch);
         TEST(testProxyAlive);
      
      
         TEST(testInvite2xxThen1xx);
         TEST(testProxyAlive);
      
      
         TEST(testInvite4xxThen1xx);
         TEST(testProxyAlive);
      
      
         TEST(testInvite2xxThen4xx);
         TEST(testProxyAlive);
      
      
         TEST(testInvite4xxThen2xx);
         TEST(testProxyAlive);
      
      
         TEST(testInviteMultiple4xx);
         TEST(testProxyAlive);
      
      
         TEST(testInviteMalformed1xxWithTimeout);
         TEST(testProxyAlive);
      
      
         TEST(testAck200WithResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureWithResponse);
         TEST(testProxyAlive);
      
      
         TEST(testNitUASRemovesProxyVia);
         TEST(testProxyAlive);
      
      
         TEST(testNitUASAddsVia);
         TEST(testProxyAlive);
      
      
         TEST(testNitUASChangesProxyBranch);
         TEST(testProxyAlive);
      
      
         TEST(testNit2xxThen1xx);
         TEST(testProxyAlive);
      
      
         TEST(testNit4xxThen1xx);
         TEST(testProxyAlive);
      
      
         TEST(testNit2xxThen4xx);
         TEST(testProxyAlive);
      
      
         TEST(testNit4xxThen2xx);
         TEST(testProxyAlive);
      
      
         TEST(testNitMultiple4xx);
         TEST(testProxyAlive);
      
      
         TEST(testNitMalformed1xxWithTimeout);
         TEST(testProxyAlive);
      

//***************************** UAS tests ********************************//
      
      //****************** Transaction collision ********************//
      
      // ******************** INVITE ********************//
      
         TEST(testInviteAndNitCollide);
         TEST(testProxyAlive);
      
      
         TEST(testInviteAndAckCollide);
         TEST(testProxyAlive);
      
      
         TEST(testInviteAndResponse);
         TEST(testProxyAlive);
      
      
         TEST(testInviteAndNitResponse);
         TEST(testProxyAlive);
      
      
         TEST(testInviteAndAckResponse);
         TEST(testProxyAlive);
      
      
      
      // ******************** non-INVITE ********************//
      
         TEST(testNitAndInviteCollide);
         TEST(testProxyAlive);
      
      
         TEST(testNitAndDifferentNitCollide);
         TEST(testProxyAlive);
      
      
         TEST(testNitAndAckCollide);
         TEST(testProxyAlive);
      
      
         TEST(testNitAndInviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testNitAndResponse);
         TEST(testProxyAlive);
      
      
         TEST(testNitAndDifferentNitResponse);
         TEST(testProxyAlive);
      
      
         TEST(testNitAndAckResponse);
         TEST(testProxyAlive);
      
      
      
      // ******************** ACK ********************//
      
         TEST(testAck200AndInviteCollide);
         TEST(testProxyAlive);
      
      
         TEST(testAck200AndNitCollide);
         TEST(testProxyAlive);
      
      
         TEST(testAck200AndInviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAck200AndNitResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAck200AndAckResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureAndInviteCollide);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureAndNitCollide);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureAndInviteResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureAndNitResponse);
         TEST(testProxyAlive);
      
      
         TEST(testAckFailureAndAckResponse);
         TEST(testProxyAlive);
      
      
      
      //****************** Oddball UAS ********************//
      
         TEST(testInvite2543Tid);
         TEST(testProxyAlive);
      
      
         TEST(testNit2543Tid);
         TEST(testProxyAlive);
      
      
         TEST(testAck2543Tid);
         TEST(testProxyAlive);

         return suiteOfTests;
      }
};

int main(int argc, char** argv)
{
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
