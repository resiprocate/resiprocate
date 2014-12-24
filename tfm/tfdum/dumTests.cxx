//#include "cppunit/TextTestRunner.h"
//#include "cppunit/TextTestResult.h"

#include <signal.h>

#include <boost/bind.hpp>

#include <cppunit/extensions/HelperMacros.h>
#include <cppunit/extensions/TestFactoryRegistry.h>
#include <cppunit/TextOutputter.h>
#include <cppunit/XmlOutputter.h>
#include <cppunit/TestResult.h>
#include <cppunit/Test.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TextTestRunner.h>
#include "tfm/CppTestSelector.hxx"
#include "tfm/CPTextTestProgressListener.hxx"

#include "DumUserAgent.hxx"
#include "DumFixture.hxx"
#include "DumUaAction.hxx"

#include "rutil/Logger.hxx"
#include "rutil/Random.hxx"
#include "tfm/repro/CommandLineParser.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestProxy.hxx"
#include "tfm/TestUser.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/predicates/ExpectUtils.hxx"
#include "tfm/repro/TestReproUser.hxx"

#include "resip/stack/Helper.hxx"
#include "resip/stack/HeaderFieldValue.hxx"
#include "resip/stack/Pidf.hxx"
#include "resip/stack/MessageWaitingContents.hxx"
#include "resip/stack/PlainContents.hxx"

#include "resip/stack/Pidf.hxx"

#include "resip/dum/DialogEventStateManager.hxx"

#include "tfm/tfdum/TestClientSubscription.hxx"
#include "tfm/tfdum/TestServerSubscription.hxx"
#include "tfm/tfdum/TestInviteSession.hxx"
#include "tfm/tfdum/TestClientPagerMessage.hxx"
#include "tfm/tfdum/TestServerPagerMessage.hxx"
#include "tfm/tfdum/TestDialogEvent.hxx"

#include "tfm/TfmHelper.hxx"
#include "tfm/CheckPrivacy.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

#define TEST_RESULT_FILE "./testResult.xml"

using namespace CppUnit;
using namespace resip;

static const int WaitFor100 = 1000;
static const int WaitFor180 = 1000;
static const int WaitFor487 = 1000;
static const int WaitForAck = 1000;  //immediate ACK for 4xx and CANCEL; not ACK for 200
static const int WaitForCommand = 1000;
static const int WaitForResponse = 1000;
static const int WaitForRegistration = 5000;
static const int PauseTime = 100;
static const int WaitForPause = 1100;
static const int WaitForEndOfTest = 1000;
static const int WaitForEndOfSeq = 1000;
static const int Seconds = 1000;
static const int CallTimeout = 40 * 1000;
static const int TimeOut = 5000;
static const int Owner491 = 5000;

const Data transport("udp");
static NameAddr localhost;

class DumTestCase : public DumFixture
{
   CPPUNIT_TEST_SUITE( DumTestCase ); 
#if 0
      // this test relies on a really long sleep, so don't run it unless you wanna wait:
      CPPUNIT_TEST(testSessionTimerUasRefresher);
#else
      CPPUNIT_TEST(testDialogEventUacBasic);
      CPPUNIT_TEST(testDialogEventUasBasic);
      //CPPUNIT_TEST(testDialogEventUacTwoBranchesProvisional);  // test has commented out code and will cause onStaleCallTimeout to fire later in the middle of another test - commenting out until fixed
      CPPUNIT_TEST(testDialogEvent302Uac);
      CPPUNIT_TEST(testDialogEvent302Uas);
      CPPUNIT_TEST(testDialogEventReferUas);
      CPPUNIT_TEST(testDialogEventCancelUas);
      CPPUNIT_TEST(testDialogEventCancelUac);
      CPPUNIT_TEST(testDialogEventReplaces);
      CPPUNIT_TEST(testDialogEventInvite500);

      // UAC Prack Scenarios - only UAC is DUM endpoint
      CPPUNIT_TEST(testUACPrackEmpty180rel);
      CPPUNIT_TEST(testUACPrackMultiple180rel);
      CPPUNIT_TEST(testUACPrackInviteBasicSuccess);
      CPPUNIT_TEST(testUACPrackUpdate);
      CPPUNIT_TEST(testUACPrackRetrans);
      CPPUNIT_TEST(testUACPrackOptionTag);
      CPPUNIT_TEST(testUACPrackUasUpdate);
      CPPUNIT_TEST(testUACPrackUasDoubleUpdate);
      CPPUNIT_TEST(testUACPrackUac491);
      CPPUNIT_TEST(testUACPrackUas491);

      // 3GPP PRACK Scenarios - both UAC and UAS as DUM endpoints
      CPPUNIT_TEST(testPrack3GPP_1);  // 3GPP 24.930 Rel 11 - 5.1.2, 5.2.2
      CPPUNIT_TEST(testPrack3GPP_2);  // 3GPP 24.930 Rel 11 - 5.2.3, 5.3.4
      CPPUNIT_TEST(testPrack3GPP_3);  // 3GPP 24.930 Rel 11 - 5.3.2, 5.5.2, 5.6.2
      CPPUNIT_TEST(testPrack3GPP_4);  // 3GPP 24.930 Rel 11 - 5.3.3, 5.6.2
      CPPUNIT_TEST(testPrack3GPP_5);  // 3GPP 24.930 Rel 11 - 5.4.2, 5.4.4
      CPPUNIT_TEST(testPrack3GPP_6);  // 3GPP 24.930 Rel 11 - 5.4.3

      // Other Prack Scenarios - both UAC and UAS as DUM endpoints
      CPPUNIT_TEST(testPrackInviteNoOffer);
      CPPUNIT_TEST(testPrackInviteNoOffer2ndProvisionalOffer);
      CPPUNIT_TEST(testPrackInviteOffer2ndReliableProvisionalAnswer);
      CPPUNIT_TEST(testPrackInviteOffer2ndRelProvAnswerPrackOffer);
      CPPUNIT_TEST(testPrackNegotiatedReliableProvisional);
      CPPUNIT_TEST(testPrackNegotiatedReliableUASUpdateFast);
      CPPUNIT_TEST(testPrackNegotiatedReliableUpdates);
      CPPUNIT_TEST(testPrackNegotiatedReliableUpdateGlare);
      CPPUNIT_TEST(testPrack1xxResubmission);  // takes 2.5 mintues to run
      CPPUNIT_TEST(testPrackRequiredWithSupportedEssential);

      // UAS Prack Scenarios - only UAS is DUM endpoint
      CPPUNIT_TEST(testUASPrackNegotiatedReliableUpdateGlareResend);
      CPPUNIT_TEST(testUASPrackNegotiatedReliableUpdateGlareCrossed);
      CPPUNIT_TEST(testUASPrackNegotiatedReliableUACDoubleUpdate);
      CPPUNIT_TEST(testUASPrackNegotiatedReliableOfferInPrack);
      CPPUNIT_TEST(testUASPrack18xRetransmissions);  // takes ~128 seconds to run when T1 timer is set to 2000ms
      CPPUNIT_TEST(testUASPrackStrayPrack);
      CPPUNIT_TEST(testUASPrack2ndOfferInNoAnswerReliableWaitingPrack);
      CPPUNIT_TEST(testUASPrack2ndOfferInOfferReliableProvidedAnswer);

      //CPPUNIT_TEST(testReinviteWithByeSentAfterMissedAck);  // comment above indicates test case needs handling for retransmissions

      CPPUNIT_TEST(testOutOfDialogReferNoReferSubWithoutReferSubHeader);
      CPPUNIT_TEST(testOutOfDialogRefer);
      CPPUNIT_TEST(testOutOfDialogReferNoReferSub);
      CPPUNIT_TEST(testOutOfDialogRefer406);
      CPPUNIT_TEST(testAttendedTransferTransferorAsUAS);
      CPPUNIT_TEST(testAttendedTransferTransferorAsUAC);
      CPPUNIT_TEST(testBlindTransferWithOneNotify);
      CPPUNIT_TEST(testInviteWithPrivacy);
      CPPUNIT_TEST(test3pccFlow1NoAnswer);
      CPPUNIT_TEST(test3pccFlow1Reject);
      CPPUNIT_TEST(testBlindTransfer);
      CPPUNIT_TEST(testBlindTransferRejected);

      CPPUNIT_TEST(testInvite302);
      CPPUNIT_TEST(testInviteBusy);
      CPPUNIT_TEST(testInviteLateMediaSuccess);
      CPPUNIT_TEST(testInviteLateMediaFailure);
      CPPUNIT_TEST(testInviteUnsuccessfulNoAnswer);
      CPPUNIT_TEST(testInviteUnsuccessfulTempUnavailable);
      CPPUNIT_TEST(testReInviteIpChange);
      CPPUNIT_TEST(testRegisterBasicWithoutRinstance);
      CPPUNIT_TEST(testReinviteLateMedia);
      CPPUNIT_TEST(testReinviteRejected); 
      CPPUNIT_TEST(testTransferNoReferSub);
      CPPUNIT_TEST(testTransferNoReferSubWithoutReferSubHeaderIn202);
      CPPUNIT_TEST(testInviteBasicSuccess);
      CPPUNIT_TEST(testKPML);
      CPPUNIT_TEST(testLucentAuthIssue);
      CPPUNIT_TEST(testPublish423);
      CPPUNIT_TEST(testPublishBasic);
      CPPUNIT_TEST(testPublishRefresh);
      CPPUNIT_TEST(testPublishUpdate); 

      CPPUNIT_TEST(testRegisterBasicWithMethodsParam);
      CPPUNIT_TEST(testRegisterBasicWithoutMethodsParam);
      // CPPUNIT_TEST(testRegisterBasicWithRinstance);  // An instance id is set in DumUserAgent::makeProfile - so rinstance won't be used
      // CPPUNIT_TEST(testServiceRoute); //could be correct; see draft-rosenberg-sip-route-construct-02.txt. Desired behaviour needs to be determined.
      
      CPPUNIT_TEST(testStaleNonceHandling);
      CPPUNIT_TEST(testSubscribe);
      CPPUNIT_TEST(testSubscribe406);
      CPPUNIT_TEST(testSubscribeRequestRefresh);
      CPPUNIT_TEST(testSubscribeQueuedRequestRefresh);
      CPPUNIT_TEST(testSubscribeRequestRefreshBetweenDumUaAndTestEndPoint);
      CPPUNIT_TEST(testIMBasic);
      CPPUNIT_TEST(testIMMergedRequestArrivedSameTime);
      CPPUNIT_TEST(testIMMergedRequestArrivedWithin32Seconds);
      CPPUNIT_TEST(testIMMergedRequestArrivedMoreThan32SecondsApart);
      CPPUNIT_TEST(testInDialogSubscribeSuccess);
      CPPUNIT_TEST(testUASSubscribeCreatingDialogFromInvite);   // Works on Windows. Uncomment once works under Linux.
      CPPUNIT_TEST(testInDialogSubscribeWithoutEventHandler);
      CPPUNIT_TEST(testOutOfDialogSubscribeWithoutEventHandler);
      CPPUNIT_TEST(testReInviteRemoveVideo);
      CPPUNIT_TEST(testByesCrossedOnTheWire);  
      CPPUNIT_TEST(testReinviteNo200);  // Requires record routing
#endif
      CPPUNIT_TEST_SUITE_END(); 

   public:

///***************************************** tests start here ********************************//


      class SessionTimerControl
      {
         public:
            enum Mode 
            {
               None,
               Uac, 
               Uas
            };
                           
            SessionTimerControl(Mode mode, int sessionExpires, int minExpires=90) :
               mMode(mode),
               mSessionExpires(sessionExpires),
               mMinExpires(minExpires)
            {
            }
               
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg)
            {
               msg->header(h_MinSE).value() = mMinExpires;
               msg->header(h_SessionExpires).value() = mSessionExpires;
               if (mMode != None)
               {
                  msg->header(h_SessionExpires).param(p_refresher) = toString(mMode);               
               }
               return msg;               
            }
            
            Data toString(Mode m)
            {
               switch(m)
               {
                  case None:
                     resip_assert(0);
                     return "Broken";                     
                  case Uac:
                     return "uac";
                  case Uas:
                     return "uas";
               }
            }
            
         private:
            //boost::shared_ptr<resip::SipMessage> mSource;
            //bool mStale;            
            Mode mMode;
            int mSessionExpires;
            int mMinExpires;
      };
      

      void testSessionTimerUasRefresher()
      {
         InfoLog(<< "testSessionTimerUasRefresher");

         jason->getProfile()->addSupportedOptionTag(Token(Symbols::Timer));
         
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, 
                            condition(SessionTimerControl(SessionTimerControl::Uas, 90), sheila->answer(answer))),
             jason->expect(Invite_NewClientSession, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         sleepSeconds(90);

         Seq(sheila->reInvite("jason", answer),
             uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardOffer)),
             sheila->expect(INVITE/200, from(jason->getInstanceId()), WaitForResponse, sheila->ack()),
             WaitForEndOfSeq);
         
         ExecuteSequences();         
         
         //jason->getProfile()->removeSupportedOptionTag(Token(Symbols::Timer));
         jason->getProfile()->clearSupportedOptionTags();
      }

      class DialogEventInfoRemoteTagUpdater;
      class ExpectedDialogEventInfoBuilderUas;

      class TestDialogEventInfo : public DialogEventInfo
      {
         public:
         TestDialogEventInfo() : DialogEventInfo()
            {
            }
               
         TestDialogEventInfo(const TestDialogEventInfo& rhs) : DialogEventInfo(rhs)
            {
            }
            
         private:
         friend class DumTestCase;
         friend class DumTestCase::DialogEventInfoRemoteTagUpdater;
         friend class DumTestCase::ExpectedDialogEventInfoBuilderUas;
      };

      class DialogEventInfoRemoteTagUpdater : public ExpectAction 
      {
         public:
            DialogEventInfoRemoteTagUpdater(TestDialogEventInfo* toUpdate,
                                        const boost::shared_ptr<resip::SipMessage>& updatedMsg) :
               mToUpdate(toUpdate),
               mUpdatedMsg(updatedMsg)
            {
            }
            virtual void operator()(boost::shared_ptr<Event> event)
            {
               mToUpdate->mDialogId = DialogId(mUpdatedMsg->header(h_CallID).value(),
                                               mUpdatedMsg->header(h_From).param(p_tag),
                                               mUpdatedMsg->header(h_To).param(p_tag));
            }
            virtual resip::Data toString() const { return "DialogEventInfoRemoteTagUpdater";}
      private:
         TestDialogEventInfo* mToUpdate;
         const boost::shared_ptr<resip::SipMessage>& mUpdatedMsg;
      };

      class InviteHelper
      {
      public:
         InviteHelper(SharedPtr<SipMessage> invite) : mInvite(invite) {}
         SharedPtr<SipMessage> getInvite() const { return mInvite; }
      private:
         SharedPtr<SipMessage> mInvite;
      };

      void testDialogEventUacBasic()
      {
         InfoLog(<< "testDialogEventUacBasic");

         // 1) create a couple of endpoints that will participate in a basic call (dial/answer/hangup)
         // 2) monitor the dialog events generated by one (or both?) of them
         // 3) do the test call, and watch the generated dialog events to ensure they match what we expect

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestSipEndPoint* sheila = sipEndPoint;
         TestDialogEvent uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         SharedPtr<SipMessage> inviteMsg = jason->getDum().makeInviteSession(sheila->getContact(), standardOffer);

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di1.mLocalIdentity = inviteMsg->header(h_From);
         di1.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di1.mRemoteIdentity = inviteMsg->header(h_To);

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di2(di1);
         di2.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di2.mState = DialogEventInfo::Early;

         TestDialogEventInfo di3(di2);
         di3.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));
         di3.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di3.mState = DialogEventInfo::Confirmed;

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> sheilasRing;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);
         DialogEventPred* pDi3 = new DialogEventPred(di3);
         DialogEventPred* pDi4 = new DialogEventPred(di4);

         InviteHelper ih1(inviteMsg);

         Seq(new DumUaSendingCommand(jason, boost::bind(&InviteHelper::getInvite, ih1)),
             uac.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, 
                            chain(save(sheilasRing, sheila->ring()), 
                                  new DialogEventInfoRemoteTagUpdater(&di2, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di3, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di4, sheilasRing))),
             jason->expect(Invite_NewClientSession, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(DialogEvent_Early, *pDi2, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->answer(answer)),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(DialogEvent_Confirmed, *pDi3, WaitForCommand, uac.noAction()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->bye()))),
             And(
                Sub(jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                Sub(uac.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::RemoteBye, WaitForCommand, uac.noAction())),
                Sub(sheila->expect(BYE/200, from(jason->getInstanceId()), WaitForCommand, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testDialogEventInvite500()
      {
         InfoLog(<< "testDialogEventInvite500");

         // 1) create a couple of endpoints that will participate in a basic call (dial/answer/hangup)
         // 2) monitor the dialog events generated by one (or both?) of them
         // 3) do the test call, and watch the generated dialog events to ensure they match what we expect

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestSipEndPoint* sheila = sipEndPoint;
         TestDialogEvent uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         SharedPtr<SipMessage> inviteMsg = jason->getDum().makeInviteSession(sheila->getContact(), standardOffer);

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di1.mLocalIdentity = inviteMsg->header(h_From);
         di1.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di1.mRemoteIdentity = inviteMsg->header(h_To);

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di2(di1);
         di2.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di2.mState = DialogEventInfo::Terminated;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);

         InviteHelper ih1(inviteMsg);

         Seq(new DumUaSendingCommand(jason, boost::bind(&InviteHelper::getInvite, ih1)),
             uac.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, chain(sheila->send100(),sheila->send500())),
             And(Sub(uac.expect(DialogEvent_Terminated, *pDi2, InviteSessionHandler::Error, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Failure, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, new TestSipEndPoint::AlwaysMatches(), WaitForCommand, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testDialogEventCancelUac()
      {
         InfoLog(<< "testDialogEventCancelUac");

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestSipEndPoint* sheila = sipEndPoint;
         TestDialogEvent testDialogEvt(jason);
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         SharedPtr<SipMessage> inviteMsg = jason->getDum().makeInviteSession(sheila->getContact(), standardOffer);

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di1.mLocalIdentity = inviteMsg->header(h_From);
         di1.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di1.mRemoteIdentity = inviteMsg->header(h_To);

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di2(di1);
         di2.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di2.mState = DialogEventInfo::Early;

         TestDialogEventInfo di3(di2);
         di3.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));
         di3.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di3.mState = DialogEventInfo::Confirmed;

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> sheilasRing;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);
//         DialogEventPred* pDi3 = new DialogEventPred(di3);
         DialogEventPred* pDi4 = new DialogEventPred(di4);

         InviteHelper ih1(inviteMsg);

         Seq(new DumUaSendingCommand(jason, boost::bind(&InviteHelper::getInvite, ih1)),
             testDialogEvt.expect(DialogEvent_Trying, *pDi1, WaitForCommand, testDialogEvt.noAction()),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, 
                            chain(sheila->send100(),
                                  save(sheilasRing, sheila->ring()), 
                                  new DialogEventInfoRemoteTagUpdater(&di2, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di3, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di4, sheilasRing))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, testDialogEvt.noAction()),
             testDialogEvt.expect(DialogEvent_Early, *pDi2, WaitForCommand, testDialogEvt.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         Seq(jason->cancelInvite(),
             And(Sub(sheila->expect(CANCEL, new TestSipEndPoint::AlwaysMatches(), WaitForCommand, chain(sheila->ok(), sheila->send487())),
                     sheila->expect(ACK, new TestSipEndPoint::AlwaysMatches(), WaitForCommand, sheila->noAction())),
                 Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::LocalCancel, WaitForCommand, testDialogEvt.noAction()),
                     jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()))),
             //jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             //uac.expect(DialogEvent_Confirmed, *pDi3, WaitForCommand, uac.noAction()),
             //And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
             //    Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->bye()))),
             //And(
             //   Sub(jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
             //   Sub(uac.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::RemoteBye, WaitForCommand, uac.noAction())),
             //   Sub(sheila->expect(BYE/200, from(jason->getInstanceId()), WaitForCommand, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testDialogEventUacTwoBranchesProvisional()
      {
         InfoLog(<< "testDialogEventUacTwoBranchesProvisional");

         // 1) create a couple of endpoints that will participate in a basic call (dial/answer/hangup)
         // 2) monitor the dialog events generated by one (or both?) of them
         // 3) do the test call, and watch the generated dialog events to ensure they match what we expect

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestSipEndPoint* sheila = sipEndPoint;
         TestDialogEvent uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         SharedPtr<SipMessage> inviteMsg = jason->getDum().makeInviteSession(sheila->getContact(), standardOffer);

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di1.mLocalIdentity = inviteMsg->header(h_From);
         di1.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di1.mRemoteIdentity = inviteMsg->header(h_To);

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di2(di1);
         di2.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di2.mState = DialogEventInfo::Early;

         TestDialogEventInfo di2a(di1);
         di2a.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di2a.mState = DialogEventInfo::Early;

         TestDialogEventInfo di3(di2);
         di3.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));
         di3.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di3.mState = DialogEventInfo::Confirmed;

         TestDialogEventInfo di3a(di2a);
         di3a.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));
         di3a.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di3a.mState = DialogEventInfo::Confirmed;

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Terminated;

         TestDialogEventInfo di4a(di3a);
         di4a.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> sheilasRing;
         boost::shared_ptr<SipMessage> sheilasSecondRing;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);
         DialogEventPred* pDi2a = new DialogEventPred(di2a);
//         DialogEventPred* pDi3 = new DialogEventPred(di3);
//         DialogEventPred* pDi4 = new DialogEventPred(di4);
//         DialogEventPred* pDi4a = new DialogEventPred(di4a);

         InviteHelper ih1(inviteMsg);

         Seq(new DumUaSendingCommand(jason, boost::bind(&InviteHelper::getInvite, ih1)),
             uac.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, 
                            chain(save(sheilasRing, sheila->ring()), 
                                  new DialogEventInfoRemoteTagUpdater(&di2, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di3, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di4, sheilasRing),
                                  save(sheilasSecondRing, sheila->ringNewBranch()),
                                  new DialogEventInfoRemoteTagUpdater(&di2a, sheilasSecondRing),
                                  new DialogEventInfoRemoteTagUpdater(&di3a, sheilasSecondRing),
                                  new DialogEventInfoRemoteTagUpdater(&di4a, sheilasSecondRing))),
             jason->expect(Invite_NewClientSession, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(DialogEvent_Early, *pDi2, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->noAction()),
             jason->expect(Invite_NewClientSession, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(DialogEvent_Early, *pDi2a, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->noAction()),
             //jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             //uac.expect(DialogEvent_Confirmed, *pDi3, WaitForCommand, uac.noAction()),
             //And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
             //    Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->bye()))),
             //And(
             //   Sub(jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
             //   Sub(uac.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::RemoteBye, WaitForCommand, uac.noAction())),
             //   Sub(sheila->expect(BYE/200, from(jason->getInstanceId()), WaitForCommand, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testDialogEvent302Uac()
      {
         InfoLog(<< "testDialogEvent302Uac");

         // 1) create a couple of endpoints that will participate in a basic call (dial/answer/hangup)
         // 2) monitor the dialog events generated by one (or both?) of them
         // 3) do the test call, and watch the generated dialog events to ensure they match what we expect

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestSipEndPoint* sheila = sipEndPoint;
         TestDialogEvent uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         SharedPtr<SipMessage> inviteMsg = jason->getDum().makeInviteSession(sheila->getContact(), standardOffer);

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di1.mLocalIdentity = inviteMsg->header(h_From);
         di1.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di1.mRemoteIdentity = inviteMsg->header(h_To);

         Uri contactDerek = derek->getAor().uri();
         contactDerek.host() = derek->getIp();
         contactDerek.port() = derek->getPort();

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di2(di1);
         di2.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di2.mState = DialogEventInfo::Trying;

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di3(di1);
         di3.mState = DialogEventInfo::Terminated;
         di3.mRemoteTarget = std::auto_ptr<Uri>(new Uri("sip:derek@localhost"));

         TestDialogEventInfo di4(di2);
         di4.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> sheilasRedirect;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);
         DialogEventPred* pDi3 = new DialogEventPred(di3);
         DialogEventPred* pDi4 = new DialogEventPred(di4);

         // when DUM processes a 3xx response it generates an INVITE to the new target
         // using the same CallID, To and From headers as in the original INVITE;
         // therefore we'll treat the new INVITE as belonging to the same DialogSet as
         // the original INVITE

         InviteHelper ih1(inviteMsg);

         Seq(new DumUaSendingCommand(jason, boost::bind(&InviteHelper::getInvite, ih1)),
             uac.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->send302(derek->getAor().uri())),
             And(Sub(uac.expect(DialogEvent_Terminated, *pDi3, InviteSessionHandler::Rejected, WaitForCommand, uac.noAction()),
                     uac.expect(DialogEvent_Trying, *pDi2, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, new TestSipEndPoint::AlwaysMatches(), WaitForResponse, sheila->noAction()))),
             uac.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::Rejected, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Failure, *TestEndPoint::AlwaysTruePred, WaitForResponse, uac.noAction()),
             jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             //sheila->expect(ACK, new TestSipEndPoint::AlwaysMatches(), WaitForResponse, sheila->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testDialogEventReferUas()
      {
         InfoLog(<< "testDialogEventReferUas");

         // 1) create a couple of endpoints that will participate in a basic call (dial/answer/hangup)
         // 2) monitor the dialog events generated by one (or both?) of them
         // 3) do the test call, and watch the generated dialog events to ensure they match what we expect

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestSipEndPoint* sheila = sipEndPoint;
         TestDialogEvent testDialogEvt(jason);
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         SharedPtr<SipMessage> inviteMsg = jason->getDum().makeInviteSession(sheila->getContact(), standardOffer);

         Uri contactDerek = derek->getAor().uri();
         contactDerek.host() = derek->getIp();
         contactDerek.port() = derek->getPort();

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId(DialogSetId(*inviteMsg), Data::Empty);
         di1.mLocalIdentity = inviteMsg->header(h_From);
         di1.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di1.mRemoteIdentity = inviteMsg->header(h_To);

         // this gets updated with the correct remote tag after sheila answers...
         TestDialogEventInfo di2(di1);
         di2.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di2.mState = DialogEventInfo::Early;

         TestDialogEventInfo di3(di2);
         di3.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));
         di3.mRemoteTarget = std::auto_ptr<Uri>(new Uri(sheila->getContact().uri()));
         di3.mState = DialogEventInfo::Confirmed;

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> inviteFromRefer;

         TestDialogEventInfo di5;
         di5.mDialogEventId = "*";
         di5.mDirection = DialogEventInfo::Initiator;
         di5.mInviteSession = InviteSessionHandle::NotValid();
         di5.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di5.mDialogId = DialogId("*", "*", "*");
         di5.mLocalIdentity = inviteMsg->header(h_From);
         di5.mLocalTarget = inviteMsg->header(h_Contacts).front().uri();
         di5.mRemoteIdentity = NameAddr(contactDerek);
         di5.mReferredBy = std::auto_ptr<NameAddr>(new NameAddr(di1.mRemoteIdentity));

         boost::shared_ptr<SipMessage> sheilasRing;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);
         DialogEventPred* pDi3 = new DialogEventPred(di3);
         DialogEventPred* pDi4 = new DialogEventPred(di4);
         DialogEventPred* pDi5 = new DialogEventPred(di5);

         // when DUM processes a 3xx response it generates an INVITE to the new target
         // using the same CallID, To and From headers as in the original INVITE;
         // therefore we'll treat the new INVITE as belonging to the same DialogSet as
         // the original INVITE

         InviteHelper ih1(inviteMsg);

         Seq(new DumUaSendingCommand(jason, boost::bind(&InviteHelper::getInvite, ih1)),
             testDialogEvt.expect(DialogEvent_Trying, *pDi1, WaitForCommand, testDialogEvt.noAction()),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, 
                            chain(save(sheilasRing, sheila->ring()), 
                                  new DialogEventInfoRemoteTagUpdater(&di2, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di3, sheilasRing),
                                  new DialogEventInfoRemoteTagUpdater(&di4, sheilasRing))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             testDialogEvt.expect(DialogEvent_Early, *pDi2, WaitForCommand, testDialogEvt.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->answer(answer)),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             testDialogEvt.expect(DialogEvent_Confirmed, *pDi3, WaitForCommand, testDialogEvt.noAction()),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->refer(jason->getAor().uri(), contactDerek)))),
             //sheila->expect(REFER, from(jason->getInstanceId()), WaitForCommand, sheila->send202()),
             jason->expect(Invite_Refer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.acceptRefer(202)),
             sheila->expect(REFER/202, from(jason->getInstanceId()), WaitForResponse, sheila->bye()),
             And(
                Sub(jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, 
                       jason->inviteFromRefer(uac.getReferMessage(), uac.getServerSubscription(), standardOffer)),
                    testDialogEvt.expect(DialogEvent_Trying, *pDi5, WaitForCommand, testDialogEvt.noAction())),
                Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::RemoteBye, WaitForCommand, uac.noAction())),
                Sub(sheila->expect(NOTIFY, from(jason->getInstanceId()), WaitForCommand, sheila->ok())),
                Sub(sheila->expect(BYE/200, from(jason->getInstanceId()), WaitForCommand, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testDialogEventReplaces()
      {
         WarningLog(<< "testDialogEventReplaces");

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestClientRegistration regScott(scott);
         TestClientRegistration regJason(jason);
         TestClientRegistration regDerek(derek);

         //TestClientInviteSession invDerekToScott(derek);
         TestServerInviteSession invDerekFromScott(derek);

         TestClientInviteSession invDerekToJason(derek);

         TestClientInviteSession invClientScott(scott);
         //TestServerInviteSession invSrvScott(scott);
         TestClientInviteSession invClientScottToDerek(scott);

         TestServerInviteSession invSrvJasonFromDerek(jason);
         TestServerInviteSession invSrvJasonFromScott(jason);

         TestClientSubscription clientSubDerek(derek);
         TestServerSubscription serverSub(scott);

         TestDialogEvent testDialogEvt(jason);

         Uri derekContact = derek->getAor().uri();
         derekContact.host() = derek->getIp();
         derekContact.port() = derek->getPort();

         Uri scottContact = scott->getAor().uri();
         scottContact.host() = scott->getIp();
         scottContact.port() = scott->getPort();

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Recipient;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         //di1.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         // the following are properties that get generated AFTER the initial INVITE is made by DUM...
         di1.mDialogId = DialogId("*", "*", "*");
         di1.mLocalIdentity = jason->getAor();
         di1.mLocalTarget = Uri("sip:anyone@anywhere.com");
         di1.mRemoteIdentity = derek->getAor();
         di1.mRemoteTarget = std::auto_ptr<Uri>(new Uri(derekContact));
         DialogEventPred* pDi1 = new DialogEventPred(di1);

         TestDialogEventInfo di1a(di1);
         di1a.mState = DialogEventInfo::Early;
         DialogEventPred* pDi1a = new DialogEventPred(di1a);

         TestDialogEventInfo di2(di1);
         di2.mState = DialogEventInfo::Confirmed;
         DialogEventPred* pDi2 = new DialogEventPred(di2);

         TestDialogEventInfo di3;
         di3.mDialogEventId = "*";
         di3.mDirection = DialogEventInfo::Recipient;
         di3.mInviteSession = InviteSessionHandle::NotValid();
         di3.mDialogId = DialogId("*", "*", "*");
         di3.mLocalIdentity = jason->getAor();
         di3.mLocalTarget = Uri("sip:anyone@anywhere.com");
         di3.mRemoteTarget = std::auto_ptr<Uri>(new Uri(scottContact));
         di3.mRemoteIdentity = scott->getAor();
         di3.mReplacesId = std::auto_ptr<DialogId>(new DialogId("*", "*", "*"));
         di3.mReferredBy = std::auto_ptr<NameAddr>(new NameAddr(derek->getProfile()->getDefaultFrom()));
         DialogEventPred* pDi3 = new DialogEventPred(di3);

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Confirmed;
         DialogEventPred* pDi4 = new DialogEventPred(di4);

         TestDialogEventInfo di5(di2);
         di5.mState = DialogEventInfo::Terminated;
         DialogEventPred* pDi5 = new DialogEventPred(di5);

         TestDialogEventInfo di6(di4);
         di6.mState = DialogEventInfo::Terminated;
         DialogEventPred* pDi6 = new DialogEventPred(di6);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         // scott calls derek
         Seq(scott->invite(derek->getProfile()->getDefaultFrom(), standardOffer),
             derek->expect(Invite_NewServerSession, invDerekFromScott, dumFrom(scott), TimeOut, invDerekFromScott.noAction()),
             invDerekFromScott.expect(Invite_Offer, dumFrom(scott), TimeOut, 
                                chain(invDerekFromScott.provisional(180), invDerekFromScott.provideAnswer(*standardAnswer), invDerekFromScott.accept(200))),

             And(Sub(invDerekFromScott.expect(Invite_Connected, dumFrom(scott), TimeOut, invDerekFromScott.noAction())),
                 Sub(scott->expect(Invite_NewClientSession, invClientScottToDerek, dumFrom(derek), TimeOut, scott->noAction()),
                     invClientScottToDerek.expect(Invite_Provisional, dumFrom(derek), TimeOut, invClientScottToDerek.noAction()),
                     invClientScottToDerek.expect(Invite_Answer, dumFrom(derek), TimeOut, invClientScottToDerek.noAction()),
                     invClientScottToDerek.expect(Invite_Connected, dumFrom(derek), TimeOut, invClientScottToDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek calls jason
         Seq(derek->invite(jason->getProfile()->getDefaultFrom(), standardOffer),
             testDialogEvt.expect(DialogEvent_Trying, *pDi1, WaitForCommand, testDialogEvt.noAction()),
             jason->expect(Invite_NewServerSession, invSrvJasonFromDerek, dumFrom(derek), TimeOut, invSrvJasonFromDerek.noAction()),
             invSrvJasonFromDerek.expect(Invite_Offer, dumFrom(derek), TimeOut, 
                                         chain(invSrvJasonFromDerek.provisional(180), invSrvJasonFromDerek.provideAnswer(*standardAnswer), 
                                               invSrvJasonFromDerek.accept(200))),

             And(Sub(testDialogEvt.expect(DialogEvent_Early, *pDi1a, WaitForCommand, testDialogEvt.noAction()),
                     testDialogEvt.expect(DialogEvent_Confirmed, *pDi2, WaitForCommand, testDialogEvt.noAction()),
                     invSrvJasonFromDerek.expect(Invite_Connected, dumFrom(derek), TimeOut, invSrvJasonFromDerek.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerekToJason, dumFrom(jason), TimeOut, derek->noAction()),
                     invDerekToJason.expect(Invite_Provisional, dumFrom(jason), TimeOut, invDerekToJason.noAction()),
                     invDerekToJason.expect(Invite_Answer, dumFrom(jason), TimeOut, invDerekToJason.noAction()),
                     invDerekToJason.expect(Invite_Connected, dumFrom(jason), TimeOut, invDerekToJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek refers scott to jason
         Seq(invDerekFromScott.refer(jason->getProfile()->getDefaultFrom(), invDerekToJason.getSessionHandle()),
             invClientScottToDerek.expect(Invite_Refer, dumFrom(derek), TimeOut, 
                                chain(invClientScottToDerek.acceptRefer(), 
                                      scott->inviteFromRefer(invClientScottToDerek.getReferMessage(), 
                                                             invClientScottToDerek.getServerSubscription(), standardOffer))),

             And(Sub(And(Sub(testDialogEvt.expect(DialogEvent_Trying, *pDi3, WaitForCommand, testDialogEvt.noAction()),
                             jason->expect(Invite_NewServerSession, invSrvJasonFromScott, findMatchingDialogToReplace(jason), 
                                           TimeOut, invSrvJasonFromScott.noAction()),
                             invSrvJasonFromScott.expect(Invite_Offer, dumFrom(scott), TimeOut, 
                                                         chain(invSrvJasonFromScott.provideAnswer(*standardAnswer), invSrvJasonFromScott.accept(200))),
                             testDialogEvt.expect(DialogEvent_Confirmed, *pDi4, WaitForCommand, testDialogEvt.noAction()),
                             invSrvJasonFromScott.expect(Invite_Connected, dumFrom(scott), TimeOut, invSrvJasonFromDerek.end())),
                         Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi5, InviteSessionHandler::Replaced, WaitForCommand, testDialogEvt.noAction()),
                             invSrvJasonFromDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, 
                                                         invSrvJasonFromDerek.noAction())))),

                 Sub(scott->expect(ServerSubscription_Terminated, serverSub, *TestEndPoint::AlwaysTruePred, TimeOut*10, serverSub.noAction()),
                     scott->expect(Invite_NewClientSession, invClientScott, dumFrom(jason), TimeOut, scott->noAction()),
                     invClientScott.expect(Invite_Answer, dumFrom(jason), TimeOut, invClientScott.noAction()),
                     invClientScott.expect(Invite_Connected, dumFrom(jason), TimeOut, invClientScott.noAction())),

                 Sub(And(Sub(/*derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), TimeOut, 
                               clientSubDerek.acceptUpdate(200)),*/
                            invDerekFromScott.expect(Invite_ReferAccepted, dumFrom(scott), TimeOut, invDerekFromScott.noAction()),
                            derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), TimeOut, 
                                          clientSubDerek.acceptUpdate(200)),
                            clientSubDerek.expect(ClientSubscription_Terminated, dumFrom(scott), TimeOut*10, clientSubDerek.noAction())),
                         Sub(invDerekToJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invDerekToJason.noAction()))))),
             
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(invDerekFromScott.end(),
             And(Sub(invDerekFromScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invDerekFromScott.noAction())),
                 Sub(invClientScottToDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invClientScottToDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invSrvJasonFromScott.end(),
             And(Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi6, InviteSessionHandler::LocalBye, WaitForCommand, testDialogEvt.noAction()),
                     invSrvJasonFromScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invSrvJasonFromScott.noAction())),
                 Sub(invClientScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invClientScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      class ExpectedDialogEventInfoBuilderUas : public ExpectAction 
      {
         public:
            ExpectedDialogEventInfoBuilderUas(TestDialogEventInfo* toUpdate,
                                              const boost::shared_ptr<resip::SipMessage>& updatedMsg) :
               mToUpdate(toUpdate),
               mUpdatedMsg(updatedMsg)
            {
            }
            virtual void operator()(boost::shared_ptr<Event> event)
            {
               resip_assert(0);
            }
            virtual void operator()()
            {
               mToUpdate->mDialogId = DialogId(mUpdatedMsg->header(h_CallID).value(),
                                               "*",
                                               mUpdatedMsg->header(h_From).param(p_tag));

               mToUpdate->mLocalIdentity = mUpdatedMsg->header(h_To);
               if (!mToUpdate->hasRemoteTarget())
               {
                  mToUpdate->mRemoteTarget = std::auto_ptr<Uri>(new Uri(mUpdatedMsg->header(h_Contacts).front().uri()));
               }
               mToUpdate->mRemoteIdentity = mUpdatedMsg->header(h_From);
            }
            virtual resip::Data toString() const { return "ExpectedDialogEventInfoBuilderUas";}
      private:
         TestDialogEventInfo* mToUpdate;
         const boost::shared_ptr<resip::SipMessage>& mUpdatedMsg;
      };

      void testDialogEventUasBasic()
      {
         InfoLog(<< "testDialogEventUasBasic");

         // Disable digest authentication for this test by treating messages from loopback adaptor as trusted
         proxy->addTrustedHost("127.0.0.1", UDP);

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestClientRegistration regJason(jason);
         
         Seq(jason->registerUa(), 
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestSipEndPoint* sheila = sipEndPoint;
         TestServerInviteSession uac(jason);
         TestDialogEvent testDialogEvt(jason);
         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         di1.mLocalTarget = Uri("sip:anyone@anywhere.com");

         TestDialogEventInfo di2(di1);
         di2.mState = DialogEventInfo::Early;

         TestDialogEventInfo di3(di2);
         di3.mState = DialogEventInfo::Confirmed;
         di3.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> sheilasInvite;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
//         DialogEventPred* pDi2 = new DialogEventPred(di2);
         DialogEventPred* pDi3 = new DialogEventPred(di3);
         DialogEventPred* pDi4 = new DialogEventPred(di4);

         Seq(chain(save(sheilasInvite, sheila->invite(jason->getAor().uri(), offer)),
                   new ExpectedDialogEventInfoBuilderUas(&di1, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di2, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di3, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di4, sheilasInvite)),
             And(Sub(optional(sheila->expect(INVITE/100, from(proxy), WaitFor100, sheila->noAction()))),
                 Sub(testDialogEvt.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_NewServerSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uac.provideAnswer(*answer),
                                                                                                   uac.accept(200))),
                     And(Sub(testDialogEvt.expect(DialogEvent_Confirmed, *pDi3, WaitForCommand, uac.noAction()),
                             uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                         Sub(sheila->expect(INVITE/200, from(proxy), WaitForResponse, chain(sheila->ack(),
                                                                                            uac.end())))))),
             And(Sub(uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(BYE, contact(jason->getContact()), CallTimeout, sheila->ok())),
                 Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::LocalBye, WaitForCommand, uac.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // Re-enable digest auth for localhost
         proxy->deleteTrustedHost("127.0.0.1", UDP);
      }

      void testDialogEvent302Uas()
      {
         InfoLog(<< "testDialogEvent302Uas");

         // Disable digest authentication for this test by treating messages from loopback adaptor as trusted
         proxy->addTrustedHost("127.0.0.1", UDP);

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestClientRegistration regJason(jason);
         
         Seq(jason->registerUa(), 
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestSipEndPoint* sheila = sipEndPoint;
         TestServerInviteSession uac(jason);
         TestDialogEvent testDialogEvt(jason);
         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Recipient;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         di1.mLocalTarget = Uri("sip:anyone@anywhere.com");

         Uri contactDerek = derek->getAor().uri();
         contactDerek.host() = derek->getIp();
         contactDerek.port() = derek->getPort();

         TestDialogEventInfo di2(di1);
         di2.mState = DialogEventInfo::Terminated;
         di2.mRemoteTarget = std::auto_ptr<Uri>(new Uri(contactDerek));

         //di3.mLocalOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardAnswer->clone()));

         resip::NameAddrs redirContacts;
         redirContacts.push_back(NameAddr(contactDerek));

         boost::shared_ptr<SipMessage> sheilasInvite;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);

         Seq(chain(save(sheilasInvite, sheila->invite(jason->getAor().uri(), offer)),
                   new ExpectedDialogEventInfoBuilderUas(&di1, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di2, sheilasInvite)),
             And(Sub(optional(sheila->expect(INVITE/100, from(proxy), WaitFor100, sheila->noAction()))),
                 Sub(testDialogEvt.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_NewServerSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.redirect(redirContacts)),
                     And(Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi2, InviteSessionHandler::Rejected, WaitForCommand, uac.noAction()),
                             uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                         Sub(sheila->expect(INVITE/302, from(proxy), WaitForResponse, sheila->ack()))))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // Re-enable digest auth for localhost
         proxy->deleteTrustedHost("127.0.0.1", UDP);
      }

      void testDialogEventCancelUas()
      {
         InfoLog(<< "testDialogEventCancelUas");

         // Disable digest authentication for this test by treating messages from loopback adaptor as trusted
         proxy->addTrustedHost("127.0.0.1", UDP);

         jason->getDum().addServerSubscriptionHandler("dialog", jason);
         DialogEventStateManager* desm(jason->getDum().createDialogEventStateManager(jason));
         (void)desm;

         TestClientRegistration regJason(jason);
         
         Seq(jason->registerUa(), 
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestSipEndPoint* sheila = sipEndPoint;
         TestServerInviteSession uac(jason);
         TestDialogEvent testDialogEvt(jason);
         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         TestDialogEventInfo di1;
         di1.mDialogEventId = "*";
         di1.mDirection = DialogEventInfo::Initiator;
         di1.mInviteSession = InviteSessionHandle::NotValid();
         di1.mRemoteOfferAnswer = std::auto_ptr<SdpContents>(static_cast<SdpContents*>(standardOffer->clone()));
         di1.mLocalTarget = Uri("sip:anyone@anywhere.com");

         TestDialogEventInfo di2(di1);
         di2.mState = DialogEventInfo::Early;

         TestDialogEventInfo di3(di2);
         di3.mState = DialogEventInfo::Confirmed;

         TestDialogEventInfo di4(di3);
         di4.mState = DialogEventInfo::Terminated;

         boost::shared_ptr<SipMessage> sheilasInvite;

         // !jjg! leaks
         DialogEventPred* pDi1 = new DialogEventPred(di1);
         DialogEventPred* pDi2 = new DialogEventPred(di2);
//         DialogEventPred* pDi3 = new DialogEventPred(di3);
         DialogEventPred* pDi4 = new DialogEventPred(di4);

         Seq(chain(save(sheilasInvite, sheila->invite(jason->getAor().uri(), offer)),
                   new ExpectedDialogEventInfoBuilderUas(&di1, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di2, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di3, sheilasInvite),
                   new ExpectedDialogEventInfoBuilderUas(&di4, sheilasInvite)),
             And(Sub(optional(sheila->expect(INVITE/100, from(proxy), WaitFor100, sheila->noAction()))),
                 Sub(testDialogEvt.expect(DialogEvent_Trying, *pDi1, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_NewServerSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provisional()),
                     And(Sub(sheila->expect(INVITE/180, from(proxy), WaitFor180, sheila->cancel())),
                         Sub(testDialogEvt.expect(DialogEvent_Early, *pDi2, WaitForCommand, uac.noAction()))),
                     And(Sub(testDialogEvt.expect(DialogEvent_Terminated, *pDi4, InviteSessionHandler::RemoteCancel, WaitForCommand, testDialogEvt.noAction()),
                             uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                         Sub(sheila->expect(CANCEL/200, from(proxy), WaitForResponse, sheila->noAction()),
                             sheila->expect(INVITE/487, from(proxy), WaitForResponse, sheila->ack()))))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // Re-enable digest auth for localhost
         proxy->deleteTrustedHost("127.0.0.1", UDP);
      }

      void testReinviteNo200()
      {
         WarningLog(<< "testReinviteNo200");

         TestClientRegistration clientReg(derek);
         TestServerInviteSession uas(derek);

         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         Uri contactDerek = derek->getAor().uri();
         contactDerek.host() = derek->getIp();
         contactDerek.port() = derek->getPort();

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, derek->registerUa()),
             derek->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, david->invite(derek->getAor().uri())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             derek->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, derek->noAction()),
             uas.expect(Invite_OfferRequired, dumFrom(proxy), WaitForCommand, chain(uas.provideOffer(*standardOffer), uas.accept(200))),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->ack(answer)),
             uas.expect(Invite_Answer, dumFrom(proxy), WaitForCommand, david->reInvite(contactDerek)),
             uas.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()), 
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             uas.expect(Invite_OfferRequired, dumFrom(proxy), 2000, uas.provideOffer(*standardOffer)),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             uas.expect(Invite_OfferRejected, *TestEndPoint::AlwaysTruePred, 5000, uas.end()),
             And(Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction())),
                 Sub(david->expect(BYE, from(proxy), CallTimeout, david->ok()))),
             CallTimeout);
         ExecuteSequences();
      }

      void test3pccFlow1NoAnswer()
      {
         WarningLog(<< "test3pccFlow1");

         TestClientRegistration clientReg(derek);
         TestServerInviteSession uas(derek);

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, derek->registerUa()), 
             derek->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, david->invite(derek->getAor().uri())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             derek->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, derek->noAction()),
             uas.expect(Invite_OfferRequired, dumFrom(proxy), WaitForCommand, chain(uas.provideOffer(*standardOffer), uas.accept(200))),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->noAction()),
             And(Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction())),
                 Sub(david->expect(BYE, contact(derek->getContact()), CallTimeout, david->ok()))),
             CallTimeout);
         ExecuteSequences();
      }

      void test3pccFlow1Reject()
      {
         WarningLog(<< "test3pccReject");

         TestClientRegistration clientReg(derek);

         TestServerInviteSession uas(derek);

          Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, derek->registerUa()), 
             derek->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, david->invite(derek->getAor().uri())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             derek->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, derek->noAction()),
             uas.expect(Invite_OfferRequired, dumFrom(proxy), WaitForCommand, chain(uas.provideOffer(*standardOffer), uas.accept(200))),
             david->expect(INVITE/200, from(proxy), CallTimeout, david->ack()),
             uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             david->expect(BYE, contact(derek->getContact()), CallTimeout, david->ok()),
             CallTimeout);
         ExecuteSequences();
      }
 
      void testKPML()
      {
         WarningLog(<< "testKPML");

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);
         boost::shared_ptr<SdpContents> ans(static_cast<SdpContents*>(standardAnswer->clone()));
         
         //!dcm! TODO need to rationalize aor/nameaddr...use SipElement. Possibly
         //SipElement will have conversion operators
         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             david->expect(INVITE, from(proxy), 5000, chain(david->ring(), david->answer(ans))),
             //!dcm! -- need to unify from/dumFrom...
             derek->expect(Invite_NewClientSession, invDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
             invDerek.expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             invDerek.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             invDerek.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             david->expect(ACK, contact(derek->getContact()), WaitForResponse, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerSubscription serv(derek);         
         Seq(david->subscribe(derek->getAor().uri(), Token("test")),
             optional(david->expect(SUBSCRIBE/407, from(proxy), WaitForResponse, david->digestRespond())),
             derek->expect(ServerSubscription_NewSubscription, serv, *TestEndPoint::AlwaysTruePred, WaitForCommand, 
                           chain(serv.accept(), serv.neutralNotify())),
             david->expect(SUBSCRIBE/200, contact(derek->getContact()), WaitForResponse, david->noAction()),
             david->expect(NOTIFY, contact(derek->getContact()), WaitForCommand, david->respond(200)),             
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(serv.end(),
             david->expect(NOTIFY, contact(derek->getContact()), WaitForCommand, david->respond(200)),  
             WaitForEndOfTest);
         ExecuteSequences();
         
      }

#if 0
      void testLargeFieldAttack()
      {

         Data txt("INVITE sip:derek@localhost SIP/2.0\r\n"
                  "Via: SIP/2.0/UDP 127.0.0.1:5060;branch=z9hG4bK00001249z9hG4bK.00004119\r\n"
                  "From: 1249 <sip:bob@127.0.0.1>;tag=1249"
                  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
                  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa: Receiver <sip:100012 (at) 172.16.1 (dot) 1 [email concealed]>\r\n"
                  "Call-ID: 4166@<172.16.3.6>\r\n"
                  "CSeq: 18571 INVITE\r\n"
                  "Expires: 1200\r\n"
                  "Max-Forwards: 70\r\n"
                  "Content-Type: application/sdp\r\n"
                  "Content-Length: 130\r\n"
                  "\r\n"
                  "v=0\r\n"
                  "o=1249 1249 1249 IN IP4 127.0.0.1\r\n"
                  "s=Session SDP\r\n"
                  "c=IN IP4 127.0.0.1\r\n"
                  "t=0 0\r\n"
                  "m=audio 9876 RTP/AVP 0\r\n"
                  "a=rtpmap:0 PCMU/8000\r\n");

         Seq(derek->registerUa(),  
             derek->expect(Register_Success, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         DebugLog(<<"now large field attack");
         

         Seq(sipEndPoint->rawSend(derek->getAor(), txt), 
             sipEndPoint->expect(INVITE/400, from(proxy), WaitForResponse, sipEndPoint->noAction()),
             WaitForEndOfTest);

         ExecuteSequences();
      }
#endif

      void testRegisterSips()
      {
         WarningLog(<< "testRegisterSips");
         
         // disable the rinstance.
         derek->getProfile()->setRinstanceEnabled(false);
         derek->getProfile()->setOutboundProxy(Uri("sips:repro.internal.xten.net"));
         
         TestClientRegistration clientReg(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, noRinstance(), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

//          Seq(clientReg.end(),
//              clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred,
//                               WaitForRegistration, clientReg.noAction()),
//              WaitForEndOfSeq);
//          ExecuteSequences();

         derek->getProfile()->setRinstanceEnabled(true);
      };


      void testRegisterBasicWithoutRinstance()
      {
         WarningLog(<< "testRegisterBasicWithoutRinstance");
         
         // disable the rinstance.
         derek->getProfile()->setRinstanceEnabled(false);
         TestClientRegistration clientReg(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, noRinstance(), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred,
                              WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         derek->getProfile()->setRinstanceEnabled(true);
      };

      void testRegisterBasicWithRinstance()
      {
         WarningLog(<< "testRegisterBasicWithRinstance");
         
         TestClientRegistration clientReg(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, hasRinstance(), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         /*
         Seq(chain(clientReg.removeMyBindings(), clientReg.addBinding(derek->getAor())),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             derek->expect(Register_Success, clientReg, hasRinstance(), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         */

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

      };

      void testRegisterBasicWithMethodsParam()
      {
         InfoLog(<< "testRegisterBasicWithMethodsParam");
         
         // enable the methodsParam.
         derek->getProfile()->setMethodsParamEnabled(true);
         TestClientRegistration clientReg(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, hasMethodsParam(), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         derek->getProfile()->setMethodsParamEnabled(false);
      };

      void testRegisterBasicWithoutMethodsParam()
      {
         InfoLog(<< "testRegisterBasicWithoutMethodsParam");
         
         TestClientRegistration clientReg(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, noMethodsParam(), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred,
                              WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      };

#if 0
      
      /*
      void testRegisterShouldFail()
      {
         //Uri aor;
         //aor = proxy->getUri();
         //aor.user() = "derek";         
         //resip::SharedPtr<MasterProfile> prof = DumUserAgent::makeProfile(aor, "derek");
         //DumUserAgent derek(prof, proxy);
         
         Seq( derek->registerUa(),
             derek->expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()), //derek->shutdownUa()),
             WaitForEndOfTest);
         ExecuteSequences();      
      };
      */
#endif

      void testSubscribeRequestRefreshBetweenDumUaAndTestEndPoint()
      {
         WarningLog(<< "testSubscribeBetweenDumUaAndTestEndPoint");
         Uri contact = derek->getAor().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();
      
         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data txt("Messages-Waiting: yes\r\n"
                  "Message-Account: sip:derek@mail.sipfoundry.org\r\n"
                  "Voice-Message: 2/8 (0/2)\r\n");
         HeaderFieldValue hfv(txt.begin(), txt.size());
         boost::shared_ptr<Contents> mwc = boost::shared_ptr<Contents>(new MessageWaitingContents(hfv, Mime("application", "simple-message-summary")));

         TestClientSubscription clientSub(derek);

         Seq(derek->subscribe(NameAddr(david->getAddressOfRecord()), Data("message-summary")),
             david->expect(SUBSCRIBE, from(contact), WaitForCommand, chain(david->send202ToSubscribe(), 
                                                                           david->notify(mwc, "message-summary", "active",
                                                                                         derek->getProfile()->getDefaultSubscriptionTime(), 
                                                                                         derek->getProfile()->getDefaultSubscriptionTime(), true),
                                                                           david->notify(mwc, "message-summary", "active", 
                                                                                         derek->getProfile()->getDefaultSubscriptionTime()-50, 
                                                                                         derek->getProfile()->getDefaultSubscriptionTime()))),
             And(Sub(optional(david->expect(NOTIFY/407, from(proxy), WaitForResponse, david->digestRespond())),
                     optional(david->expect(NOTIFY/407, from(proxy), WaitForResponse, david->digestRespond()))),
                 Sub(derek->expect(ClientSubscription_NewSubscription, clientSub, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
                     clientSub.expect(ClientSubscription_UpdateActive, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientSub.acceptUpdate()),
                     clientSub.expect(ClientSubscription_UpdateActive, *TestEndPoint::AlwaysTruePred, WaitForCommand, 
                                      clientSub.acceptUpdate())),
                                  
                 Sub(david->expect(NOTIFY/200, from(contact), WaitForResponse, david->noAction()),
                     david->expect(NOTIFY/200, from(contact), WaitForResponse, david->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientSub.requestRefresh(),
             And(Sub(clientSub.expect(ClientSubscription_UpdateActive, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientSub.acceptUpdate())),
                 Sub(david->expect(SUBSCRIBE, from(contact), WaitForCommand,
                                   chain(david->send202ToSubscribe(),
                                         david->notify(mwc, "message-summary", "active",
                                                       derek->getProfile()->getDefaultSubscriptionTime(),
                                                       derek->getProfile()->getDefaultSubscriptionTime()))),
                     optional(david->expect(NOTIFY/407, from(contact), WaitForCommand, david->digestRespond())),
                     david->expect(NOTIFY/200, from(contact), WaitForResponse, david->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }
            
      void testSubscribe406()
      {
         WarningLog(<< "testSubscribe406");

         TestClientRegistration regJason(jason);

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerSubscription srvSub(jason);

         Seq(derek->subscribe(jason->getAor(), Data("presence")),
             jason->expect(ServerSubscription_NewSubscription, srvSub, dumFrom(*derek), WaitForCommand, srvSub.reject(406)),

             And(Sub(srvSub.expect(ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvSub.noAction())),
                 Sub(derek->expect(ClientSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testSubscribe()
      {
         WarningLog(<< "testSubscribe");

         TestClientRegistration clientRegDerek(derek);
         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientRegDerek, *TestEndPoint::AlwaysTruePred, 
                           WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data txt("Messages-Waiting: yes\r\n"
                  "Message-Account: sip:derek@mail.sipfoundry.org\r\n"
                  "Voice-Message: 2/8 (0/2)\r\n");
         HeaderFieldValue hfv(txt.begin(), txt.size());
         boost::shared_ptr<resip::Contents> mwc = 
            boost::shared_ptr<resip::Contents>(new MessageWaitingContents(hfv, Mime("application", "simple-message-summary")));

         TestClientSubscription clientSub(jason);
         TestServerSubscription serverSub(derek);


         Seq(jason->subscribe(NameAddr(derek->getAor()), Data("message-summary")),
             derek->expect(ServerSubscription_NewSubscription, serverSub, dumFrom(*jason), WaitForCommand, 
                           chain(serverSub.setSubscriptionState(Active), serverSub.accept(), serverSub.update(mwc.get()))),
             jason->expect(ClientSubscription_NewSubscription, clientSub, dumFrom(*derek), WaitForCommand, jason->noAction()),
             clientSub.expect(ClientSubscription_UpdateActive, dumFrom(*derek), WaitForCommand, clientSub.acceptUpdate(200)),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientSub.end(),
             serverSub.expect(ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, serverSub.noAction()),
             clientSub.expect(ClientSubscription_Terminated, dumFrom(derek), WaitForCommand, clientSub.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientRegDerek.end(),
             clientRegDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientRegDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }


      void testInviteWithPrivacy()
      {
         WarningLog(<< "testInviteWithPrivacy");

         TestClientInviteSession uac(hiss);

         boost::shared_ptr<SipMessage> privInvite;
         boost::shared_ptr<SdpContents> ans(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(hiss->invite(sipEndPoint->getContact(), standardOffer),
             sipEndPoint->expect(INVITE, from(hiss->getInstanceId()), WaitForCommand, 
                                 chain(sipEndPoint->check1(CheckPrivacy(), "Check privacy in INV"), sipEndPoint->answer(ans))),
             And(Sub(hiss->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sipEndPoint->expect(ACK, from(hiss->getInstanceId()), WaitForResponse, sipEndPoint->noAction()))),
             WaitForEndOfSeq);
         
         ExecuteSequences();
         
         Seq(uac.info(*standardOffer),
             sipEndPoint->expect(INFO, from(hiss->getInstanceId()), WaitForCommand,
                                 chain(sipEndPoint->check1(CheckPrivacy(), "Check privacy in info"), sipEndPoint->ok())),
             uac.expect(Invite_InfoSuccess, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             WaitForEndOfSeq);
         
         ExecuteSequences();
         
         Seq(uac.end(), 
             And(Sub(sipEndPoint->expect(BYE, from(hiss->getInstanceId()), WaitForCommand, 
                                         chain(sipEndPoint->check1(CheckPrivacy(), "Check Privacy in BYE"), sipEndPoint->ok()))),
                 Sub(uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()))),
             WaitForEndOfTest);
         
         ExecuteSequences();
      }


      void testInviteBasicSuccess()
      {
         WarningLog(<< "testInviteBasicSuccess");

         TestClientRegistration regJason(jason);
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uas, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, chain(uas.provisional(180), 
                                                                             uas.provideAnswer(*standardAnswer), 
                                                                             uas.accept(200))),
             And(Sub(uas.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uas.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, uac, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     uac.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Answer, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Connected, dumFrom(*jason), WaitForCommand, uac.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uac.end(InviteSession::UserHangup), 
             uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.removeMyBindings(true),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteLateMediaSuccess()
      {
         WarningLog(<< "testInviteLateMediaSuccess");

         TestClientRegistration regJason(jason);
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), 0),
             jason->expect(Invite_NewServerSession, uas, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_OfferRequired, *TestEndPoint::AlwaysTruePred, WaitForCommand, 
                        chain(uas.provisional(180), uas.provideOffer(*standardOffer), uas.accept(200))),

             And(Sub(derek->expect(Invite_NewClientSession, uac, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     uac.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Offer, dumFrom(*jason), WaitForCommand, uac.provideAnswer(*standardAnswer)),
                     uac.expect(Invite_Connected, dumFrom(*jason), WaitForCommand, uac.noAction())),
                 Sub(uas.expect(Invite_Answer, dumFrom(*derek), WaitForCommand, uas.noAction()),
                     uas.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uas.noAction()))),

             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uac.end(),
             uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.removeMyBindings(true),
             regJason.expect(Register_Removed, dumFrom(proxy), WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteLateMediaFailure()
      {
         WarningLog(<< "testInviteLateMediaFailure");
         Uri contact = derek->getProfile()->getDefaultFrom().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();

         TestClientRegistration regDerek(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uacDerek(derek);
         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), 0),
             david->expect(INVITE, from(contact), WaitForCommand, chain(david->ring(), david->answer())), // no offer

             And(Sub(derek->expect(Invite_NewClientSession, uacDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
                     uacDerek.expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Failure, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction())),
                 Sub(david->expect(ACK, from(contact), WaitForCommand, david->noAction()),
                     david->expect(BYE, from(contact), WaitForCommand, david->ok()))),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
         
         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteBusy()
      {
         WarningLog(<< "testInviteBusy");

         TestClientRegistration regDerek(derek);
         TestClientRegistration regJason(jason);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, dumFrom(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), 0),
             jason->expect(Invite_NewServerSession, uas, dumFrom(derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_OfferRequired, dumFrom(derek), WaitForCommand, uas.reject(486)),

             And(Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction())),
                 Sub(derek->expect(Invite_Failure, dumFrom(jason), WaitForCommand, derek->noAction()),
                     derek->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInvite302()
      {
         WarningLog(<< "testInvite302");

         TestClientRegistration regJason(jason);
         TestClientRegistration regScott(scott);
         
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         NameAddrs contacts;
         Uri contact = scott->getAor().uri();
         contact.host() = scott->getIp();
         contact.port() = scott->getPort();
         contacts.push_back(NameAddr(contact));

         TestClientInviteSession uacDerekToJason(derek);
         TestClientInviteSession uacDerekToScott(derek);
         TestServerInviteSession uasJason(jason);
         TestServerInviteSession uasScott(scott);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uasJason, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uasJason.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, chain(uasJason.provisional(180), uasJason.redirect(contacts, 302))),

             And(Sub(uasJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uasJason.noAction())),
                 Sub(
                    And(Sub(derek->expect(Invite_NewClientSession, dumFrom(*jason), WaitForCommand, derek->noAction()),
                            derek->expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, derek->noAction()),
                            derek->expect(Invite_Redirected, dumFrom(*scott), WaitForCommand, derek->noAction()),
                            derek->expect(Invite_NewClientSession, uacDerekToScott, dumFrom(*scott), WaitForCommand, derek->noAction()),
                            uacDerekToScott.expect(Invite_Provisional, dumFrom(*scott), WaitForCommand,uacDerekToScott.noAction()),
                            uacDerekToScott.expect(Invite_Answer, dumFrom(*scott), WaitForCommand, uacDerekToScott.noAction()),
                            uacDerekToScott.expect(Invite_Connected, dumFrom(*scott), WaitForCommand, uacDerekToScott.end()),
                            uacDerekToScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerekToScott.noAction())),
                        Sub(scott->expect(Invite_NewServerSession, uasScott, dumFrom(*derek), WaitForCommand, scott->noAction()),
                            uasScott.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, 
                                            chain(uasScott.provisional(180), uasScott.provideAnswer(*standardAnswer), uasScott.accept(200))),
                            uasScott.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uasScott.noAction()),
                            uasScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uasScott.noAction()))))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(regJason.removeMyBindings(true),
             regJason.expect(Register_Removed, dumFrom(proxy), WaitForRegistration, regJason.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regScott.removeMyBindings(true),
             regScott.expect(Register_Removed, dumFrom(proxy), WaitForRegistration, regScott.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

      }

      void testReInviteIpChange()
      {
         WarningLog(<< "testReInviteIpChange");

         TestClientRegistration regJason(jason);
         
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data ipChange("v=0\r\n"
                       "o=dumTfm 44527 44528 IN IP4 localhost\r\n"
                       "s=SIP Call\r\n"
                       "c=IN IP4 127.0.0.5\r\n"
                       "t=0 0\r\n"
                       "m=audio 47172 RTP/AVP 0\r\n"
                       "a=rtpmap:0 PCMU/8000\r\n");
       
         Mime type("application", "sdp");
         HeaderFieldValue hfv(ipChange.data(), ipChange.size());
         SdpContents sdpIpChange(hfv, type);

         TestClientInviteSession uacDerek(derek);
         TestServerInviteSession uasJason(jason);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uasJason, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uasJason.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, 
                             chain(uasJason.provisional(180), uasJason.provideAnswer(*standardAnswer), uasJason.accept(200))),

             And(Sub(uasJason.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uasJason.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, uacDerek, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     uacDerek.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Answer, dumFrom(*jason), WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Connected, dumFrom(*jason), WaitForCommand, uacDerek.noAction()))),

             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uasJason.provideOffer(sdpIpChange),
             uacDerek.expect(Invite_Offer, dumFrom(*jason), 2000, uacDerek.provideAnswer(*standardAnswer)),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uacDerek.end(),
             uacDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()),
             uasJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uasJason.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteUnsuccessfulTempUnavailable()
      {
         WarningLog(<< "testInviteUnsuccessfulTempUnavailable");

         TestClientRegistration regJason(jason);
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), 0),
             jason->expect(Invite_NewServerSession, uas, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_OfferRequired, dumFrom(*derek), WaitForCommand, uas.reject(480)),

             And(Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction())),
                 Sub(derek->expect(Invite_Failure, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     derek->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, dumFrom(proxy), WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInviteUnsuccessfulNoAnswer()
      {
         WarningLog(<< "testInviteUnsuccessfulNoAnswer");

         TestClientRegistration regJason(jason);
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uas, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, uas.provisional(180)),
             derek->expect(Invite_NewClientSession, uac, dumFrom(*jason), WaitForCommand, derek->noAction()),
             uac.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uac.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(derek->cancelInvite(),
             And(Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction())),
                 Sub(uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testSubscribeRequestRefresh()
      {
         WarningLog(<< "testSubscribeRequestRefresh");

         TestClientRegistration regDerek(derek);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data txt("Messages-Waiting: yes\r\n"
                  "Message-Account: sip:derek@mail.sipfoundry.org\r\n"
                  "Voice-Message: 2/8 (0/2)\r\n");
         HeaderFieldValue hfv(txt.begin(), txt.size());
         boost::shared_ptr<Contents> mwc = 
            boost::shared_ptr<Contents>(new MessageWaitingContents(hfv, Mime("application", "simple-message-summary")));

         TestClientSubscription clientSub(jason);
         TestServerSubscription serverSub(derek);

         Seq(jason->subscribe(NameAddr(derek->getAor()), Data("message-summary")),
             derek->expect(ServerSubscription_NewSubscription, serverSub, dumFrom(jason), WaitForCommand, 
                           chain(serverSub.setSubscriptionState(Active), serverSub.accept(), serverSub.update(mwc.get()))),
             jason->expect(ClientSubscription_NewSubscription, clientSub, dumFrom(derek), WaitForCommand, jason->noAction()),
             clientSub.expect(ClientSubscription_UpdateActive, dumFrom(derek), WaitForCommand, 
                              chain(clientSub.acceptUpdate(200), clientSub.requestRefresh())),
             clientSub.expect(ClientSubscription_UpdateActive, dumFrom(derek), WaitForCommand, clientSub.acceptUpdate()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientSub.end(),
             And(Sub(clientSub.expect(ClientSubscription_Terminated, dumFrom(derek), WaitForCommand, clientSub.noAction())),
                 Sub(serverSub.expect(ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, serverSub.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testSubscribeQueuedRequestRefresh()
      {
         WarningLog(<< "testSubscribeQueuedRequestRefresh");

         TestClientRegistration regJason(jason);
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data txt("Messages-Waiting: yes\r\n"
                  "Message-Account: sip:derek@mail.sipfoundry.org\r\n"
                  "Voice-Message: 2/8 (0/2)\r\n");
         HeaderFieldValue hfv(txt.begin(), txt.size());
         boost::shared_ptr<Contents> mwc = 
            boost::shared_ptr<Contents>(new MessageWaitingContents(hfv, Mime("application", "simple-message-summary")));

         TestServerSubscription srvSubJason(jason);
         TestClientSubscription clientSubDerek(derek);

         Seq(derek->subscribe(jason->getAor(), Data("message-summary")),
             jason->expect(ServerSubscription_NewSubscription, srvSubJason, dumFrom(*derek), WaitForCommand, 
                           chain(srvSubJason.setSubscriptionState(Active), srvSubJason.accept(), srvSubJason.update(mwc.get()))),

             derek->expect(ClientSubscription_NewSubscription, clientSubDerek, dumFrom(*jason), WaitForCommand, derek->noAction()),
             clientSubDerek.expect(ClientSubscription_UpdateActive, dumFrom(*jason), WaitForCommand, 
                                   chain(clientSubDerek.acceptUpdate(), clientSubDerek.requestRefresh(), clientSubDerek.requestRefresh())),
             clientSubDerek.expect(ClientSubscription_UpdateActive, dumFrom(*jason), WaitForCommand, clientSubDerek.acceptUpdate()),
             clientSubDerek.expect(ClientSubscription_UpdateActive, dumFrom(*jason), WaitForCommand, clientSubDerek.acceptUpdate()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientSubDerek.end(),
             And(Sub(clientSubDerek.expect(ClientSubscription_Terminated, dumFrom(jason), WaitForCommand, clientSubDerek.noAction())),
                 Sub(srvSubJason.expect(ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvSubJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testPublishBasic()
      {
         WarningLog(<< "testPublishBasic");
         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getAor().uri());

         Uri contact = derek->getAor().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();
         
         TestClientPublication clientPub(derek);
         
         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPub, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPub.end(),
             david->expect(PUBLISH, from(contact), WaitForResponse, david->send200ToPublish()),
             clientPub.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientPub.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

      }

      void testPublishRefresh()
      {
         WarningLog(<< "testPublishRefresh");

         TestClientPublication clientPubDerek(derek);
         Uri contact = derek->getAor().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();

         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getAor().uri()));
         pidf->getTuples().front().id = derek->getAor().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getAor().uri());
         
         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPubDerek.refresh(0),
             david->expect(PUBLISH, from(contact), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientPubDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPubDerek.end(),
             david->expect(PUBLISH, from(contact), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientPubDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

      }

      void testPublishUpdate()
      {
         WarningLog(<< "testPublishUpdate");

         TestClientPublication clientPubDerek(derek);
         Uri contact = derek->getAor().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();

         //!dcm! -- should be auto_ptr, move generation to utility function;
         //wheere content doesn't matter, should have static bodies in fixture
         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getProfile()->getDefaultFrom().uri());
         
         Pidf* pidf2 = new Pidf;
         boost::shared_ptr<resip::Contents> body2(pidf2);
         pidf2->setSimpleStatus(false, "offline", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf2->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf2->getTuples().front().attributes["displayName"] = "displayName";
         pidf2->setEntity(derek->getProfile()->getDefaultFrom().uri());


         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPubDerek.update(body2.get()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPubDerek.end(),
             david->expect(PUBLISH, from(contact), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientPubDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

      }

      class ReUseNonce
      {
         public:
            ReUseNonce(boost::shared_ptr<resip::SipMessage> source, bool stale = false) :
               mSource(source),
               mStale(stale)
            {
               resip_assert(mSource->header(h_StatusLine).responseCode() == 401 || mSource->header(h_StatusLine).responseCode() == 407);
               resip_assert(mSource->header(h_WWWAuthenticates).size() == 1);
            }
               
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg)
            {
               resip_assert(msg->header(h_StatusLine).responseCode() == 401 || msg->header(h_StatusLine).responseCode() == 407);
               resip_assert(msg->header(h_WWWAuthenticates).size() == 1);
               resip_assert(mSource->header(h_WWWAuthenticates).size() == 1);
               msg->header(h_WWWAuthenticates).front().param(p_nonce) = mSource->header(h_WWWAuthenticates).front().param(p_nonce);
               if (mStale)
               {
                  //.dcm. stale should prob. be a boolean-type param
                  msg->header(h_WWWAuthenticates).front().param(p_stale) = "\"true\"";                  
               }
               return msg;;               
            }
         private:
            boost::shared_ptr<resip::SipMessage> mSource;
            bool mStale;            
      };
      
      void testStaleNonceHandling()
      {
         WarningLog(<< "testStaleNonceHandling");

         TestClientPublication clientPubDerek(derek);

         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getProfile()->getDefaultFrom().uri());

         boost::shared_ptr<resip::SipMessage> nonceSource;
         
         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, save(nonceSource, david->send401())),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Pidf* pidf2 = new Pidf;
         boost::shared_ptr<resip::Contents> body2(pidf2);
         pidf2->setSimpleStatus(false, "offline", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf2->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf2->getTuples().front().attributes["displayName"] = "displayName";
         pidf2->setEntity(derek->getProfile()->getDefaultFrom().uri());
         
         WarningLog(<< "testStaleNonceHandling, second phase");
         Seq(clientPubDerek.update(body2.get()),
             david->expect(PUBLISH, matchNonceCount(2), WaitForResponse, 
                           condition(ReUseNonce(nonceSource, true), david->send401())),
             david->expect(PUBLISH, matchNonceCount(1), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Success, *TestEndPoint::AlwaysTruePred, 
                                   WaitForResponse*4, clientPubDerek.end()),
             david->expect(PUBLISH, matchNonceCount(2), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, 
                                   WaitForResponse, clientPubDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();         
      }

      void testLucentAuthIssue()
      {
         WarningLog(<< "testLucentAuthIssue");

         TestClientPublication clientPubDerek(derek);

         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getProfile()->getDefaultFrom().uri());
         
         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Pidf* pidf2 = new Pidf;
         boost::shared_ptr<resip::Contents> body2(pidf2);
         pidf2->setSimpleStatus(false, "offline", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf2->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf2->getTuples().front().attributes["displayName"] = "displayName";
         pidf2->setEntity(derek->getProfile()->getDefaultFrom().uri());
         
         boost::shared_ptr<resip::SipMessage> nonceSource;
         
         //re-use nonce
         Seq(clientPubDerek.update(body2.get()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, 
                           save(nonceSource, david->send401())),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         WarningLog(<< "testLucentAuthIssue, second phase");
         Seq(clientPubDerek.update(body.get()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, 
                           condition(ReUseNonce(nonceSource), david->send401())),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Success, *TestEndPoint::AlwaysTruePred, 

//!dcm! -- something around here is causing an access violation on failure,
//just comment out the rest to reproduce
                                   WaitForResponse*4, clientPubDerek.end()),
             david->expect(PUBLISH, alwaysMatches(), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, 
                                   WaitForResponse, clientPubDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();         
      }

      void testPublish423()
      {
         WarningLog(<< "testPublish423");

         TestClientPublication clientPubDerek(derek);
         Uri contact = derek->getAor().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();

         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getProfile()->getDefaultFrom().uri());
         
         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send423Or200ToPublish(200)),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send423Or200ToPublish(200)),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Pidf* pidf2 = new Pidf;
         boost::shared_ptr<resip::Contents> body2(pidf2);
         pidf2->setSimpleStatus(false, "offline", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf2->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf2->getTuples().front().attributes["displayName"] = "displayName";
         pidf2->setEntity(derek->getProfile()->getDefaultFrom().uri());
         Seq(clientPubDerek.update(body2.get()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send401()),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPubDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPubDerek.end(),
             david->expect(PUBLISH, from(contact), WaitForResponse, david->send200ToPublish()),
             clientPubDerek.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientPubDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
      }

      void testReinviteLateMedia()
      {
         WarningLog(<< "testReinviteLateMedia");

         TestClientRegistration regDerek(derek);
         TestClientRegistration regJason(jason);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, dumFrom(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success,regJason,  dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uas, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, 
                        chain(uas.provisional(180), uas.provideAnswer(*standardAnswer), uas.accept(200))),

             And(Sub(uas.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uas.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, uac, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     uac.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Answer, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Connected, dumFrom(*jason), WaitForCommand, uac.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uas.requestOffer(),
             And(Sub(uac.expect(Invite_OfferRequired, dumFrom(jason), WaitForCommand, uac.provideOffer(*standardOffer)),
                     uac.expect(Invite_Answer, dumFrom(jason), WaitForCommand, uac.noAction())),
                 Sub(uas.expect(Invite_Offer, dumFrom(derek), WaitForCommand, uas.provideAnswer(*standardAnswer)))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uac.end(),
             And(Sub(uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction())),
                 Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, jason->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testReinviteRejected()
      {
         WarningLog(<< "testReinviteRejected");

         TestClientRegistration regDerek(derek);
         TestClientRegistration regJason(jason);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, dumFrom(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(derek);
         TestServerInviteSession uas(jason);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uas, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uas.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, 
                        chain(uas.provisional(180), uas.provideAnswer(*standardAnswer), uas.accept(200))),

             And(Sub(uas.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uas.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, uac, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     uac.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Answer, dumFrom(*jason), WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Connected, dumFrom(*jason), WaitForCommand, uac.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uac.provideOffer(*standardOffer),
             uas.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, uas.reject(488)),
             uac.expect(Invite_OfferRejected, dumFrom(*jason), WaitForCommand, uac.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uac.end(),
             And(Sub(uac.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction())),
                 Sub(uas.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, jason->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

#if 0
      void testReinviteWithByeSentAfterMissedAck() // not quite work, how to
                                                   // handle retransmissions?
      {
         WarningLog(<< "testReinviteWithByeSentAfterMissedAck");

         TestClientRegistration regDerek(derek);
         Seq(derek->registerUa(),
             derek->expect(Register_Success, dumFrom(proxy), WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->registerUser(60, david->getDefaultContacts()),
             david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond()),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         TestClientInviteSession uacDerek(derek);
         Uri contactDerek = derek->getAor().uri();
         contactDerek.host() = derek->getIp();
         contactDerek.port() = derek->getPort();

         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             david->expect(INVITE, from(contactDerek), WaitForCommand, chain(david->ring(), david->answer())),

             And(Sub(derek->expect(Invite_NewClientSession, uacDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
                     uacDerek.expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction())),
                 Sub(david->expect(ACK, from(contactDerek), WaitForAck, david->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->reInvite(contactDerek),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             And(Sub(optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
                     david->expect(INVITE/200, from(contactDerek), WaitForCommand, david->noAction()), // retransmitted 200s
                     david->expect(BYE, from(contactDerek), WaitForPause, david->ok())),
                 Sub(uacDerek.expect(Invite_OfferRequired, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.provideOffer(*standardOffer)),
                     uacDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }
#endif

      void testBlindTransferRejected()
      {
         WarningLog(<< "testBlindTransferRejected");

         Uri aorBob;
         aorBob = proxy->getUri();
         aorBob.user() = "bob";
         resip::SharedPtr<MasterProfile> profBob = DumUserAgent::makeProfile(aorBob, "bob");
         DumUserAgent bob(profBob, proxy);
         bob.init();
//         bob.run();

         Uri aorSean;
         aorSean = proxy->getUri();
         aorSean.user() = "sean";
         resip::SharedPtr<MasterProfile> profSean = DumUserAgent::makeProfile(aorSean, "sean");
         DumUserAgent sean(profSean, proxy);
         sean.init();
//         sean.run();

         Uri aorAlan;
         aorAlan = proxy->getUri();
         aorAlan.user() = "alan";
         resip::SharedPtr<MasterProfile> profAlan = DumUserAgent::makeProfile(aorAlan, "alan");
         DumUserAgent alan(profAlan, proxy);
         alan.init();
//         alan.run();

         Uri bobContact = bob.getProfile()->getDefaultFrom().uri();
         bobContact.host() = bob.getIp();
         bobContact.port() = bob.getPort();

         Uri seanContact = sean.getProfile()->getDefaultFrom().uri();
         seanContact.host() = sean.getIp();
         seanContact.port() = sean.getPort();

         Uri alanContact = alan.getProfile()->getDefaultFrom().uri();
         alanContact.host() = alan.getIp();
         alanContact.port() = alan.getPort();

         TestClientRegistration regBob(&bob);
         TestClientRegistration regSean(&sean);
         TestClientRegistration regAlan(&alan);

         Seq(bob.registerUa(),
             bob.expect(Register_Success, regBob, *TestEndPoint::AlwaysTruePred, WaitForRegistration, bob.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(sean.registerUa(),
             sean.expect(Register_Success, regSean, *TestEndPoint::AlwaysTruePred, WaitForRegistration, sean.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(alan.registerUa(),
             alan.expect(Register_Success, regAlan, *TestEndPoint::AlwaysTruePred, WaitForRegistration, alan.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data offer("v=0\r\n"
                    "o=dumTfm 2087 3916 IN IP4 127.0.0.3\r\n"
                    "s=SIP Call\r\n"
                    "c=IN IP4 127.0.0.3\r\n"
                    "t=0 0\r\n"
                    "m=audio 48172 RTP/AVP 0\r\n"
                    "a=rtpmap:0 PCMU/8000\r\n");

         Data answer("v=0\r\n"
                     "o=dumTfm 44527 44527 IN IP4 localhost\r\n"
                     "s=SIP Call\r\n"
                     "c=IN IP4 127.0.0.4\r\n"
                     "t=0 0\r\n"
                     "m=audio 3456 RTP/AVP 0\r\n"
                     "a=rtpmap:0 PCMU/8000\r\n");

         HeaderFieldValue hfv(offer.data(), offer.size());
         Mime type("application", "sdp");
         SdpContents sdpOffer(hfv, type);

         HeaderFieldValue hfv1(answer.data(), answer.size());
         SdpContents sdpAnswer(hfv1, type);

         TestClientInviteSession invBob(&bob);
         TestServerInviteSession invSean(&sean);

         // bob calls sean
         Seq(bob.invite(sean.getProfile()->getDefaultFrom(), &sdpOffer),
             sean.expect(Invite_NewServerSession, invSean, dumFrom(bob), WaitForCommand, invSean.provisional(180)),
             invSean.expect(Invite_Offer, dumFrom(bob), WaitForCommand, chain(invSean.provideAnswer(sdpAnswer), invSean.accept(200))),
             invSean.expect(Invite_Connected, dumFrom(bob), WaitForCommand, invSean.noAction()),
             bob.expect(Invite_NewClientSession, invBob, dumFrom(sean), WaitForCommand, bob.noAction()),
             invBob.expect(Invite_Provisional, dumFrom(sean), WaitForCommand, invBob.noAction()),
             invBob.expect(Invite_Answer, dumFrom(sean), WaitForCommand, invBob.noAction()),
             invBob.expect(Invite_Connected, dumFrom(sean), WaitForCommand, invBob.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         // bob transfers sean to alan and gets rejected.
         Seq(invBob.refer(NameAddr(alanContact)),
             invSean.expect(Invite_Refer, dumFrom(bob), WaitForCommand, invSean.rejectRefer(406)),
             invBob.expect(Invite_ReferRejected, dumFrom(sean), WaitForCommand, invBob.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regBob.removeMyBindings(true),
             regBob.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regBob.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regSean.removeMyBindings(true),
             regSean.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regSean.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regAlan.removeMyBindings(true),
             regAlan.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regAlan.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }
      
      void testBlindTransfer()
      {
         WarningLog(<< "testBlindTransfer");

         TestClientRegistration regDerek(derek);
         TestClientRegistration regScott(scott);
         TestClientRegistration regJason(jason);


         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);
         TestServerInviteSession invScott(scott);

         TestServerInviteSession invJason(jason);
         TestClientInviteSession invScottClient(scott);

         TestClientSubscription clientSubDerek(derek);

         TestServerSubscription serverSub(scott);

         // derek calls scott
         Seq(derek->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, invScott, dumFrom(derek), WaitForCommand, invScott.noAction()),
             invScott.expect(Invite_Offer, dumFrom(derek), WaitForCommand, 
                             chain(invScott.provisional(180), invScott.provideAnswer(*standardAnswer), invScott.accept(200))),

             And(Sub(invScott.expect(Invite_Connected, dumFrom(derek), WaitForCommand, invScott.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerek, dumFrom(scott), WaitForCommand, derek->noAction()),
                     invDerek.expect(Invite_Provisional, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Answer, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Connected, dumFrom(scott), WaitForCommand, invDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek transfers scott to jason
         Seq(invDerek.refer(jason->getProfile()->getDefaultFrom()),
             invScott.expect(Invite_Refer, dumFrom(derek), WaitForCommand, 
                            chain(invScott.acceptRefer(), 
                                  scott->inviteFromRefer(invScott.getReferMessage(), invScott.getServerSubscription(), standardOffer))),
             And(Sub(And(Sub(jason->expect(Invite_NewServerSession, invJason, dumFrom(scott), WaitForCommand, invJason.noAction()),
                             invJason.expect(Invite_Offer, dumFrom(scott), WaitForCommand, chain(invJason.provideAnswer(*standardAnswer), invJason.accept(200))),
                             invJason.expect(Invite_Connected, dumFrom(scott), WaitForCommand, invJason.noAction())),
                         Sub(scott->expect(ServerSubscription_Terminated, serverSub, *TestEndPoint::AlwaysTruePred, WaitForCommand, serverSub.noAction()),
                             scott->expect(Invite_NewClientSession, invScottClient, dumFrom(jason), WaitForCommand, scott->noAction()),
                             invScottClient.expect(Invite_Answer, dumFrom(jason), WaitForCommand, invScottClient.noAction()),
                             invScottClient.expect(Invite_Connected, dumFrom(jason), WaitForCommand, invScottClient.noAction())))),
                 
                 Sub(invDerek.expect(Invite_ReferAccepted, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), WaitForCommand, clientSubDerek.acceptUpdate(200)),
                     clientSubDerek.expect(ClientSubscription_Terminated, dumFrom(scott), WaitForCommand, clientSubDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invScottClient.end(),
             And(Sub(invScottClient.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invScottClient.noAction())),
                 Sub(invJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(invDerek.end(),
             And(Sub(invDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction())),
                 Sub(invScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testBlindTransferWithOneNotify()
      {
         WarningLog(<< "testBlindTransferWithOneNotify");

         Uri derekContact = derek->getAor().uri();
         derekContact.host() = derek->getIp();
         derekContact.port() = derek->getPort();

         TestClientRegistration regDerek(derek);


         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);
         TestClientSubscription clientSubDerek(derek);
         boost::shared_ptr<SdpContents> ans(static_cast<SdpContents*>(standardAnswer->clone()));

         // derek calls david
         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             And(Sub(david->expect(INVITE, contact(NameAddr(derekContact)), WaitForCommand, chain(david->ring(), david->answer(ans))),
                     david->expect(ACK, contact(NameAddr(derekContact)), WaitForResponse, david->noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerek, dumFrom(proxy), WaitForCommand, derek->noAction()),
                     invDerek.expect(Invite_Provisional, dumFrom(proxy), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Answer, dumFrom(proxy), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Connected, dumFrom(proxy), WaitForCommand, invDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek transfers david, david sends 202 and notify(200OK) to derek.
         Seq(invDerek.refer(jason->getProfile()->getDefaultFrom()),
             david->expect(REFER, contact(NameAddr(derekContact)), WaitForCommand, chain(david->send202(), david->notify200(derekContact))),
             And(Sub(optional(david->expect(NOTIFY/407, from(proxy), WaitForResponse, david->digestRespond()))),
                 Sub(david->expect(NOTIFY/200, contact(NameAddr(derekContact)), WaitForResponse, david->noAction())),
                 Sub(invDerek.expect(Invite_ReferAccepted, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
//                     derek->expect(ClientSubscription_UpdateActive, clientSubDerek, *TestEndPoint::AlwaysTruePred, 
//                                   WaitForCommand, clientSubDerek.acceptUpdate(200)),
                     clientSubDerek.expect(ClientSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientSubDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invDerek.end(),
             And(Sub(invDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction())),
                 Sub(david->expect(BYE, contact(NameAddr(derekContact)), WaitForPause, david->ok()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testTransferNoReferSub()
      {
         WarningLog(<< "testTransferNoReferSub");

         TestClientRegistration regDerek(derek);
         TestClientRegistration regScott(scott);
         TestClientRegistration regJason(jason);


         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Data offer("v=0\r\n"
                    "o=dumTfm 2087 3916 IN IP4 127.0.0.3\r\n"
                    "s=SIP Call\r\n"
                    "c=IN IP4 127.0.0.3\r\n"
                    "t=0 0\r\n"
                    "m=audio 48172 RTP/AVP 0\r\n"
                    "a=rtpmap:0 PCMU/8000\r\n");

         Data answer("v=0\r\n"
                     "o=dumTfm 44527 44527 IN IP4 localhost\r\n"
                     "s=SIP Call\r\n"
                     "c=IN IP4 127.0.0.4\r\n"
                     "t=0 0\r\n"
                     "m=audio 3456 RTP/AVP 0\r\n"
                     "a=rtpmap:0 PCMU/8000\r\n");

         HeaderFieldValue hfv(offer.data(), offer.size());
         Mime type("application", "sdp");
         SdpContents sdpOffer(hfv, type);

         HeaderFieldValue hfv1(answer.data(), answer.size());
         SdpContents sdpAnswer(hfv1, type);

         TestClientInviteSession invDerek(derek);
         TestServerInviteSession invScott(scott);

         TestServerInviteSession invJason(jason);
         TestClientInviteSession invScottClient(scott);

         TestClientSubscription clientSubDerek(derek);

         // derek calls scott
         Seq(derek->invite(scott->getProfile()->getDefaultFrom(), &sdpOffer),
             scott->expect(Invite_NewServerSession, invScott, dumFrom(derek), WaitForCommand, invScott.noAction()),
             invScott.expect(Invite_Offer, dumFrom(derek), WaitForCommand, 
                             chain(invScott.provisional(180), invScott.provideAnswer(sdpAnswer), invScott.accept(200))),

             And(Sub(invScott.expect(Invite_Connected, dumFrom(derek), WaitForCommand, invScott.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerek, dumFrom(scott), WaitForCommand, derek->noAction()),
                     invDerek.expect(Invite_Provisional, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Answer, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Connected, dumFrom(scott), WaitForCommand, invDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek transfers scott to jason
         Seq(invDerek.refer(jason->getProfile()->getDefaultFrom(), false),
             invScott.expect(Invite_ReferNoSub, dumFrom(derek), WaitForCommand, 
                            chain(invScott.acceptReferNoSub(200), 
                                  scott->inviteFromRefer(invScott.getReferMessage(), invScott.getServerSubscription(), &sdpOffer))),
             And(Sub(jason->expect(Invite_NewServerSession, invJason, dumFrom(scott), WaitForCommand, invJason.noAction()),
                     invJason.expect(Invite_Offer, dumFrom(scott), WaitForCommand, chain(invJason.provideAnswer(sdpAnswer), invJason.accept(200))),
                     invJason.expect(Invite_Connected, dumFrom(scott), WaitForCommand, invJason.noAction()),
                     scott->expect(Invite_NewClientSession, invScottClient, dumFrom(jason), WaitForCommand, scott->noAction()),
                     invScottClient.expect(Invite_Answer, dumFrom(jason), WaitForCommand, invScottClient.noAction()),
                     invScottClient.expect(Invite_Connected, dumFrom(jason), WaitForCommand, invScottClient.noAction())),
                 
                 Sub(invDerek.expect(Invite_ReferAccepted, dumFrom(scott), WaitForCommand, invDerek.noAction()))),

             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invScottClient.end(),
             And(Sub(invScottClient.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invScottClient.noAction())),
                 Sub(invJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(invDerek.end(),
             And(Sub(invDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction())),
                 Sub(invScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         //!dcm! -- should be end.
         Seq(regScott.removeMyBindings(true),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJason.removeMyBindings(true),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testServerConnectionTermination()
      {
         WarningLog( << "testServerConnectionTermination");
         TestClientRegistration regDuane(duane);


         Seq(duane->registerUa(),
             sipEndPoint->expect(REGISTER, from(duane->getInstanceId()), WaitForCommand, sipEndPoint->send200ToRegister()),
             duane->expect(Register_Success, regDuane, *TestEndPoint::AlwaysTruePred, WaitForRegistration, 
                           chain(sipEndPoint->closeTransport(), 
                                 regDuane.removeMyBindings(true))),
             WaitForEndOfSeq);
         ExecuteSequences();
      }
      
      class SetServiceRoute
      {
         public:
            SetServiceRoute(const NameAddrs& sRoute) :
               mServiceRoute(sRoute)
            {
            }
               
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg)
            {
               msg->header(h_ServiceRoutes) = mServiceRoute;
               return msg;
            }
         private:
            NameAddrs mServiceRoute;
      };

      void testServiceRoute()
      {
         WarningLog( << "testServiceRoute");
         TestClientRegistration regDuane(duane);

         NameAddrs sRoute;
         sRoute.push_back(serviceEndPoint->getContact());
         sRoute.front().uri().param(p_lr)= true;         
         
         //duane->getProfile()->setImsAuthUri(duane->getAor().uri());         
         duane->getProfile()->setImsAuthUser(duane->getAor().uri().user(), duane->getAor().uri().host());
         
         Seq(duane->registerUa(),
             sipEndPoint->expect(REGISTER, from(duane->getInstanceId()), WaitForCommand, condition(SetServiceRoute(sRoute),
                                                                                                   sipEndPoint->send401())),
             sipEndPoint->expect(REGISTER, from(duane->getInstanceId()), WaitForCommand, condition(SetServiceRoute(sRoute),
                                                                                                   sipEndPoint->send200ToRegister())),
             duane->expect(Register_Success, regDuane, *TestEndPoint::AlwaysTruePred, WaitForRegistration, duane->noAction()),
             WaitForEndOfSeq);

         ExecuteSequences();
         
         TestClientPublication clientPub(duane);
         
         auto_ptr<Pidf> pidf = makePidf(duane);
         
         Seq(duane->publish(NameAddr(sipEndPoint->getContact()), *pidf, Data("presence"), 100),
             serviceEndPoint->expect(PUBLISH, from(duane->getInstanceId()), WaitForResponse, serviceEndPoint->send401()),
             serviceEndPoint->expect(PUBLISH, from(duane->getInstanceId()), WaitForResponse, serviceEndPoint->send200ToPublish()),
             duane->expect(ClientPublication_Success, clientPub, *TestEndPoint::AlwaysTruePred, WaitForCommand, duane->noAction()),
             WaitForEndOfSeq);

         ExecuteSequences();
         
         Seq(clientPub.end(),
             serviceEndPoint->expect(PUBLISH, from(duane->getInstanceId()), WaitForResponse, serviceEndPoint->send200ToPublish()),
             clientPub.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForResponse, clientPub.noAction()),
             WaitForEndOfTest);
             
         ExecuteSequences();

         Seq(duane->invite(NameAddr(sipEndPoint->getContact()), standardOffer),
             serviceEndPoint->expect(INVITE, from(duane->getInstanceId()), WaitForCommand, serviceEndPoint->send403()),
             And(Sub(serviceEndPoint->expect(ACK, alwaysMatches(), WaitForCommand, serviceEndPoint->noAction())),
                 Sub(duane->expect(Invite_Failure, *TestEndPoint::AlwaysTruePred, WaitForResponse, duane->noAction()),
                     duane->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForResponse, duane->noAction()))),
             WaitForEndOfSeq);
         
         ExecuteSequences();

         Seq(regDuane.requestRefresh(),
             serviceEndPoint->expect(REGISTER, from(duane->getInstanceId()), WaitForCommand, serviceEndPoint->send200ToRegister()),
             regDuane.expect(Register_Success, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDuane.noAction()),
             WaitForEndOfSeq);

         ExecuteSequences();

         TestClientPublication clientPub2(duane);
         
         Seq(duane->publish(NameAddr(sipEndPoint->getContact()), *pidf, Data("presence"), 100),
             sipEndPoint->expect(PUBLISH, from(duane->getInstanceId()), WaitForResponse, sipEndPoint->send401()),
             sipEndPoint->expect(PUBLISH, from(duane->getInstanceId()), WaitForResponse, sipEndPoint->send200ToPublish()),
             duane->expect(ClientPublication_Success, clientPub2, *TestEndPoint::AlwaysTruePred, WaitForCommand, duane->noAction()),
             WaitForEndOfSeq);

         ExecuteSequences();
         
         Seq(clientPub2.end(),
             sipEndPoint->expect(PUBLISH, from(duane->getInstanceId()), WaitForResponse, sipEndPoint->send200ToPublish()),
             clientPub2.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForResponse, clientPub2.noAction()),
             WaitForEndOfTest);

         ExecuteSequences();

         Seq(regDuane.removeMyBindings(true),
             sipEndPoint->expect(REGISTER, from(duane->getInstanceId()), WaitForCommand, sipEndPoint->send200ToRegister()),
             regDuane.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDuane.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         //duane->getProfile()->setImsAuthUri(Uri());
         duane->getProfile()->setImsAuthUser(Uri().user(), Uri().host());
      }

      void testIMBasic()
      {
         WarningLog(<< "testIMBasic");
         
         TestClientRegistration clientReg(jason);

         Seq(jason->registerUa(),
             jason->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         ClientPagerMessageHandle h = derek->makePagerMessage(jason->getAor());
         TestClientPagerMessage clientPager(derek, h);
         TestServerPagerMessage srvPager(jason);
         std::auto_ptr<Contents> contents = std::auto_ptr<Contents>(new PlainContents("hello"));

         Seq(clientPager.page(contents),
             jason->expect(ServerPagerMessage_MessageArrived, srvPager, dumFrom(derek), WaitForCommand, srvPager.accept()),
             clientPager.expect(ClientPagerMessage_Success, dumFrom(jason), WaitForCommand, clientPager.end()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

      };

      class SetMergedRequest
      {
         public:
            SetMergedRequest(int seq, const Data& tag, const Data& callId) :
               mSequence(seq),
               mTag(tag),
               mCallId(callId)               
            {
            }
               
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg)
            {
               msg->header(h_CallId).value() = mCallId;
               msg->header(h_From).param(p_tag) = mTag;
               msg->header(h_CSeq).sequence() = mSequence;
               msg->header(h_CSeq).method() = MESSAGE;
               return msg;
            }
         private:
            int mSequence;
            Data mTag;
            Data mCallId;
      };

      void testIMMergedRequestArrivedSameTime()
      {
         WarningLog(<< "testIMMergedRequestArrivedSameTime");

         int sequence = 1;
         Data tag = Helper::computeTag(Helper::tagSize);
         Data callId = Helper::computeCallId();

         TestClientRegistration clientReg(derek);
         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerPagerMessage srvPager(derek);
         Seq(chain(condition(SetMergedRequest(sequence, tag, callId), david->message(derek->getAor().uri(), "hello")),
                   condition(SetMergedRequest(sequence, tag, callId), david->message(derek->getAor().uri(), "hello"))),
             optional(david->expect(MESSAGE/407, from(proxy), WaitForResponse, david->digestRespond())),
             optional(david->expect(MESSAGE/407, from(proxy), WaitForResponse, david->digestRespond())),
             derek->expect(ServerPagerMessage_MessageArrived, srvPager, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvPager.accept()),
             And(Sub(david->expect(MESSAGE/200, from(proxy), WaitForCommand, david->noAction())),
                 Sub(david->expect(MESSAGE/482, from(proxy), WaitForCommand, david->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      };

      void testIMMergedRequestArrivedWithin32Seconds()
      {
         WarningLog(<< "testIMMergedRequestArrivedWithin32Seconds");

         int sequence = 1;
         Data tag = Helper::computeTag(Helper::tagSize);
         Data callId = Helper::computeCallId();

         TestClientRegistration clientReg(derek);
         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerPagerMessage srvPager(derek);
         Seq(condition(SetMergedRequest(sequence, tag, callId), david->message(derek->getAor().uri(), "hello")),
             optional(david->expect(MESSAGE/407, from(proxy), WaitForResponse, david->digestRespond())),
             derek->expect(ServerPagerMessage_MessageArrived, srvPager, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvPager.accept()),
             david->expect(MESSAGE/200, from(proxy), WaitForCommand, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         sleepSeconds(25);

         Seq(condition(SetMergedRequest(sequence, tag, callId), david->message(derek->getAor().uri(), "hello")),
             optional(david->expect(MESSAGE/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(MESSAGE/482, from(proxy), WaitForCommand, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testIMMergedRequestArrivedMoreThan32SecondsApart()
      {
         WarningLog(<< "testIMMergedRequestArrivedMoreThan32SecondsApart");

         int sequence = 1;
         Data tag = Helper::computeTag(Helper::tagSize);
         Data callId = Helper::computeCallId();

         TestClientRegistration clientReg(derek);
         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientReg, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerPagerMessage srvPager(derek);
         Seq(condition(SetMergedRequest(sequence, tag, callId), david->message(derek->getAor().uri(), "hello")),
             optional(david->expect(MESSAGE/407, from(proxy), WaitForResponse, david->digestRespond())),
             derek->expect(ServerPagerMessage_MessageArrived, srvPager, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvPager.accept()),
             david->expect(MESSAGE/200, from(proxy), WaitForCommand, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         sleepSeconds(40);

         Seq(condition(SetMergedRequest(sequence, tag, callId), david->message(derek->getAor().uri(), "hello")),
             optional(david->expect(MESSAGE/407, from(proxy), WaitForResponse, david->digestRespond())),
             derek->expect(ServerPagerMessage_MessageArrived, srvPager, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvPager.accept()),
             david->expect(MESSAGE/200, from(proxy), WaitForCommand, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientReg.end(),
             clientReg.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientReg.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      class RemoveBranchId
      {
         public:
            RemoveBranchId()
            {
            }
               
            boost::shared_ptr<resip::SipMessage> operator()(boost::shared_ptr<resip::SipMessage> msg)
            {
               msg->header(h_Vias).front().remove(p_branch);
               return msg;
            }

      };

      void test401WithoutBranchId()
      {
         WarningLog(<< "test401WithoutBranchId");

         Pidf* pidf = new Pidf;
         boost::shared_ptr<resip::Contents> body(pidf);
         pidf->setSimpleStatus(true, "online", Data::from(derek->getProfile()->getDefaultFrom().uri()));
         pidf->getTuples().front().id = derek->getProfile()->getDefaultFrom().uri().getAor();
         pidf->getTuples().front().attributes["displayName"] = "displayName";
         pidf->setEntity(derek->getAor().uri());

         Uri contact = derek->getAor().uri();
         contact.host() = derek->getIp();
         contact.port() = derek->getPort();
         
         TestClientPublication clientPub(derek);
         
         Seq(derek->publish(NameAddr(david->getContact()), *body, Data("presence"), 100),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, condition(RemoveBranchId(), david->send401())),
             david->expect(PUBLISH, hasMessageBodyMatch(), WaitForResponse, david->send200ToPublish()),
             derek->expect(ClientPublication_Success, clientPub, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientPub.end(),
             david->expect(PUBLISH, from(contact), WaitForResponse, david->send200ToPublish()),
             clientPub.expect(ClientPublication_Remove, *TestEndPoint::AlwaysTruePred, WaitForCommand, clientPub.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
      }

      void testTransferNoReferSubWithoutReferSubHeaderIn202()
      {
         WarningLog(<< "testTransferNoReferSubWithoutReferSubHeaderIn202");

         Uri derekContact = derek->getAor().uri();
         derekContact.host() = derek->getIp();
         derekContact.port() = derek->getPort();

         TestClientRegistration regDerek(derek);


         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);
         TestClientSubscription clientSubDerek(derek);
         boost::shared_ptr<SdpContents> ans(static_cast<SdpContents*>(standardAnswer->clone()));

         // derek calls david
         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             And(Sub(david->expect(INVITE, contact(NameAddr(derekContact)), WaitForCommand, chain(david->ring(), david->answer(ans))),
                     david->expect(ACK, contact(NameAddr(derekContact)), WaitForResponse, david->noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerek, dumFrom(scott), WaitForCommand, derek->noAction()),
                     invDerek.expect(Invite_Provisional, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Answer, dumFrom(scott), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Connected, dumFrom(scott), WaitForCommand, invDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invDerek.refer(jason->getProfile()->getDefaultFrom(), false),
             david->expect(REFER, contact(NameAddr(derekContact)), WaitForCommand, david->send202()),
             invDerek.expect(Invite_ReferAccepted, dumFrom(scott), WaitForCommand, invDerek.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invDerek.end(),
             And(Sub(invDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction())),
                 Sub(david->expect(BYE, contact(NameAddr(derekContact)), WaitForPause, david->ok()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testFailover()
      {
         WarningLog(<< "testFailOver");
         DumUserAgent* jozsef = createUserAgentForFailoverTest("jozsef", domain);
         TestClientRegistration* regJozsef = new TestClientRegistration(jozsef);

         Uri contact = jozsef->getAor().uri();
         contact.host() = jozsef->getIp();
         contact.port() = jozsef->getPort();

         Seq(jozsef->registerUa(),
             registrar->expect(REGISTER, from(contact), 40 * Seconds, registrar->send200ToRegister()),
             jozsef->expect(Register_Success, *regJozsef, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jozsef->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJozsef->end(),
             registrar->expect(REGISTER, from(contact), Seconds, registrar->send200ToRegister()),
             regJozsef->expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJozsef->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         delete regJozsef;
         delete jozsef;
      }

      void testOutOfDialogRefer()
      {
         WarningLog(<< "testOutOfDialogRefer");

         TestClientRegistration regScott(scott);
         TestClientRegistration regJason(jason);


         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession invJason(jason);
         TestClientInviteSession invScottClient(scott);

         TestServerSubscription serverSub(scott);
         TestClientSubscription clientSub(derek);
         
         Uri contactScott = scott->getAor().uri();
         contactScott.host() = scott->getIp();
         contactScott.port() = scott->getPort();

         // derek transfers scott to jason
         Seq(derek->refer(NameAddr(contactScott), jason->getProfile()->getDefaultFrom()),
             scott->expect(ServerSubscription_NewSubscriptionFromRefer, serverSub, dumFrom(derek), WaitForCommand, 
                           chain(serverSub.accept(202), 
                                 scott->inviteFromRefer(scott->getReferMessage(), scott->getServerSubscription(), standardOffer))),

             And(Sub(And(Sub(jason->expect(Invite_NewServerSession, invJason, dumFrom(scott), WaitForCommand, invJason.noAction()),
                             invJason.expect(Invite_Offer, dumFrom(scott), WaitForCommand, chain(invJason.provideAnswer(*standardAnswer), invJason.accept(200))),
                             invJason.expect(Invite_Connected, dumFrom(scott), WaitForCommand, invJason.noAction())),
                         Sub(serverSub.expect(ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, serverSub.noAction()),
                             scott->expect(Invite_NewClientSession, invScottClient, dumFrom(jason), WaitForCommand, scott->noAction()),
                             invScottClient.expect(Invite_Answer, dumFrom(jason), WaitForCommand, invScottClient.noAction()),
                             invScottClient.expect(Invite_Connected, dumFrom(jason), WaitForCommand, invScottClient.noAction())))),
                 
                 Sub(derek->expect(ClientSubscription_UpdateActive, clientSub, dumFrom(scott), WaitForCommand, clientSub.acceptUpdate(200)),
                     clientSub.expect(ClientSubscription_Terminated, dumFrom(scott), WaitForCommand, clientSub.noAction()))),

             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invScottClient.end(),
             And(Sub(invScottClient.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invScottClient.noAction())),
                 Sub(invJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testOutOfDialogRefer406()
      {
         WarningLog(<< "testOutOfDialogRefer406");

         TestClientRegistration regScott(scott);

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerSubscription srvSub(scott);

         Uri contactScott = scott->getAor().uri();
         contactScott.host() = scott->getIp();
         contactScott.port() = scott->getPort();

         // derek transfers scott to jason
         Seq(derek->refer(NameAddr(contactScott), jason->getProfile()->getDefaultFrom()),
             scott->expect(ServerSubscription_NewSubscriptionFromRefer, srvSub, dumFrom(derek), WaitForCommand, srvSub.reject(406)),

             And(Sub(srvSub.expect(ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, srvSub.noAction())),
                 Sub(derek->expect(ClientSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testOutOfDialogReferNoReferSubWithoutReferSubHeader()
      {
         WarningLog(<< "testOutOfDialogReferNoReferSubWithoutReferSubHeader");

         TestClientRegistration regScott(scott);

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerOutOfDialogReq srvOutOfDialogReq(scott);
         
         Uri contactScott = scott->getAor().uri();
         contactScott.host() = scott->getIp();
         contactScott.port() = scott->getPort();

         // derek transfers scott to jason
         Seq(derek->referNoReferSubWithoutReferSubHeader(NameAddr(contactScott), jason->getProfile()->getDefaultFrom()),
             scott->expect(ServerOutOfDialogReq_ReceivedRequest, srvOutOfDialogReq, dumFrom(derek), WaitForCommand, srvOutOfDialogReq.accept()),                 
             derek->expect(ClientOutOfDialogReq_Success, dumFrom(scott), WaitForCommand, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testOutOfDialogReferNoReferSub()
      {
         WarningLog(<< "testOutOfDialogReferNoReferSub");

         TestClientRegistration regScott(scott);

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerOutOfDialogReq srvOutOfDialogReq(scott);
         
         Uri contactScott = scott->getAor().uri();
         contactScott.host() = scott->getIp();
         contactScott.port() = scott->getPort();

         // derek transfers scott to jason
         Seq(derek->referNoReferSub(NameAddr(contactScott), jason->getProfile()->getDefaultFrom()),
             scott->expect(ServerOutOfDialogReq_ReceivedRequest, srvOutOfDialogReq, dumFrom(derek), WaitForCommand, srvOutOfDialogReq.accept()),                 
             derek->expect(ClientOutOfDialogReq_Success, dumFrom(scott), WaitForCommand, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testAttendedTransferTransferorAsUAC()
      {
         WarningLog(<< "testAttendedTransferTransferorAsUAC");

         TestClientRegistration regScott(scott);
         TestClientRegistration regJason(jason);

         TestClientInviteSession invDerekToScott(derek);
         TestClientInviteSession invDerekToJason(derek);

         TestClientInviteSession invClientScott(scott);
         TestServerInviteSession invSrvScott(scott);
         TestServerInviteSession invSrvJasonFromDerek(jason);
         TestServerInviteSession invSrvJasonFromScott(jason);

         TestClientSubscription clientSubDerek(derek);
         TestServerSubscription serverSub(scott);

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek calls scott
         Seq(derek->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, invSrvScott, dumFrom(derek), TimeOut, invSrvScott.noAction()),
             invSrvScott.expect(Invite_Offer, dumFrom(derek), TimeOut, 
                                chain(invSrvScott.provisional(180), invSrvScott.provideAnswer(*standardAnswer), invSrvScott.accept(200))),

             And(Sub(invSrvScott.expect(Invite_Connected, dumFrom(derek), TimeOut, invSrvScott.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerekToScott, dumFrom(scott), TimeOut, derek->noAction()),
                     invDerekToScott.expect(Invite_Provisional, dumFrom(scott), TimeOut, invDerekToScott.noAction()),
                     invDerekToScott.expect(Invite_Answer, dumFrom(scott), TimeOut, invDerekToScott.noAction()),
                     invDerekToScott.expect(Invite_Connected, dumFrom(scott), TimeOut, invDerekToScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek calls jason
         Seq(derek->invite(jason->getProfile()->getDefaultFrom(), standardOffer),
             jason->expect(Invite_NewServerSession, invSrvJasonFromDerek, dumFrom(derek), TimeOut, invSrvJasonFromDerek.noAction()),
             invSrvJasonFromDerek.expect(Invite_Offer, dumFrom(derek), TimeOut, 
                                         chain(invSrvJasonFromDerek.provisional(180), invSrvJasonFromDerek.provideAnswer(*standardAnswer), 
                                               invSrvJasonFromDerek.accept(200))),

             And(Sub(invSrvJasonFromDerek.expect(Invite_Connected, dumFrom(derek), TimeOut, invSrvJasonFromDerek.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerekToJason, dumFrom(jason), TimeOut, derek->noAction()),
                     invDerekToJason.expect(Invite_Provisional, dumFrom(jason), TimeOut, invDerekToJason.noAction()),
                     invDerekToJason.expect(Invite_Answer, dumFrom(jason), TimeOut, invDerekToJason.noAction()),
                     invDerekToJason.expect(Invite_Connected, dumFrom(jason), TimeOut, invDerekToJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek refers scott to jason
         Seq(invDerekToScott.refer(jason->getProfile()->getDefaultFrom(), invDerekToJason.getSessionHandle()),
             invSrvScott.expect(Invite_Refer, dumFrom(derek), TimeOut, 
                                chain(invSrvScott.acceptRefer(), 
                                      scott->inviteFromRefer(invSrvScott.getReferMessage(), invSrvScott.getServerSubscription(), standardOffer))),

             And(Sub(And(Sub(jason->expect(Invite_NewServerSession, invSrvJasonFromScott, findMatchingDialogToReplace(jason), 
                                           TimeOut, invSrvJasonFromScott.noAction()),
                             invSrvJasonFromScott.expect(Invite_Offer, dumFrom(scott), TimeOut, 
                                                         chain(invSrvJasonFromScott.provideAnswer(*standardAnswer), invSrvJasonFromScott.accept(200))),
                             invSrvJasonFromScott.expect(Invite_Connected, dumFrom(scott), TimeOut, invSrvJasonFromDerek.end())),
                         Sub(invSrvJasonFromDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, 
                                                         invSrvJasonFromDerek.noAction())))),

                 Sub(scott->expect(ServerSubscription_Terminated, serverSub, *TestEndPoint::AlwaysTruePred, TimeOut*10, serverSub.noAction()),
                     scott->expect(Invite_NewClientSession, invClientScott, dumFrom(jason), TimeOut, scott->noAction()),
                     invClientScott.expect(Invite_Answer, dumFrom(jason), TimeOut, invClientScott.noAction()),
                     invClientScott.expect(Invite_Connected, dumFrom(jason), TimeOut, invClientScott.noAction())),

                 Sub(And(Sub(/*derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), TimeOut, 
                               clientSubDerek.acceptUpdate(200)),*/
                            invDerekToScott.expect(Invite_ReferAccepted, dumFrom(scott), TimeOut, invDerekToScott.noAction()),
                            derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), TimeOut, 
                                          clientSubDerek.acceptUpdate(200)),
                            clientSubDerek.expect(ClientSubscription_Terminated, dumFrom(scott), TimeOut*10, clientSubDerek.noAction())),
                         Sub(invDerekToJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invDerekToJason.noAction()))))),
             
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(invDerekToScott.end(),
             And(Sub(invDerekToScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invDerekToScott.noAction())),
                 Sub(invSrvScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invSrvScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(invSrvJasonFromScott.end(),
             And(Sub(invSrvJasonFromScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invSrvJasonFromScott.noAction())),
                 Sub(invClientScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invClientScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

      }

      void testAttendedTransferTransferorAsUAS()
      {
         WarningLog(<< "testAttendedTransferTransferorAsUAS");

         TestClientRegistration regScott(scott);
         TestClientRegistration regJason(jason);
         TestClientRegistration regDerek(derek);

         //TestClientInviteSession invDerekToScott(derek);
         TestServerInviteSession invDerekFromScott(derek);

         TestClientInviteSession invDerekToJason(derek);

         TestClientInviteSession invClientScott(scott);
         //TestServerInviteSession invSrvScott(scott);
         TestClientInviteSession invClientScottToDerek(scott);

         TestServerInviteSession invSrvJasonFromDerek(jason);
         TestServerInviteSession invSrvJasonFromScott(jason);

         TestClientSubscription clientSubDerek(derek);
         TestServerSubscription serverSub(scott);

         Seq(derek->registerUa(),
             derek->expect(Register_Success, regDerek, *TestEndPoint::AlwaysTruePred, WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, *TestEndPoint::AlwaysTruePred, WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, *TestEndPoint::AlwaysTruePred, WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         // scott calls derek
         Seq(scott->invite(derek->getProfile()->getDefaultFrom(), standardOffer),
             derek->expect(Invite_NewServerSession, invDerekFromScott, dumFrom(scott), TimeOut, invDerekFromScott.noAction()),
             invDerekFromScott.expect(Invite_Offer, dumFrom(scott), TimeOut, 
                                chain(invDerekFromScott.provisional(180), invDerekFromScott.provideAnswer(*standardAnswer), invDerekFromScott.accept(200))),

             And(Sub(invDerekFromScott.expect(Invite_Connected, dumFrom(scott), TimeOut, invDerekFromScott.noAction())),
                 Sub(scott->expect(Invite_NewClientSession, invClientScottToDerek, dumFrom(derek), TimeOut, scott->noAction()),
                     invClientScottToDerek.expect(Invite_Provisional, dumFrom(derek), TimeOut, invClientScottToDerek.noAction()),
                     invClientScottToDerek.expect(Invite_Answer, dumFrom(derek), TimeOut, invClientScottToDerek.noAction()),
                     invClientScottToDerek.expect(Invite_Connected, dumFrom(derek), TimeOut, invClientScottToDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek calls jason
         Seq(derek->invite(jason->getProfile()->getDefaultFrom(), standardOffer),
             jason->expect(Invite_NewServerSession, invSrvJasonFromDerek, dumFrom(derek), TimeOut, invSrvJasonFromDerek.noAction()),
             invSrvJasonFromDerek.expect(Invite_Offer, dumFrom(derek), TimeOut, 
                                         chain(invSrvJasonFromDerek.provisional(180), invSrvJasonFromDerek.provideAnswer(*standardAnswer), 
                                               invSrvJasonFromDerek.accept(200))),

             And(Sub(invSrvJasonFromDerek.expect(Invite_Connected, dumFrom(derek), TimeOut, invSrvJasonFromDerek.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerekToJason, dumFrom(jason), TimeOut, derek->noAction()),
                     invDerekToJason.expect(Invite_Provisional, dumFrom(jason), TimeOut, invDerekToJason.noAction()),
                     invDerekToJason.expect(Invite_Answer, dumFrom(jason), TimeOut, invDerekToJason.noAction()),
                     invDerekToJason.expect(Invite_Connected, dumFrom(jason), TimeOut, invDerekToJason.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         // derek refers scott to jason
         Seq(invDerekFromScott.refer(jason->getProfile()->getDefaultFrom(), invDerekToJason.getSessionHandle()),
             invClientScottToDerek.expect(Invite_Refer, dumFrom(derek), TimeOut, 
                                chain(invClientScottToDerek.acceptRefer(), 
                                      scott->inviteFromRefer(invClientScottToDerek.getReferMessage(), 
                                                             invClientScottToDerek.getServerSubscription(), standardOffer))),

             And(Sub(And(Sub(jason->expect(Invite_NewServerSession, invSrvJasonFromScott, findMatchingDialogToReplace(jason), 
                                           TimeOut, invSrvJasonFromScott.noAction()),
                             invSrvJasonFromScott.expect(Invite_Offer, dumFrom(scott), TimeOut, 
                                                         chain(invSrvJasonFromScott.provideAnswer(*standardAnswer), invSrvJasonFromScott.accept(200))),
                             invSrvJasonFromScott.expect(Invite_Connected, dumFrom(scott), TimeOut, invSrvJasonFromDerek.end())),
                         Sub(invSrvJasonFromDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, 
                                                         invSrvJasonFromDerek.noAction())))),

                 Sub(scott->expect(ServerSubscription_Terminated, serverSub, *TestEndPoint::AlwaysTruePred, TimeOut*10, serverSub.noAction()),
                     scott->expect(Invite_NewClientSession, invClientScott, dumFrom(jason), TimeOut, scott->noAction()),
                     invClientScott.expect(Invite_Answer, dumFrom(jason), TimeOut, invClientScott.noAction()),
                     invClientScott.expect(Invite_Connected, dumFrom(jason), TimeOut, invClientScott.noAction())),

                 Sub(And(Sub(/*derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), TimeOut, 
                               clientSubDerek.acceptUpdate(200)),*/
                            invDerekFromScott.expect(Invite_ReferAccepted, dumFrom(scott), TimeOut, invDerekFromScott.noAction()),
                            derek->expect(ClientSubscription_UpdateActive, clientSubDerek, dumFrom(scott), TimeOut, 
                                          clientSubDerek.acceptUpdate(200)),
                            clientSubDerek.expect(ClientSubscription_Terminated, dumFrom(scott), TimeOut*10, clientSubDerek.noAction())),
                         Sub(invDerekToJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invDerekToJason.noAction()))))),
             
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(invDerekFromScott.end(),
             And(Sub(invDerekFromScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invDerekFromScott.noAction())),
                 Sub(invClientScottToDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invClientScottToDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invSrvJasonFromScott.end(),
             And(Sub(invSrvJasonFromScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invSrvJasonFromScott.noAction())),
                 Sub(invClientScott.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, TimeOut, invClientScott.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regDerek.end(),
             regDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regScott.end(),
             regScott.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regScott.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testReInviteRemoveVideo()
      {
         WarningLog(<< "testReInviteRemoveVideo");

         TestClientRegistration regJason(jason);
         
         Seq(jason->registerUa(),
             jason->expect(Register_Success, regJason, dumFrom(proxy), WaitForRegistration, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uacDerek(derek);
         TestServerInviteSession uasJason(jason);

         Seq(derek->invite(jason->getAor(), standardOffer),
             jason->expect(Invite_NewServerSession, uasJason, dumFrom(*derek), WaitForCommand, jason->noAction()),
             uasJason.expect(Invite_Offer, dumFrom(*derek), WaitForCommand, 
                             chain(uasJason.provisional(180), uasJason.provideAnswer(*standardAnswer), uasJason.accept(200))),

             And(Sub(uasJason.expect(Invite_Connected, dumFrom(*derek), WaitForCommand, uasJason.noAction())),
                 Sub(derek->expect(Invite_NewClientSession, uacDerek, dumFrom(*jason), WaitForCommand, derek->noAction()),
                     uacDerek.expect(Invite_Provisional, dumFrom(*jason), WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Answer, dumFrom(*jason), WaitForCommand, uacDerek.noAction()),
                     uacDerek.expect(Invite_Connected, dumFrom(*jason), WaitForCommand, uacDerek.noAction()))),

             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uasJason.provideOffer(*standardOfferAudio),
             uacDerek.expect(Invite_Offer, dumFrom(*jason), 2000, uacDerek.provideAnswer(*standardAnswerAudio)),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(uacDerek.end(),
             uacDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uacDerek.noAction()),
             uasJason.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uasJason.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(regJason.end(),
             regJason.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, regJason.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();

         sleepSeconds(3);
      }

      void testInDialogSubscribeSuccess()
      {
         WarningLog(<< "testInDialogSubscribeSuccess");

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);
         boost::shared_ptr<SdpContents> ans(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             david->expect(INVITE, from(proxy), WaitForCommand*10, chain(david->ring(), david->answer(ans))),
             //!dcm! -- need to unify from/dumFrom...
             derek->expect(Invite_NewClientSession, invDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
             invDerek.expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             invDerek.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             invDerek.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             david->expect(ACK, contact(derek->getContact()), WaitForResponse, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerSubscription serv(derek);         
         Seq(david->subscribe(derek->getAor().uri(), Token("presence")),
             optional(david->expect(SUBSCRIBE/407, from(proxy), WaitForResponse, david->digestRespond())),
             derek->expect(ServerSubscription_NewSubscription, serv, *TestEndPoint::AlwaysTruePred, WaitForCommand, 
                           chain(serv.accept(), serv.neutralNotify())),
             david->expect(SUBSCRIBE/200, contact(derek->getContact()), WaitForResponse, david->noAction()),
             david->expect(NOTIFY, contact(derek->getContact()), WaitForCommand, david->respond(200)),             
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(serv.end(),
             david->expect(NOTIFY, contact(derek->getContact()), WaitForCommand, david->respond(200)),  
             WaitForEndOfTest);
         ExecuteSequences();

         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      /**
       * When we send an INVITE, we generally get a response message of either 1xx or 2xx.
       * In this test, we want to receive a SUBSCRIBE request as a dialog-creating message
       * to the initial INVITE request. Something like the call flow below.  For brevity,
       * only the first few messages are shown:
       *
       * A            P            B
       * |----INV---->|----------->|
       * |<-----------|<----SUB----|
       * |<-----------|<--200/INV--|
       * |---ACK/200(SUB)/NOTIFY-->|      // all these messages in response to B in any order.
       * ...
       *
       */
      void testUASSubscribeCreatingDialogFromInvite()
      {
         WarningLog(<< "testUASSubscribeCreatingDialogFromInvite");
         david->clean();

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);

         boost::shared_ptr<SipMessage> invMsg;
         TestServerSubscription serv(derek);

         Seq(
            derek->invite(NameAddr(david->getUri()), standardOffer),
            david->expect(
                  INVITE,
                  saveMatcher(from(proxy), invMsg),
                  WaitForCommand,
                  chain(david->subscribe(derek->getAor().uri(), Token("presence")), david->answerTo(invMsg))),
            //!dcm! -- need to unify from/dumFrom...
            And(
               Sub(
                  optional(david->expect(SUBSCRIBE/407, from(proxy), WaitForResponse, david->digestRespond()))),
               Sub(
                  derek->expect(Invite_NewClientSession, invDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
                  invDerek.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
                  invDerek.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
                  david->expect(ACK, contact(derek->getContact()), WaitForResponse, derek->noAction())),
               Sub(
                  derek->expect(
                     ServerSubscription_NewSubscription,
                     serv, *TestEndPoint::AlwaysTruePred, WaitForCommand,
                     chain(serv.accept(), serv.neutralNotify()))),
               Sub(david->expect(SUBSCRIBE/200, contact(derek->getContact()), WaitForResponse, david->noAction())),
               Sub(david->expect(NOTIFY, contact(derek->getContact()), WaitForCommand, david->respond(200))),
               serv.end()),
            And(
               Sub(serv.expect(
                        ServerSubscription_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, serv.noAction())),
               Sub(david->expect(NOTIFY, contact(derek->getContact()), WaitForCommand, david->respond(200)))),
            WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testInDialogSubscribeWithoutEventHandler()
      {
         WarningLog(<< "testInDialogSubscribeWithoutEventHandler");

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);
         boost::shared_ptr<SdpContents> ans(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             david->expect(INVITE, from(proxy), WaitForCommand*10, chain(david->ring(), david->answer(ans))),
             //!dcm! -- need to unify from/dumFrom...
             derek->expect(Invite_NewClientSession, invDerek, *TestEndPoint::AlwaysTruePred, WaitForCommand, derek->noAction()),
             invDerek.expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             invDerek.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             invDerek.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction()),
             david->expect(ACK, contact(derek->getContact()), WaitForResponse, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->subscribe(derek->getAor().uri(), Token("unknownEvent")),
             optional(david->expect(SUBSCRIBE/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(SUBSCRIBE/489, from(derek->getContact()), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();
         
         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testOutOfDialogSubscribeWithoutEventHandler()
      {
         WarningLog(<< "testOutOfDialogSubscribeWithoutEventHandler");

         TestClientRegistration clientRegDerek(derek);
         Seq(derek->registerUa(),
             derek->expect(Register_Success, clientRegDerek, *TestEndPoint::AlwaysTruePred, 
                           WaitForRegistration, derek->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientSubscription clientSub(jason);

         Seq(jason->subscribe(NameAddr(derek->getAor()), Data("unknownEvent")),
             jason->expect(ClientSubscription_Terminated, clientSub, dumFrom(*derek), WaitForCommand, jason->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(clientRegDerek.end(),
             clientRegDerek.expect(Register_Removed, *TestEndPoint::AlwaysTruePred, WaitForRegistration, clientRegDerek.noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testUACPrackInviteBasicSuccess()
      {
         InfoLog(<< "testUACPrackInviteBasicSuccess");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), sheila->answer())),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackEmpty180rel()
      {
         InfoLog(<< "testUACPrackEmpty180rel");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));

         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, 
                            sheila->reliableProvisional(1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), sheila->answer(answer))),

             And(Sub(uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

     void testUACPrackMultiple180rel()
      {
         InfoLog(<< "testUACPrackMultiple180rel");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), sheila->reliableProvisional(answer, 2))),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), sheila->answer())),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackUpdate()
      {
         InfoLog(<< "testUACPrackUpdate");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), uac.provideOffer(*standardOffer))),
             sheila->expect(UPDATE, from(jason->getInstanceId()), WaitForCommand, sheila->answerUpdate(answer)),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->answer()),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackUac491()
      {
         InfoLog(<< "testUACPrackUac491");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), uac.provideOffer(*standardOffer))),
             sheila->expect(UPDATE, from(jason->getInstanceId()), WaitForCommand, chain(sheila->update("jason", answer), sheila->send491())),
             sheila->expect(UPDATE/491, alwaysMatches(), WaitForCommand, sheila->noAction()), 
             sheila->expect(UPDATE, from(jason->getInstanceId()), Owner491, sheila->answerUpdate(answer)), 
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->answer()),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackUas491()
      {
         InfoLog(<< "testUACPrackUas491");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), uac.provideOffer(*standardOffer))),
             sheila->expect(UPDATE, from(jason->getInstanceId()), WaitForCommand, chain(sheila->update("jason", answer), sheila->send491())),
             sheila->expect(UPDATE/491, alwaysMatches(), WaitForCommand, chain(sheila->pause(1000), sheila->update("jason", answer))),
             uac.expect(Invite_OfferRejected, *TestEndPoint::AlwaysTruePred, Owner491, uac.noAction()),
             uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             sheila->expect(UPDATE/200, from(jason->getInstanceId()), WaitForCommand, sheila->answer()),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackUasUpdate()
      {
         InfoLog(<< "testUACPrackUasUpdate");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);

         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), sheila->update("jason", answer))),
             uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             sheila->expect(UPDATE/200, from(jason->getInstanceId()), WaitForCommand, sheila->answer()),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackUasDoubleUpdate()
      {
         InfoLog(<< "testUACPrackUasDoubleUpdate");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);

         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->reliableProvisional(answer, 1)),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), sheila->update("jason", answer))),
             uac.expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, sheila->update("jason", answer)),  // 2nd update before previous one is answered
             sheila->expect(UPDATE/500, alwaysMatches(), WaitForCommand, uac.provideAnswer(*standardAnswer)),
             sheila->expect(UPDATE/200, from(jason->getInstanceId()), WaitForCommand, sheila->answer()),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackRetrans()
      {
         InfoLog(<< "testUACPrackRetrans");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         boost::shared_ptr<resip::SipMessage> prov;

         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, save(prov, sheila->reliableProvisional(answer, 1))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             uac.expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             sheila->expect(PRACK, from(jason->getInstanceId()), WaitForCommand, chain(sheila->ok(), 
                                                                                       sheila->pause(1000),
                                                                                       sheila->retransmit(prov), 
                                                                                       sheila->answer())),
             And(Sub(uac.expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(sheila->expect(ACK, from(jason->getInstanceId()), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testUACPrackOptionTag()
      {
         InfoLog(<< "testUACPrackOptionTag");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Required);
         TestSipEndPoint* sheila = sipEndPoint;
         TestClientInviteSession uac(jason);
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));
         
         boost::shared_ptr<resip::SipMessage> prov;

         Seq(jason->invite(sheila->getContact(), standardOffer),
             sheila->expect(INVITE, from(jason->getInstanceId()), WaitForCommand, sheila->send420(Token(Symbols::C100rel))),
             And(Sub(jason->expect(Invite_Failure, *TestEndPoint::AlwaysTruePred, WaitForResponse, jason->noAction()),
                     jason->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForResponse, jason->noAction())),
                 Sub(sheila->expect(ACK, alwaysMatches(), WaitForResponse, sheila->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();
         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
      }

      void testByesCrossedOnTheWire()
      {
         WarningLog(<< "testByesCrossedOnTheWire");

         Uri derekContact = derek->getAor().uri();
         derekContact.host() = derek->getIp();
         derekContact.port() = derek->getPort();

         TestClientRegistration regDerek(derek);

         Seq(david->registerUser(60, david->getDefaultContacts()),
             optional(david->expect(REGISTER/407, from(proxy), WaitForResponse, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForResponse, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession invDerek(derek);

         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(derek->invite(NameAddr(david->getAddressOfRecord()), standardOffer),
             And(Sub(david->expect(INVITE, contact(NameAddr(derekContact)), WaitForCommand, chain(david->ring(), david->answer(answer))),
                     david->expect(ACK, contact(NameAddr(derekContact)), WaitForResponse, david->noAction())),
                 Sub(derek->expect(Invite_NewClientSession, invDerek, dumFrom(proxy), WaitForCommand, derek->noAction()),
                     invDerek.expect(Invite_Provisional, dumFrom(proxy), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Answer, dumFrom(proxy), WaitForCommand, invDerek.noAction()),
                     invDerek.expect(Invite_Connected, dumFrom(proxy), WaitForCommand, invDerek.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(invDerek.end(),
             And(Sub(invDerek.expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, invDerek.noAction())),
                 Sub(david->expect(BYE, contact(NameAddr(derekContact)), WaitForPause, chain(david->bye(), david->pause(100), david->ok())),
                     david->expect(BYE/200, contact(NameAddr(derekContact)), WaitForPause, david->noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         Seq(david->removeRegistrationBindings(),
             optional(david->expect(REGISTER/407, from(proxy), WaitForRegistration, david->digestRespond())),
             david->expect(REGISTER/200, from(proxy), WaitForRegistration, david->noAction()),
             WaitForEndOfTest);
         ExecuteSequences();
      }

      void testPrack3GPP_1() 
      {
         InfoLog(<< "testPrack3GPP_1: 3GPP 24.930 Rel 11 - 5.1.2, 5.2.2");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideOffer(*standardOffer) /* jason to send update */),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideAnswer(*standardAnswer)), 
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provisional(180), uas.accept())),
             And(Sub(jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrack3GPP_2() 
      {
         InfoLog(<< "testPrack3GPP_2: 3GPP 24.930 Rel 11 - 5.2.3, 5.3.4");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provisional(180), uas.accept())),
             And(Sub(jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrack3GPP_3()
      {
         InfoLog(<< "testPrack3GPP_3: 3GPP 24.930 Rel 11 - 5.3.2, 5.5.2, 5.6.2");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(180))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrack3GPP_4() 
      {
         InfoLog(<< "testPrack3GPP_4: 3GPP 24.930 Rel 11 - 5.3.3, 5.6.2");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrack3GPP_5()
      {
         InfoLog(<< "testPrack3GPP_5: 3GPP 24.930 Rel 11 - 5.4.2, 5.4.4");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(180))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideOffer(*standardOffer))),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideAnswer(*standardAnswer)),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrack3GPP_6() 
      {
         InfoLog(<< "testPrack3GPP_6: 3GPP 24.930 Rel 11 - 5.4.3");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         boost::shared_ptr<SdpContents> prackOffer(static_cast<SdpContents*>(standardOffer->clone()));

         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, jason->setOfferToProvideInNextOnAnswerCallback(prackOffer) /* special flag to instruct jason to send new offer in PRACK - requires calling directly from onAnswer callback */),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideAnswer(*standardAnswer)),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideOffer(*standardOffer) /* jason to send new offer in UPDATE */),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideAnswer(*standardAnswer)), 
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provisional(180), uas.accept())),
             And(Sub(jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackInviteNoOffer() 
      {
         InfoLog(<< "testPrackInviteNoOffer");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), 0 /* No Offer */),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_OfferRequired, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideOffer(*standardOffer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             scott->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackInviteNoOffer2ndProvisionalOffer() 
      {
         InfoLog(<< "testPrackInviteNoOffer2ndProvisionalOffer");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), 0 /* No Offer */),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_OfferRequired, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideOffer(*standardOffer), uas.provisional(183, false))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provisional(183, true)),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             scott->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackInviteOffer2ndReliableProvisionalAnswer() 
      {
         InfoLog(<< "testPrackInviteOffer2ndReliableProvisionalAnswer");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);  // Not using SupportEssential will allow reliable provisionals with no sdp
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, false))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provisional(183, true)),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackInviteOffer2ndRelProvAnswerPrackOffer() 
      {
         InfoLog(<< "testPrackInviteOffer2ndRelProvAnswerPrackOffer");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);  // Not using SupportEssential will allow reliable provisionals with no sdp
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         boost::shared_ptr<SdpContents> prackOffer(static_cast<SdpContents*>(standardOffer->clone()));

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, false))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provisional(183, true)),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, jason->setOfferToProvideInNextOnAnswerCallback(prackOffer)),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.accept())),
             And(Sub(jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackNegotiatedReliableProvisional() 
      {
         InfoLog(<< "testPrackNegotiatedReliableProvisional");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provisional(183, false)), // will cause a queued provisional to be send in NegotiatedReliable state
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),  // Note:  accept before prack will cause 200 to be queued - interesting test case
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackNegotiatedReliableUASUpdateFast() 
      {
         InfoLog(<< "testPrackNegotiatedReliableUASUpdateFast");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
              // Answer, Provisional, Offer (UPDATE UAS->UAC) then accept all at once
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true), uas.provideOffer(*standardOffer), uas.accept())),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()), 
             jason->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             scott->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackNegotiatedReliableUpdates() 
      {
         InfoLog(<< "testPrackNegotiatedReliableUpdates");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideOffer(*standardOffer)),  // UPDATE UAS->UAC
             jason->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)),
             scott->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideOffer(*standardOffer)), // UPDATE UAC->UAS
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideAnswer(*standardAnswer)),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackNegotiatedReliableUpdateGlare() 
      {
         InfoLog(<< "testPrackNegotiatedReliableUpdateGlare");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uac.provideOffer(*standardOffer), uas.provideOffer(*standardOffer))),   // UPDATE UAC->UAS, UPDATE UAS->UAC
             jason->expect(Invite_OfferRejected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.provideAnswer(*standardAnswer)), 
             scott->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrack1xxResubmission() 
      {
         InfoLog(<< "testPrack1xxResubmission (takes 2.5 mintues to run)");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, 150000 + WaitForCommand, uac.noAction()),  // Wait 2.5 mins + 1000ms buffer time
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testPrackRequiredWithSupportedEssential() 
      {
         InfoLog(<< "testPrackRequiredWithSupportedEssential");

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Required);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Required);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::SupportedEssential);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::SupportedEssential);

         TestClientRegistration regScott(scott);
         
         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestClientInviteSession uac(jason);
         TestServerInviteSession uas(scott);
         
         Seq(jason->invite(scott->getProfile()->getDefaultFrom(), standardOffer),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, false))),  // should be reliable provisional (no answer) due to required in invite
             jason->expect(Invite_NewClientSession, uac, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             jason->expect(Invite_Provisional, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(jason->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction()),
                     jason->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uac.noAction())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         jason->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         jason->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrackNegotiatedReliableUpdateGlareResend() 
      {
         InfoLog(<< "testUASPrackNegotiatedReliableUpdateGlareResend");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getProfile()->getDefaultFrom().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideOffer(*standardOffer)),  // UAS send UPDATE
             david->expect(PRACK/200, from(scott->getInstanceId()), WaitForCommand, david->noAction()),
             david->expect(UPDATE, from(scott->getInstanceId()), WaitForCommand, david->send491()),
             david->expect(UPDATE, from(scott->getInstanceId()), 5000, david->answerUpdate(answer)),   // Expect UAS to resend UPDATE within 5 seconds
             scott->expect(Invite_Answer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.accept()),
             And(Sub(david->expect(INVITE/200, from(scott->getInstanceId()), WaitForCommand, david->ack())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrackNegotiatedReliableUpdateGlareCrossed() 
      {
         InfoLog(<< "testUASPrackNegotiatedReliableUpdateGlareCrossed");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provideOffer(*standardOffer)),  // UAS send UPDATE
             david->expect(PRACK/200, from(scott->getInstanceId()), WaitForCommand, david->noAction()),
             david->expect(UPDATE, from(scott->getInstanceId()), WaitForCommand, chain(david->send491(), david->update(scott->getAor().uri().user(), offer))),  // UAC send 491 to UPDATE and send crossed UPDATE to UAS
             optional(david->expect(UPDATE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(UPDATE/407, from(proxy), WaitForCommand, david->digestRespond()),
             And(Sub(scott->expect(Invite_OfferRejected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
                     scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.accept())),
                     scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction())),
                 Sub(optional(david->expect(UPDATE/100, from(proxy), WaitFor100, david->noAction())),
                     david->expect(UPDATE/200, from(scott->getInstanceId()), WaitForCommand, uas.noAction()),
                     david->expect(INVITE/200, from(scott->getInstanceId()), WaitForCommand, david->ack()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrackNegotiatedReliableUACDoubleUpdate() 
      {
         InfoLog(<< "testUASPrackNegotiatedReliableUACDoubleUpdate");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, david->update(scott->getAor().uri().user(), offer)),
             david->expect(PRACK/200, from(scott->getInstanceId()), WaitForCommand, david->noAction()),
             optional(david->expect(UPDATE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(UPDATE/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(UPDATE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, david->update(scott->getAor().uri().user(), offer)),  // 2nd offer - before answer for first has been sent
             optional(david->expect(UPDATE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(UPDATE/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(UPDATE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(UPDATE/500, alwaysMatches(), WaitForCommand, uas.provideAnswer(*standardAnswer)),
             david->expect(UPDATE/200, from(scott->getInstanceId()), WaitForCommand, uas.accept()),
             And(Sub(david->expect(INVITE/200, from(scott->getInstanceId()), WaitForCommand, david->ack())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrackNegotiatedReliableOfferInPrack() 
      {
         InfoLog(<< "testUASPrackNegotiatedReliableOfferInPrack");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, chain(uas.provisional(183, false), david->digestRespond())),  // will cause a queued provisional to be send in NegotiatedReliable state
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             david->expect(PRACK/200, from(scott->getInstanceId()), WaitForCommand, david->noAction()),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack(offer)),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.accept())),
             And(Sub(david->expect(PRACK/200, from(scott->getInstanceId()), WaitForCommand, david->noAction()),
                     david->expect(INVITE/200, from(scott->getInstanceId()), WaitForCommand, david->ack())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrack18xRetransmissions() 
      {
         InfoLog(<< "testUASPrack18xRetransmissions (takes ~128 seconds to run)");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         int retransInterval = resip::Timer::T1;

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->noAction()),  // first 18x
             david->expect(INVITE/183, from(scott->getInstanceId()), retransInterval+WaitForCommand, david->noAction()),  // first retrans - resip::Timer::T1 later (allow 1000ms time variance)
             david->expect(INVITE/183, from(scott->getInstanceId()), (retransInterval*2)+WaitForCommand, david->noAction()),  // next retrans - resip::Timer::T1*2 ms later (allow 1000ms time variance)
             david->expect(INVITE/183, from(scott->getInstanceId()), (retransInterval*2*2)+WaitForCommand, david->noAction()),  // next retrans
             david->expect(INVITE/183, from(scott->getInstanceId()), (retransInterval*2*2*2)+WaitForCommand, david->noAction()),  // next retrans
             david->expect(INVITE/183, from(scott->getInstanceId()), (retransInterval*2*2*2*2)+WaitForCommand, david->noAction()),  // next retrans
             scott->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, (retransInterval*2*2*2*2*2)+WaitForCommand, uas.noAction()),
             david->expect(INVITE/504, alwaysMatches(), WaitForCommand, david->ack()),  // 504 error - no PRACK
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrackStrayPrack() 
      {
         InfoLog(<< "testUASPrackStrayPrack");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, true))),
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Prack, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             david->expect(PRACK/200, from(scott->getInstanceId()), WaitForCommand, david->prack()),  // Stray prack (will be a copy of last one)
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/481, alwaysMatches(), WaitForCommand, uas.accept()),
             And(Sub(david->expect(INVITE/200, from(scott->getInstanceId()), WaitForCommand, david->ack())),
                 Sub(scott->expect(Invite_Connected, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()))),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrack2ndOfferInNoAnswerReliableWaitingPrack() 
      {
         InfoLog(<< "testUASPrack2ndOfferInNoAnswerReliableWaitingPrack");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.provisional(183, false)),  // provisional no answer
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack(offer)),  // 2nd offer without an answer - session should be torn down
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             david->expect(PRACK/406, alwaysMatches(), WaitForCommand, david->noAction()),
             david->expect(INVITE/406, alwaysMatches(), WaitForCommand, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }

      void testUASPrack2ndOfferInOfferReliableProvidedAnswer() 
      {
         InfoLog(<< "testUASPrack2ndOfferInOfferReliableProvidedAnswer");

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Supported);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Supported);

         TestClientRegistration regScott(scott);

         boost::shared_ptr<SdpContents> offer(static_cast<SdpContents*>(standardOffer->clone()));
         boost::shared_ptr<SdpContents> answer(static_cast<SdpContents*>(standardAnswer->clone()));

         Seq(scott->registerUa(),
             scott->expect(Register_Success, regScott, dumFrom(proxy), WaitForRegistration, scott->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         TestServerInviteSession uas(scott);
         
         Seq(david->invite(scott->getAor().uri(), offer, TestSipEndPoint::RelProvModeSupported),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(INVITE/407, from(proxy), WaitForResponse, chain(david->ack(), david->digestRespond())),
             optional(david->expect(INVITE/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_NewServerSession, uas, dumFrom(proxy), WaitForCommand, uas.noAction()),
             scott->expect(Invite_Offer, *TestEndPoint::AlwaysTruePred, WaitForCommand, chain(uas.provideAnswer(*standardAnswer), uas.provisional(183, false))),  // provisional no answer
             david->expect(INVITE/183, from(scott->getInstanceId()), WaitForCommand, david->prack(offer)),  // 2nd offer without an answer - session should be torn down
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             david->expect(PRACK/407, from(proxy), WaitForCommand, david->digestRespond()),
             optional(david->expect(PRACK/100, from(proxy), WaitFor100, david->noAction())),
             scott->expect(Invite_Terminated, *TestEndPoint::AlwaysTruePred, WaitForCommand, uas.noAction()),
             david->expect(PRACK/406, alwaysMatches(), WaitForCommand, david->noAction()),
             david->expect(INVITE/406, alwaysMatches(), WaitForCommand, david->noAction()),
             WaitForEndOfSeq);
         ExecuteSequences();

         scott->getProfile()->setUacReliableProvisionalMode(MasterProfile::Never);
         scott->getProfile()->setUasReliableProvisionalMode(MasterProfile::Never);
      }
};

// Registers the fixture into the 'registry'
CPPUNIT_TEST_SUITE_REGISTRATION( DumTestCase );

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

      Log::setLevel(Log::Crit, Subsystem::CONTENTS);
      Log::setLevel(Log::Crit, Subsystem::DNS);
      Log::setLevel(Log::Crit, Subsystem::SDP);
      Log::setLevel(Log::Crit, Subsystem::SIP);
      Log::setLevel(Log::Crit, Subsystem::TRANSACTION);
      Log::setLevel(Log::Crit, Subsystem::TRANSPORT);
      Log::setLevel(Log::Crit, Subsystem::STATS);
      Log::setLevel(Log::Crit, Subsystem::REPRO);
      Log::setLevel(Log::Debug, Subsystem::TEST);

      resip::Timer::T100 = 0;
      resip::Timer::T1 = 2000;
      resip::Timer::T2 = 8000;

      if(args.mInteractive)
      {
         interactive = true;
         CommandLineSelector testSelector;
         CPPUNIT_NS::TextTestRunner testrunner; 

         // informs test-listener about testresults
         CPPUNIT_NS::TestResult testresult;
         // register listener for collecting the test-results
         CPPUNIT_NS::TestResultCollector collectedresults;
         testresult.addListener (&collectedresults);
         // Add a listener that displays test progres
         CPTextTestProgressListener progress;
         testresult.addListener( &progress );   

         // Get the top level suite from the registry
         CppUnit::Test *suite = CppUnit::TestFactoryRegistry::getRegistry().makeTest();

         int numRepetitions = 1;
         if(CppTestSelector::SelectTests(suite,testrunner, testSelector,numRepetitions) > 0)
         {
            DumFixture::initialize(argc, argv);

            for(int x=0; x<numRepetitions; x++)
            {
               testrunner.run (testresult);
            }

            DumFixture::destroyStatic();

            // output results in text-format
            //TextOutputter (TestResultCollector *result, OStream &stream)
            CPPUNIT_NS :: TextOutputter textoutputter (&collectedresults, std::cerr);
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
         DumFixture::initialize(argc, argv);
      
         CppUnit::TextTestRunner runner;

         runner.addTest( CppUnit::TestFactoryRegistry::getRegistry().makeTest() );
         runner.run();
         DebugLog(<< "Finished: waiting for all transactions to die.");
      
         sleepSeconds(32);

         DumFixture::destroyStatic();
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
