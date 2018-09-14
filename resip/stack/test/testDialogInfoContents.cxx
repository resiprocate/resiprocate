#include "resip/stack/DialogInfoContents.hxx"
#include "resip/stack/SdpContents.hxx"
#include <iostream>
#include "rutil/Logger.hxx"
#include "TestSupport.hxx"

using namespace resip;
using namespace std;

#define CRLF "\r\n"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, Log::Debug, argv[0]);

   // Create simple Dialog Info test
   {
      Data sdpTxt("v=0\r\n"
         "o=- 333525334858460 333525334858460 IN IP4 192.168.0.156\r\n"
         "s=test123\r\n"
         "c=IN IP4 192.168.0.156\r\n"
         "t=4058038202 0\r\n"
         "m=audio 41466 RTP/AVP 0 101\r\n"
         "a=ptime:20\r\n"
         "a=rtpmap:0 PCMU/8000\r\n"
         "a=rtpmap:101 telephone-event/8000\r\n"
         "a=fmtp:101 0-11\r\n");

      HeaderFieldValue hfvSdp(sdpTxt.data(), sdpTxt.size());
      Mime typeSdp("application", "sdp");
      SdpContents sdp(hfvSdp, typeSdp);

      DialogInfoContents di;
      di.setEntity(Uri("sip:entity@domain"));
      di.setVersion(0);
      di.setDialogInfoState(DialogInfoContents::Full);
      
      DialogInfoContents::Dialog dialog;
      dialog.setId("zxcvbnm3");
      dialog.setCallId("a84b4c76e66710");
      dialog.setLocalTag("1928301774");
      dialog.setRemoteTag("8736347");
      dialog.setDirection(DialogInfoContents::Recipient);
      dialog.setState(DialogInfoContents::Proceeding);
      dialog.setDuration(60);
      dialog.setStateEvent(DialogInfoContents::Replaced);
      dialog.setStateCode(200);
      dialog.setReplacesInfo("lksjdflkasdjfl23432", "1234112", "sdfkjl22");
      NameAddr referredBy;
      referredBy.displayName() = "UserA's Name";
      referredBy.uri().host() = "domain.com";
      referredBy.uri().user() = "1000";
      dialog.setReferredBy(referredBy);
      dialog.addRouteToRouteSet(NameAddr("sip:route1.com"));
      dialog.addRouteToRouteSet(NameAddr("sip:route2.com"));

      NameAddr localIdentity;
      localIdentity.displayName() = "UserB";
      localIdentity.uri() = Uri("sip:me@here.com");
      dialog.localParticipant().setIdentity(localIdentity);
      dialog.localParticipant().setTarget(NameAddr("<sips:bob@biloxi.example.com>;+sip.rendering=\"no\";isfocus;class=call"));
      dialog.localParticipant().setSessionDescription(Data::from(sdp), "application/sdp");
      dialog.localParticipant().setCSeq(5);

      NameAddr remoteIdentity;
      remoteIdentity.displayName() = "UserC";
      remoteIdentity.uri() = Uri("sip:someone@somewhere.com");
      dialog.remoteParticipant().setIdentity(remoteIdentity);
      dialog.remoteParticipant().setTarget(Uri("sip:me@192.168.1.5"));
      dialog.remoteParticipant().addTargetParam("name", "value");
      sdp.session().name() = "sdpC";
      dialog.remoteParticipant().setSessionDescription(Data::from(sdp), "application/sdp");
      dialog.remoteParticipant().setCSeq(10);

      dialog.addDialogElement("Host", "UA1234");
      dialog.addDialogElement("MultiElement", "0");
      dialog.addDialogElement("MultiElement", "1");
      dialog.addDialogElement("MultiElement", "2");

      di.addDialog(dialog);

      DialogInfoContents::Dialog mindialog;
      mindialog.setId("mindialog-id");
      di.addDialog(mindialog);

      cout << di << endl;

      // Now create a Data blob and try and parse our own output
      Data dialogInfoData = Data::from(di);
      HeaderFieldValue hfv(dialogInfoData.data(), dialogInfoData.size());
      DialogInfoContents di2(hfv, DialogInfoContents::getStaticType());
      assert(di2.getVersion() == di.getVersion());
      assert(di2.getDialogInfoState() == di.getDialogInfoState());
      assert(di2.getEntity() == di.getEntity());
      if(di2.getDialogs().size() != 2)
      {
         resipCout << "di2 size: " << di2.getDialogs().size() << endl << endl;
         cout << di2 << endl;
      }
      assert(di2.getDialogs().size() == 2);

      assert(di2.getDialogs().front().getId() == di.getDialogs().front().getId());
      assert(di2.getDialogs().front().getCallId() == di.getDialogs().front().getCallId());
      assert(di2.getDialogs().front().getLocalTag() == di.getDialogs().front().getLocalTag());
      assert(di2.getDialogs().front().getRemoteTag() == di.getDialogs().front().getRemoteTag());
      assert(di2.getDialogs().front().getDirection() == di.getDialogs().front().getDirection());
      assert(di2.getDialogs().front().getState() == di.getDialogs().front().getState());
      assert(di2.getDialogs().front().getDuration() == di.getDialogs().front().getDuration());
      assert(di2.getDialogs().front().hasDuration() == di.getDialogs().front().hasDuration());
      assert(di2.getDialogs().front().getStateEvent() == di.getDialogs().front().getStateEvent());
      assert(di2.getDialogs().front().getStateCode() == di.getDialogs().front().getStateCode());
      assert(di2.getDialogs().front().getReplacesCallId() == di.getDialogs().front().getReplacesCallId());
      assert(di2.getDialogs().front().getReplacesLocalTag() == di.getDialogs().front().getReplacesLocalTag());
      assert(di2.getDialogs().front().getReplacesRemoteTag() == di.getDialogs().front().getReplacesRemoteTag());
      assert(di2.getDialogs().front().getReferredBy() == di.getDialogs().front().getReferredBy());
      assert(di2.getDialogs().front().getRouteSet().size() == di.getDialogs().front().getRouteSet().size());
      assert(di2.getDialogs().front().getRouteSet().front() == di.getDialogs().front().getRouteSet().front());
      assert(di2.getDialogs().front().getRouteSet().back() == di.getDialogs().front().getRouteSet().back());
      
      assert(di2.getDialogs().front().localParticipant().getIdentity() == di.getDialogs().front().localParticipant().getIdentity());
      assert(di2.getDialogs().front().localParticipant().getTarget() == di.getDialogs().front().localParticipant().getTarget());
      assert(di2.getDialogs().front().localParticipant().getTargetParams().size() == di.getDialogs().front().localParticipant().getTargetParams().size());      
      Data value, value2;
      assert(di2.getDialogs().front().localParticipant().getTargetParam("+sip.rendering", value));
      assert(di.getDialogs().front().localParticipant().getTargetParam("+sip.rendering", value2));
      assert(value == value2);
      assert(di2.getDialogs().front().localParticipant().getTargetParam("isfocus", value));
      assert(di.getDialogs().front().localParticipant().getTargetParam("isfocus", value2));
      assert(value == value2);
      assert(di2.getDialogs().front().localParticipant().getTargetParam("class", value));
      assert(di.getDialogs().front().localParticipant().getTargetParam("class", value2));
      assert(value == value2);      
      assert(di2.getDialogs().front().localParticipant().getSessionDescription() == di.getDialogs().front().localParticipant().getSessionDescription());
      assert(di2.getDialogs().front().localParticipant().getSessionDescriptionType() == di.getDialogs().front().localParticipant().getSessionDescriptionType());
      assert(di2.getDialogs().front().localParticipant().getCSeq() == di.getDialogs().front().localParticipant().getCSeq());
      assert(di2.getDialogs().front().localParticipant().hasCSeq() == di.getDialogs().front().localParticipant().hasCSeq());

      assert(di2.getDialogs().front().remoteParticipant().getIdentity() == di.getDialogs().front().remoteParticipant().getIdentity());
      assert(di2.getDialogs().front().remoteParticipant().getTarget() == di.getDialogs().front().remoteParticipant().getTarget());
      assert(di2.getDialogs().front().remoteParticipant().getTargetParams().size() == di.getDialogs().front().remoteParticipant().getTargetParams().size());
      assert(di2.getDialogs().front().remoteParticipant().getTargetParam("name", value));
      assert(di.getDialogs().front().remoteParticipant().getTargetParam("name", value2));
      assert(value == value2);
      HeaderFieldValue hfvSdp2(di2.getDialogs().front().remoteParticipant().getSessionDescription().data(), di2.getDialogs().front().remoteParticipant().getSessionDescription().size());
      SdpContents sdp2(hfvSdp2, typeSdp);
      assert(sdp == sdp2);  // Check if newly parse sdp equals one originally set
      assert(di2.getDialogs().front().remoteParticipant().getSessionDescriptionType() == di.getDialogs().front().remoteParticipant().getSessionDescriptionType());
      assert(di2.getDialogs().front().remoteParticipant().getCSeq() == di.getDialogs().front().remoteParticipant().getCSeq());
      assert(di2.getDialogs().front().remoteParticipant().hasCSeq() == di.getDialogs().front().remoteParticipant().hasCSeq());

      Data hostValue;
      assert(di2.getDialogs().front().getDialogElement("Host", hostValue) == true);
      assert(hostValue == "UA1234");
      assert(di2.getDialogs().front().getDialogElement("Host", hostValue, 0) == true);
      assert(hostValue == "UA1234");
      assert(di2.getDialogs().front().getDialogElement("Host", hostValue, 1) == false);
      assert(hostValue.empty());
      assert(di.getDialogs().front().getDialogElement("Host", hostValue) == true);
      assert(hostValue == "UA1234");
      assert(di.getDialogs().front().getDialogElement("Host", hostValue, 0) == true);
      assert(hostValue == "UA1234");
      assert(di.getDialogs().front().getDialogElement("Host", hostValue, 1) == false);
      assert(hostValue.empty());

      Data multiValue;
      assert(di2.getDialogs().front().getDialogElement("MultiElement", multiValue, 0) == true);
      cout << "multi[0]: " << multiValue << endl;
      // Currently using multimap, not sure order can be assumed
      assert(multiValue == "0" || multiValue == "1" || multiValue == "2");
      assert(di2.getDialogs().front().getDialogElement("MultiElement", multiValue, 1) == true);
      cout << "multi[0]: " << multiValue << endl;
      assert(multiValue == "0" || multiValue == "1" || multiValue == "2");
      assert(di2.getDialogs().front().getDialogElement("MultiElement", multiValue, 2) == true);
      cout << "multi[0]: " << multiValue << endl;
      assert(multiValue == "0" || multiValue == "1" || multiValue == "2");
      assert(di2.getDialogs().front().getDialogElement("MultiElement", multiValue, 3) == false);
      assert(multiValue.empty());
      assert(di.getDialogs().front().getDialogElement("MultiElement", multiValue, 0) == true);
      cout << "multi[0]: " << multiValue << endl;
      assert(multiValue == "0" || multiValue == "1" || multiValue == "2");
      assert(di.getDialogs().front().getDialogElement("MultiElement", multiValue, 1) == true);
      cout << "multi[0]: " << multiValue << endl;
      assert(multiValue == "0" || multiValue == "1" || multiValue == "2");
      assert(di.getDialogs().front().getDialogElement("MultiElement", multiValue, 2) == true);
      cout << "multi[0]: " << multiValue << endl;
      assert(multiValue == "0" || multiValue == "1" || multiValue == "2");
      assert(di.getDialogs().front().getDialogElement("MultiElement", multiValue, 3) == false);
      assert(multiValue.empty());

      assert(di2.getDialogs().back().getId() == di.getDialogs().back().getId());
      assert(di2.getDialogs().back().getState() == di.getDialogs().back().getState());

      // Test assingment and copy construction
      DialogInfoContents copy(di);
      assert(copy.getVersion() == di.getVersion());
      assert(copy.getDialogInfoState() == di.getDialogInfoState());
      assert(copy.getEntity() == di.getEntity());
      assert(copy.getDialogs().size() == 2);
      DialogInfoContents assign = di;
      assert(assign.getVersion() == di.getVersion());
      assert(assign.getDialogInfoState() == di.getDialogInfoState());
      assert(assign.getEntity() == di.getEntity());
      assert(assign.getDialogs().size() == 2);
   }

   // Test SipMessage Parse with RFC4235 Basic Example Body
   {
      Data txt(
         "NOTIFY sip:bob@biloxi.com SIP/2.0" CRLF
         "Via: SIP/2.0/UDP pc33.atlanta.com;branch=z9hG4bKnashds8" CRLF
         "To: Bob <sip:bob@biloxi.com>" CRLF
         "From: Alice <sip:alice@atlanta.com>;tag=1928301774" CRLF
         "Call-ID: a84b4c76e66710" CRLF
         "CSeq: 314159 NOTIFY" CRLF
         "Max-Forwards: 70" CRLF
         "Contact: <sip:alice@pc33.atlanta.com>" CRLF
         "Content-Type: application/dialog-info+xml" CRLF
         "Content-Length: 8000" CRLF
         CRLF
         "<?xml version=\"1.0\"?>" CRLF
         "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\"" CRLF
         "             version=\"0\"" CRLF
         "             state=\"full\"" CRLF
         "             entity=\"sip:alice@example.com\">" CRLF
         "  <dialog id=\"as7d900as8\" call-id=\"a84b4c76e66710\"" CRLF
         "          local-tag=\"1928301774\" direction=\"initiator\">" CRLF
         "    <state>trying</state>" CRLF
         "  </dialog>" CRLF
         "</dialog-info>" CRLF
         );
      try
      {
         auto_ptr<SipMessage> msg(TestSupport::makeMessage(txt.c_str()));
         Contents* body = msg->getContents();

         assert(body != 0);
         DialogInfoContents* dialogInfo = dynamic_cast<DialogInfoContents*>(body);
         if (dialogInfo)
         {
            assert(dialogInfo->getEntity() == Uri("sip:alice@example.com"));
            assert(dialogInfo->getVersion() == 0);
            assert(dialogInfo->getDialogInfoState() == DialogInfoContents::Full);
            assert(dialogInfo->getDialogs().size() == 1);
            assert(dialogInfo->getDialogs().front().getId() ==  "as7d900as8");
            assert(dialogInfo->getDialogs().front().getCallId() == "a84b4c76e66710");
            assert(dialogInfo->getDialogs().front().getLocalTag() == "1928301774");
            assert(dialogInfo->getDialogs().front().getDirection() == DialogInfoContents::Initiator);
            assert(dialogInfo->getDialogs().front().getState() == DialogInfoContents::Trying);

            // Modify the contents so that it will call our encode method for this following cout call
            dialogInfo->setVersion(1);
            cout << *dialogInfo << endl;
         }
         else
         {
            assert(false);
         }
      }
      catch (BaseException& e)
      {
         cerr << e << endl;
         assert(false);
      }
   }

   // Test a bad entity Uri (missing sip:) - expecting a throw
   {
      Data txt(
         "<?xml version=\"1.0\"?>" CRLF
         "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\"" CRLF
         "             version=\"0\"" CRLF
         "             state=\"full\"" CRLF
         "             entity=\"alice@example.com\">" CRLF
         "  <dialog id=\"as7d900as8\" call-id=\"a84b4c76e66710\"" CRLF
         "          local-tag=\"1928301774\" direction=\"initiator\">" CRLF
         "    <state>trying</state>" CRLF
         "  </dialog>" CRLF
         "</dialog-info>" CRLF
         );

      try
      {
         HeaderFieldValue hfv(txt.data(), txt.size());
         DialogInfoContents dialogInfo(hfv, DialogInfoContents::getStaticType());
         cout << "NOTE:  exceptions appearing in output are expected!!" << endl;
         dialogInfo.checkParsed(); // trigger a parse
         assert(false);  // shouldn't get here - should throw
      }
      catch (BaseException& e)
      {
         cout << "Expected exception received: " << e << endl;
      }
   }

   // RFC4235 - example body from section 6.2 - with fixes from RFC Errata: 771, 781, 788
   {
      Data txt(
         "<?xml version=\"1.0\"?>" CRLF
         "<dialog-info xmlns=\"urn:ietf:params:xml:ns:dialog-info\"" CRLF
         "             version=\"5\" state=\"partial\"" CRLF
         "             entity=\"sip:alice@example.com\">" CRLF
         "  <dialog id=\"zxcvbnm3\" call-id=\"a84b4c76e66710\"" CRLF
         "          local-tag=\"1928301774\"" CRLF
         "          remote-tag=\"8736347\" direction=\"initiator\">" CRLF
         "    <state event=\"replaced\">terminated</state>" CRLF
         "  </dialog>" CRLF
         "  <dialog id=\"sfhjsjk12\" call-id=\"o34oii1\"" CRLF
         "          local-tag=\"8903j4\"" CRLF
         "          remote-tag=\"78cjkus\" direction=\"recipient\">" CRLF
         "    <state event=\"replaced\">confirmed</state>" CRLF
         "    <replaces call-id=\"a84b4c76e66710\"" CRLF
         "          local-tag=\"1928301774\"" CRLF
         "          remote-tag=\"8736347\"/>" CRLF
         "    <referred-by>" CRLF
         "      sip:bob-is-not-here@vm.example.net" CRLF
         "    </referred-by>" CRLF
         "    <local>" CRLF
         "      <target uri=\"sip:alice@pc33.example.com\">" CRLF
         "        <param pname=\"+sip.rendering\" pval=\"yes\"/>" CRLF
         "      </target>" CRLF
         "    </local>" CRLF
         "    <remote>" CRLF
         "      <identity display=\"Cathy Jones\">" CRLF
         "         sip:cjones@example.net" CRLF
         "      </identity>" CRLF
         "      <target uri=\"sip:line3@host3.example.net\">" CRLF
         "        <param pname=\"actor\" pval=\"attendant\"/>" CRLF
         "        <param pname=\"automaton\" pval=\"false\"/>" CRLF
         "      </target>" CRLF
         "    </remote>" CRLF
         "  </dialog>" CRLF
         "</dialog-info>" CRLF
         );

      try
      {
         HeaderFieldValue hfv(txt.data(), txt.size());
         DialogInfoContents dialogInfo(hfv, DialogInfoContents::getStaticType());
         
         assert(dialogInfo.getEntity() == Uri("sip:alice@example.com"));
         assert(dialogInfo.getVersion() == 5);
         assert(dialogInfo.getDialogInfoState() == DialogInfoContents::Partial);
         assert(dialogInfo.getDialogs().size() == 2);
         assert(dialogInfo.getDialogs().front().getId() == "zxcvbnm3");
         assert(dialogInfo.getDialogs().front().getCallId() == "a84b4c76e66710");
         assert(dialogInfo.getDialogs().front().getLocalTag() == "1928301774");
         assert(dialogInfo.getDialogs().front().getRemoteTag() == "8736347");
         assert(dialogInfo.getDialogs().front().getDirection() == DialogInfoContents::Initiator);
         assert(dialogInfo.getDialogs().front().getState() == DialogInfoContents::Terminated);
         assert(dialogInfo.getDialogs().front().getStateEvent() == DialogInfoContents::Replaced);
         assert(dialogInfo.getDialogs().back().getId() == "sfhjsjk12");
         assert(dialogInfo.getDialogs().back().getCallId() == "o34oii1");
         assert(dialogInfo.getDialogs().back().getLocalTag() == "8903j4");
         assert(dialogInfo.getDialogs().back().getRemoteTag() == "78cjkus");
         assert(dialogInfo.getDialogs().back().getDirection() == DialogInfoContents::Recipient);
         assert(dialogInfo.getDialogs().back().getState() == DialogInfoContents::Confirmed);
         assert(dialogInfo.getDialogs().back().getStateEvent() == DialogInfoContents::Replaced);
         assert(dialogInfo.getDialogs().back().getReplacesCallId() == "a84b4c76e66710");
         assert(dialogInfo.getDialogs().back().getReplacesLocalTag() == "1928301774");
         assert(dialogInfo.getDialogs().back().getReplacesRemoteTag() == "8736347");
         assert(dialogInfo.getDialogs().back().getReferredBy() == NameAddr("sip:bob-is-not-here@vm.example.net"));
         assert(dialogInfo.getDialogs().back().localParticipant().getTarget() == Uri("sip:alice@pc33.example.com"));
         Data value;
         assert(dialogInfo.getDialogs().back().localParticipant().getTargetParam("+sip.rendering", value));
         assert(value == "yes");
         assert(dialogInfo.getDialogs().back().remoteParticipant().getIdentity() == NameAddr("\"Cathy Jones\" <sip:cjones@example.net>"));
         assert(dialogInfo.getDialogs().back().remoteParticipant().getTarget() == Uri("sip:line3@host3.example.net"));
         assert(dialogInfo.getDialogs().back().remoteParticipant().getTargetParam("actor", value));
         assert(value == "attendant");
         assert(dialogInfo.getDialogs().back().remoteParticipant().getTargetParam("automaton", value));
         assert(value == "false");
      }
      catch (BaseException& e)
      {
         cout << "Unexpected exception received: " << e << endl;
         assert(false);
      }
   }
   
   cerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
*
* Copyright (c) 2016 SIP Spectrum, Inc.  All rights reserved.
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
* 3. Neither the name of the author(s) nor the names of any contributors
*    may be used to endorse or promote products derived from this software
*    without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
* OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
* HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
* LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
* OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
* SUCH DAMAGE.
*
* ====================================================================
*
*/
/*
* vi: set shiftwidth=3 expandtab:
*/
