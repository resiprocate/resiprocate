#include "B2BSession.hxx"
#include "Server.hxx"
#include "IChatIPPortData.hxx"
#include "AppSubsystem.hxx"

#include <resip/stack/PlainContents.hxx>
#include <resip/stack/SdpContents.hxx>
#include <resip/dum/ServerInviteSession.hxx>
#include <resip/dum/ClientInviteSession.hxx>
#include <resip/dum/ClientSubscription.hxx>
#include <rutil/DnsUtil.hxx>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/WinLeakCheck.hxx>

using namespace gateway;
using namespace resip;
using namespace std;

#define RESIPROCATE_SUBSYSTEM AppSubsystem::GATEWAY

#ifdef BRIEF_MSG_LOGGING
#define LOG_MSG msg.brief()
#define LOG_MSG_WITH_SDP msg.brief() << ", sdp=" << sdp
#define LOG_MSGP msg->brief()
#else
#define LOG_MSG endl << msg
#define LOG_MSG_WITH_SDP endl << msg
#define LOG_MSGP endl << *msg
#endif
#define B2BLOG_PREFIX << "B2BSession[" << mHandle << "] "

namespace gateway 
{

B2BSession::B2BSession(Server& server, bool hasDialogSet) : 
   AppDialogSet(server.getDialogUsageManager()), 
   mServer(server), 
   mHasDialogSet(hasDialogSet),
   mDum(server.getDialogUsageManager()),
   mPeer(0),
   mUACConnectedDialogId(Data::Empty, Data::Empty, Data::Empty),
   mWaitingOfferFromPeer(false),
   mWaitingAnswerFromPeer(false),
   mWaitingNitAnswerFromPeer(false),
   mAnchorMedia(false),
   mMediaRelayPort(0),
   mIChatEndpoint(false),
   mIChatWaitingToAccept(false),
   mIChatWaitingToProceed(false),
   mIChatWaitingToContinue(false),
   mIChatSdp(0)
{
   mHandle = mServer.registerB2BSession(this);
}

B2BSession::~B2BSession()
{
   endPeer();
   if(mMediaRelayPort != 0)
   {
      mServer.mMediaRelay->destroyRelay(mMediaRelayPort);
   }
   if(mIChatSdp) delete mIChatSdp;
   mServer.unregisterB2BSession(mHandle);
}

void 
B2BSession::end()
{
   if(mIChatWaitingToAccept)
   {
      rejectIChatCall();
   }

   if (mHasDialogSet)
   {
      AppDialogSet::end();
   }
   else
   {
      delete this;
   }
}

void 
B2BSession::setPeer(B2BSession* peer)
{
   mPeer = peer;
}

B2BSession* 
B2BSession::getPeer()
{
   return mPeer;
}

void 
B2BSession::stealPeer(B2BSession* victimSession)
{
   // Assume Peer mapping of victim - and copy some settings
   setPeer(victimSession->getPeer());
   if(mPeer)
   {
      mPeer->setPeer(this);
   }
   mMediaRelayPort = victimSession->mMediaRelayPort;
   victimSession->mMediaRelayPort = 0;  // clear out so that session will not dealloc media port
   mAnchorMedia = victimSession->mAnchorMedia;

   // Clear peer mapping in victim session
   victimSession->setPeer(0);
}

class IChatCallTimeout : public resip::DumCommand
{
   public:
      IChatCallTimeout(const Server& server, const B2BSessionHandle& handle) :
         mServer(server), mB2BSessionHandle(handle) {}
      IChatCallTimeout(const IChatCallTimeout& rhs) :
         mServer(rhs.mServer), mB2BSessionHandle(rhs.mB2BSessionHandle) {}
      ~IChatCallTimeout() {}

      void executeCommand() 
      {
         B2BSession* session = mServer.getB2BSession(mB2BSessionHandle);
         if(session)
         {
            session->timeoutIChatCall(); 
         }
      }

      resip::Message* clone() const { return new IChatCallTimeout(*this); }
      EncodeStream& encode(EncodeStream& strm) const { strm << "IChatCallTimeout: handle=" << mB2BSessionHandle; return strm; }
      EncodeStream& encodeBrief(EncodeStream& strm) const { return encode(strm); }
      
   private:
      const Server& mServer;
      B2BSessionHandle mB2BSessionHandle;
};

void 
B2BSession::initiateIChatCallRequest(const std::string& to, const std::string& from)
{
   // Notify jabber connector we are proceeding and what our session handle is
   mIChatCallToJID = to;
   mIChatCallFromJID = from;
   mIChatEndpoint = true;
   mIChatWaitingToAccept = true;
   proceedingIChatCall();

   bool result=false;
   try
   {
      Uri toUri(Data("xmpp:") + to.c_str());
      try
      {
         NameAddr fromNameAddr(Data("sip:") + from.c_str());
         fromNameAddr.displayName() = fromNameAddr.uri().user();
         result = createNewPeer(toUri, fromNameAddr, 0);
      }
      catch(resip::BaseException& e)
      {
         ErrLog(B2BLOG_PREFIX << "Error creating NameAddr from Jabber from header=" << from << ", error=" << e);
      }
   }
   catch(resip::BaseException& e)
   {
      ErrLog(B2BLOG_PREFIX << "Error extracting Uri from Jabber To header=" << to << ", error=" << e);
   }

   if(!result)
   {
      rejectIChatCall();
      end();
   }
}

void 
B2BSession::startIChatCall(const Uri& destinationUri, const NameAddr& from, const SdpContents *sdp)
{
   resip_assert(destinationUri.scheme() == "xmpp");

   mIChatEndpoint = true;
   mIChatDestination = destinationUri;
   mIChatFrom = from;
   if(sdp)
   {
      mIChatSdp = new SdpContents(*sdp);
   }
   
   mIChatCallToJID = Data::from(destinationUri).substr(5).c_str();  // Get all of address, except xmpp: part
   Uri fromUri;
   // Note:  for some reason iChat will send a 603 Decline if the username is more than 10 characters - so we truncate
   fromUri.user() = from.uri().user().substr(0, from.uri().user().size() > 10 ? 10 : from.uri().user().size());  
   fromUri.host() = mServer.mJabberComponentName;
   mIChatCallFromJID = fromUri.getAor().c_str();

   initiateIChatCall();

   // Start a timer
   IChatCallTimeout t(mServer, mHandle);
   mServer.post(t, mServer.mIChatProceedingTimeout);  
   mIChatWaitingToProceed = true;
}

void 
B2BSession::startSIPCall(const Uri& destinationUri, const NameAddr& from, const SdpContents *sdp)
{
   // Create a UserProfile for new call
   SharedPtr<UserProfile> userProfile(new UserProfile(mServer.getMasterProfile()));

   // Set the From address
   userProfile->setDefaultFrom(from);  
   userProfile->getDefaultFrom().remove(p_tag);  // Remove tag (if it exists)

   // Create the invite message
   SharedPtr<SipMessage> invitemsg = mDum.makeInviteSession(
      NameAddr(destinationUri), 
      userProfile,
      sdp, 
      this);

   // Send the invite message
   mDum.send(invitemsg);
}

bool 
B2BSession::checkIChatCallMatch(const resip::SipMessage& msg)
{
   if(!mHasDialogSet && !mIChatWaitingToAccept) 
   {
      // Check if URI matches
      // If we are an iChat endpoint then real To uri is actually in the display name of the To header 
      InfoLog(B2BLOG_PREFIX << "B2BSession::checkIChatCallMatch: checking to: " << msg.header(h_To).displayName() << " = " << mIChatCallToJID);
      InfoLog(B2BLOG_PREFIX << "B2BSession::checkIChatCallMatch: checking from: " << msg.header(h_From).displayName() << " = " << mIChatCallFromJID);
      if(msg.header(h_To).displayName() == Data(mIChatCallToJID.c_str()) &&
         msg.header(h_From).displayName() == Data(mIChatCallFromJID.c_str()))
      {
         mHasDialogSet = true; // If we match, we are going to get a dialog set associated with us
         return true;
      }
   }
   return false;
}

bool 
B2BSession::createNewPeer(const Uri& destinationUri, const NameAddr& from, const SdpContents *sdp)
{
   const SdpContents *pSdp = sdp;

   resip_assert(!mPeer);

   mPeer = new B2BSession(mServer);

   Data destinationData;
   if(!mServer.translateAddress(Data::from(destinationUri), destinationData, true /* failIfNoRule */))
   {
      ErrLog(B2BLOG_PREFIX << "B2BSession::createNewPeer: No translation rule for " << destinationUri);
      delete mPeer;
      mPeer = 0;
      return false;
   }
   Uri destination;
   try
   {
      destination = Uri(destinationData);
   }
   catch(resip::BaseException& e)
   {
      ErrLog(B2BLOG_PREFIX << "B2BSession::createNewPeer: Translation of " << destinationUri << " resulted in invalid uri format: " << e);
      delete mPeer;
      mPeer = 0;
      return false;
   }

   // Check if media needs to be anchored 
   SdpContents localOffer;
   if((mServer.mAlwaysRelayIChatMedia && mIChatEndpoint) || destination.scheme() == "xmpp" || !sdp)
   {
      mAnchorMedia = true;
      mPeer->mAnchorMedia = true;

      // Check if sdp used to create new peer is locally generated - if so, don't allocate a new media relay, use the existing one
      if(sdp && sdp->session().origin().user() == SDP_ICHATGW_ORIGIN_USER)
      {
         if(sdp->session().media().size() >= 1 && sdp->session().media().front().port() != 0)
         {
            mMediaRelayPort = sdp->session().media().front().port();
            InfoLog(B2BLOG_PREFIX << "B2BSession::createNewPeer: Detected looped back session - sharing media relay (port=" << mMediaRelayPort << ") with originator.");
         }
      }

      // Replace passed in remote SDP with local SDP
      if(!buildLocalOffer(localOffer))
      {
         ErrLog(B2BLOG_PREFIX << "B2BSession::createNewPeer: Unable to build local offer.");
         delete mPeer;
         mPeer = 0;
         return false;
      }
      pSdp = &localOffer;
      mPeer->mMediaRelayPort = mMediaRelayPort;
   }

   // Create B2BCall
   InfoLog(B2BLOG_PREFIX << "B2BSession::createNewPeer: Routing new call to " << destination);

   // Map other leg to this one
   mPeer->setPeer(this);

   if(destination.scheme() == "xmpp")
   {
      mPeer->startIChatCall(destination, from, pSdp);
   }
   else
   {
      mPeer->startSIPCall(destination, from, pSdp);
   }

   return true;
}

void 
B2BSession::notifyIChatCallCancelled()
{
   InfoLog(B2BLOG_PREFIX << "notifyIChatCallCancelled");
   mIChatWaitingToAccept = false;
   end();
}

void 
B2BSession::notifyIChatCallProceeding(const std::string& to)
{
   InfoLog(B2BLOG_PREFIX << "notifyIChatCallProceeding: to=" << to);
   mIChatWaitingToProceed = false;
   mIChatWaitingToContinue = true;
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         // Sending a provisional back to the other end should cause ringback to be played
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            sis->provisional(180);
         }
      }
   }
}

void 
B2BSession::notifyIChatCallFailed(unsigned int statusCode)
{
   InfoLog(B2BLOG_PREFIX << "notifyIChatCallFailed: Failed IChat call, statusCode=" << statusCode << " - tearing down other leg.");
   endPeer();
   delete this;
}

void 
B2BSession::continueIChatCall(const std::string& remoteIPPortListBlob)
{
   resip_assert(!mIChatWaitingToProceed);
   resip_assert(mIChatWaitingToContinue);

   mIChatWaitingToContinue = false;

   // Create a UserProfile for new call
   SharedPtr<UserProfile> userProfile(new UserProfile(mServer.getMasterProfile()));

   // Doesn't really matter what's in the SIP message from header that goes to iChat - it's never displayed to the client
   userProfile->setDefaultFrom(mServer.getMasterProfile()->getDefaultFrom()); 

   Tuple destinationIPPort;
   IChatIPPortData remoteIPPortList(remoteIPPortListBlob);

   // Take first ipv4 or ipv6 entry - for ipv4 it appears to be a STUN mapped address
   bool skipFirst = mServer.mSkipFirstIChatAddress;  // Note (for testing): skipping the first IPV4 NAT mapped address (on systems with one interface) causes the local address to be used
   bool findTilda = false;  // Note (for testing): Not too sure what the ~ addresses are - they could be media relay servers - you cannot just send RTP to them, so there must be some additional protocol to allocate a relay
   bool v4found = false;
   IChatIPPortData::IPPortDataList::const_iterator it = remoteIPPortList.getIPPortDataList().begin();
   for(; it != remoteIPPortList.getIPPortDataList().end(); it++)
   {
      if(!v4found && it->second.ipVersion() == V4)
      {
         if(skipFirst)
         {
            skipFirst = false;            
         }
         else
         {
            if(!findTilda || it->first.find("~") != Data::npos)
            {
               // return first matching address
               destinationIPPort = it->second;
               if(!mServer.mPreferIPv6) break;
               v4found = true;
            }
         }
      }
      if(mServer.mPreferIPv6 && it->second.ipVersion() == V6)
      {
         if(skipFirst)
         {
            skipFirst = false;            
         }
         else
         {
            if(!findTilda || it->first.find("~") != Data::npos)
            {
               // return first matching address
               destinationIPPort = it->second;
               break;
            }
         }
      }
   }

   if(destinationIPPort.getPort() == 0)
   {
      // We didn't find an applicable address
      InfoLog(B2BLOG_PREFIX << "B2BSession::continueIChatCall: no appropriate address found in remoteIPPortList.");
      notifyIChatCallFailed(603);  // Decline
      return;
   }

   Data uriData;
   if(destinationIPPort.ipVersion() == V6)
   {
      uriData = "sip:user@[" + destinationIPPort.presentationFormat() + "]:" + Data(destinationIPPort.getPort());
   }
   else
   {
      uriData = "sip:user@" + destinationIPPort.presentationFormat() + ":" + Data(destinationIPPort.getPort());
   }

   try
   {
      NameAddr destination(uriData); 
      userProfile->setOutboundProxy(destination.uri());
      userProfile->setForceOutboundProxyOnAllRequestsEnabled(true);

      InfoLog(B2BLOG_PREFIX << "B2BSession::continueIChatCall: to=" << destination << " tuple=" << destinationIPPort);

      // Create the invite message
      SharedPtr<SipMessage> invitemsg = mDum.makeInviteSession(
         destination, 
         userProfile,
         mIChatSdp, 
         this);

      // Send the invite message
      mDum.send(invitemsg);

      // Prime the RTP engine of iChat if using IPv6
      // Note: under IPv6 iChat will not send an initial RTP packet until it receives one
      if(destinationIPPort.ipVersion() == V6 && mMediaRelayPort > 0)
      {
         mServer.mMediaRelay->primeNextEndpoint(mMediaRelayPort, destinationIPPort);
      }
   }
   catch(resip::BaseException& e)
   {
      ErrLog(B2BLOG_PREFIX << "continueIChatCall: Invalid NameAddr format=" << uriData << ": " << e);
      notifyIChatCallFailed(500);
      return;
   }
}

void 
B2BSession::timeoutIChatCall()
{
   if(mIChatWaitingToProceed)
   {
      InfoLog(B2BLOG_PREFIX << "timeoutIChatCall: could not find an appropriate iChat endpoint.");
      cancelIChatCall();

      notifyIChatCallFailed(408);
      return;
   }
}

void 
B2BSession::initiateIChatCall()
{
   IPCMsg msg;
   msg.addArg("initiateIChatCall");
   msg.addArg(mIChatCallToJID.c_str());
   msg.addArg(mIChatCallFromJID.c_str());
   msg.addArg(mHandle);
   mServer.mIPCThread->sendIPCMsg(msg);
}

void 
B2BSession::cancelIChatCall()
{
   IPCMsg msg;
   msg.addArg("cancelIChatCall");
   msg.addArg(mIChatCallToJID.c_str());
   msg.addArg(mIChatCallFromJID.c_str());
   mServer.mIPCThread->sendIPCMsg(msg);
}

void 
B2BSession::proceedingIChatCall()
{
   IPCMsg msg;
   msg.addArg("proceedingIChatCall");
   msg.addArg(mIChatCallToJID.c_str());
   msg.addArg(mIChatCallFromJID.c_str());
   msg.addArg(mHandle);
   mServer.mIPCThread->sendIPCMsg(msg);
}

void 
B2BSession::acceptIChatCall()
{
   IPCMsg msg;
   msg.addArg("acceptIChatCall");
   msg.addArg(mIChatCallToJID.c_str());
   msg.addArg(mIChatCallFromJID.c_str());
   mServer.mIPCThread->sendIPCMsg(msg);

   mIChatWaitingToAccept = false;
}

void 
B2BSession::rejectIChatCall()
{
   IPCMsg msg;
   msg.addArg("rejectIChatCall");
   msg.addArg(mIChatCallToJID.c_str());
   msg.addArg(mIChatCallFromJID.c_str());
   mServer.mIPCThread->sendIPCMsg(msg);

   mIChatWaitingToAccept = false;
}

bool 
B2BSession::buildLocalOffer(SdpContents& offer)
{
   // Build s=, o=, t=, and c= lines
   UInt64 currentTime = Timer::getTimeMicroSec();

   unsigned int port=0;
   if(mAnchorMedia)
   {
      if(mMediaRelayPort == 0)
      {
         // If we don't have an allocated port yet, then create one
         if(!mServer.mMediaRelay->createRelay(mMediaRelayPort))
         {
            ErrLog(B2BLOG_PREFIX << "Failed to allocate relay port!");
            return false;
         }
      }
      port = mMediaRelayPort;
   }

   // Note:  The outbound decorator will take care of filling in the correct IP address before the message is sent 
   //        to the wire.
   SdpContents::Session::Origin origin(SDP_ICHATGW_ORIGIN_USER, currentTime /* sessionId */, currentTime /* version */, SdpContents::IP4, "0.0.0.0");   // o=   
   SdpContents::Session session(0, origin, "-" /* s= */);
   session.connection() = SdpContents::Session::Connection(SdpContents::IP4, "0.0.0.0");  // c=
   session.addTime(SdpContents::Session::Time(0, 0));

   // Build Codecs and media offering
   SdpContents::Session::Medium medium("audio", port, 1, "RTP/AVP");
   SdpContents::Session::Codec g711ucodec("PCMU", 8000);
   g711ucodec.payloadType() = 0;  /* RFC3551 */ ;
   medium.addCodec(g711ucodec);

   if(mAnchorMedia)
   {
      medium.addAttribute("sendrecv");
   }
   else
   {
      medium.addAttribute("inactive");
   }
   session.addMedium(medium);

   offer.session() = session;
   return true;
}

bool 
B2BSession::buildLocalAnswer(SdpContents& answer)
{
   bool valid = false;

   if(mInviteSessionHandle.isValid() && mInviteSessionHandle->hasProposedRemoteSdp())
   {
      const SdpContents& offer = mInviteSessionHandle->getProposedRemoteSdp();

      try
      {
         // use our local offer as a starting place - this also allocates a relay port (if needed)
         if(!buildLocalOffer(answer))
         {
            return false;
         }

         // Set sessionid and version for this answer
         UInt64 currentTime = Timer::getTimeMicroSec();
         answer.session().origin().getSessionId() = currentTime;
         answer.session().origin().getVersion() = currentTime;  

         // Copy t= field from sdp (RFC3264)
         resip_assert(answer.session().getTimes().size() > 0);
         if(offer.session().getTimes().size() >= 1)
         {
            answer.session().getTimes().clear();
            answer.session().addTime(offer.session().getTimes().front());
         }

         // Clear out m= lines in answer then populate below
         answer.session().media().clear();

         // Loop through each offered m= line and provide a response
         std::list<SdpContents::Session::Medium>::const_iterator itMediaLine = offer.session().media().begin();
         for(; itMediaLine != offer.session().media().end(); itMediaLine++)
         {
            const SdpContents::Session::Medium& offerMediaLine = *itMediaLine;

            // We only process one media stream - so if we already have a valid - just reject the rest
            if(valid)
            {
               SdpContents::Session::Medium rejmedium(offerMediaLine.name(), 0, 1,  // Reject medium by specifying port 0 (RFC3264)	
                                                      offerMediaLine.protocol());
               answer.session().addMedium(rejmedium);
               continue;
            }

            // Answer Media Line
            // If this is a valid audio medium then process it

            if(isEqualNoCase(offerMediaLine.name(), "audio") &&
               isEqualNoCase(offerMediaLine.protocol(), "RTP/AVP") &&
               offerMediaLine.port() != 0)
            {
               SdpContents::Session::Medium medium("audio", mMediaRelayPort, 1, "RTP/AVP");
       
               // Iterate through codecs and look for supported codecs (only G711 for now) - tag found ones by storing their payload id
               std::list<Codec>::const_iterator itCodec = offerMediaLine.codecs().begin();
               for(; itCodec != offerMediaLine.codecs().end(); itCodec++)
               {
                  const Codec& codec = *itCodec;

                  if(isEqualNoCase(codec.getName(), "pcmu") && codec.getRate() == 8000)
                  {
                     SdpContents::Session::Codec answerCodec(codec);
                     answerCodec.payloadType() = codec.payloadType(); // honour offered payload id - just to be nice  :)
                     medium.addCodec(answerCodec);
                     // Consider offer valid if we see any matching codec 
                     valid = true;
                     break;
                  }
               }
         
               if(valid)
               {
                  //medium.addAttribute("ptime", 20);
      
                  // Check requested direction                  
                  if(offerMediaLine.exists("inactive"))  
                  {
                     medium.addAttribute("inactive");
                  }
                  else if(offerMediaLine.exists("sendonly"))
                  {
                     medium.addAttribute("recvonly");
                  }
                  else if(offerMediaLine.exists("recvonly"))
                  {
                     medium.addAttribute("sendonly");
                  }
                  else
                  {
                     // Note:  sendrecv is the default in SDP
                     medium.addAttribute("sendrecv");
                  }
                  answer.session().addMedium(medium);
               }
               else
               {
                  SdpContents::Session::Medium rejmedium(offerMediaLine.name(), 0, 1,  // Reject medium by specifying port 0 (RFC3264)	
                                                         offerMediaLine.protocol());
                  answer.session().addMedium(rejmedium);
               }
            }
            else
            {
               SdpContents::Session::Medium rejmedium(offerMediaLine.name(), 0, 1,  // Reject medium by specifying port 0 (RFC3264)	
                                                      offerMediaLine.protocol());
               answer.session().addMedium(rejmedium);
            }
         }  // end loop through m= offers
         if(!valid)
         {
            WarningLog( B2BLOG_PREFIX << "B2BSession::buildLocalAnswer - no matching codecs found in offer");
         }
      }
      catch(BaseException &e)
      {
         WarningLog( B2BLOG_PREFIX << "B2BSession::buildLocalAnswer - exception parsing SDP offer: " << e.getMessage());
         valid = false;
      }
      catch(...)
      {
         WarningLog(B2BLOG_PREFIX << "B2BSession::buildLocalAnswer - unknown exception parsing SDP offer");
         valid = false;
      }

      //InfoLog( << "B2BSession::buildLocalAnswer - SDPOffer: " << offer);
      //InfoLog( << "B2BSession::buildLocalAnswer - SDPAnswer: " << answer);
   }
   else
   {
      ErrLog(B2BLOG_PREFIX << "B2BSession::buildLocalAnswer - unable to build local answer, ishValid=" << mInviteSessionHandle.isValid() << ", hasProposedRemoteSdp=" << mInviteSessionHandle->hasProposedRemoteSdp());
   }
   return valid;
}

bool 
B2BSession::provideLocalAnswer()
{
   SdpContents answer;
   resip_assert(mInviteSessionHandle.isValid());
   bool answerOk = buildLocalAnswer(answer);

   if(answerOk)
   {
      mInviteSessionHandle->provideAnswer(answer);
   }
   else
   {
      mInviteSessionHandle->reject(488);
   }

   return answerOk;
}

void 
B2BSession::endPeer()
{
   if(mPeer)
   {
      mPeer->setPeer(0);
      if(mPeer->mIChatWaitingToContinue || mPeer->mIChatWaitingToProceed)
      {
         mPeer->cancelIChatCall();
         delete mPeer;
      }
      else
      {
         mPeer->end();  // send cancel or bye appropriately
      }
      mPeer = 0;
   }
}

bool 
B2BSession::isUACConnected()
{
   return !mUACConnectedDialogId.getCallId().empty();
}

bool 
B2BSession::isStaleFork(const DialogId& dialogId)
{
   return (!mUACConnectedDialogId.getCallId().empty() && dialogId != mUACConnectedDialogId);
}

void 
B2BSession::fixupSdp(const SdpContents& origSdp, SdpContents& fixedSdp)
{
   fixedSdp = origSdp;

   if(fixedSdp.session().media().size() >= 1)
   {
      fixedSdp.session().media().front().clearCodecs();
      const std::list<Codec>& origcodecs = origSdp.session().media().front().codecs();
      std::list<Codec>::const_iterator it = origcodecs.begin();
      for(; it != origcodecs.end(); it++)
      {
         if(mServer.mCodecIdFilterList.find(it->payloadType()) == mServer.mCodecIdFilterList.end())
         {
            fixedSdp.session().media().front().addCodec(*it);
         }
      }
   }
}

SharedPtr<UserProfile> 
B2BSession::selectUASUserProfile(const SipMessage& msg)
{
   // If this is an iChat endpoint to force all requests in the session to go to the IP address and port
   // that the initiatial invite was from
   if(msg.exists(h_UserAgent) && msg.header(h_UserAgent).value().prefix("Viceroy"))
   {
      // Create a UserProfile for new call
      SharedPtr<UserProfile> userProfile(new UserProfile(mServer.getMasterProfile()));

      // Doesn't really matter what's in the SIP message from header that goes to iChat - it's never displayed to the client
      userProfile->setDefaultFrom(mServer.getMasterProfile()->getDefaultFrom()); 

      // Force endpoint routing
      Data destinationData;
      if(msg.getSource().ipVersion() == V6)
      {
         destinationData = "sip:[" + msg.getSource().presentationFormat() + "]:" + Data(msg.getSource().getPort());
      }
      else
      {
         destinationData = "sip:" + msg.getSource().presentationFormat() + ":" + Data(msg.getSource().getPort());
      }
      Uri destination(destinationData);
      userProfile->setOutboundProxy(destination);
      userProfile->setForceOutboundProxyOnAllRequestsEnabled(true);
      return userProfile;
   }
   else
   {
      return mServer.getMasterProfile();
   }
}


////////////////////////////////////////////////////////////////////////////////
// InviteSessionHandler      ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////

void
B2BSession::onNewSession(ClientInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onNewSession(ClientInviteSessionHandle): msg=" << LOG_MSG);
   mInviteSessionHandle = h->getSessionHandle();
   if(msg.exists(h_UserAgent))
   {
      if(msg.header(h_UserAgent).value().prefix("Viceroy"))
      {
         mIChatEndpoint = true;
      }
   }
}

void
B2BSession::onNewSession(ServerInviteSessionHandle h, InviteSession::OfferAnswerType oat, const SipMessage& msg)
{
   mInviteSessionHandle = h->getSessionHandle();

   if(msg.exists(h_UserAgent))
   {
      if(msg.header(h_UserAgent).value().prefix("Viceroy"))
      {
         mIChatEndpoint = true;
      }
   }

   // First check if this INVITE is to replace an existing session
   if(msg.exists(h_Replaces))
   {
      pair<InviteSessionHandle, int> presult;
      presult = mDum.findInviteSession(msg.header(h_Replaces));
      if(!(presult.first == InviteSessionHandle::NotValid())) 
      {         
         B2BSession* sessionToReplace = dynamic_cast<B2BSession *>(presult.first->getAppDialogSet().get());
         InfoLog(B2BLOG_PREFIX << "onNewSession(ServerInviteSessionHandle): to replace handle=" << sessionToReplace->getB2BSessionHandle() << ", msg=" << LOG_MSG);

         // Assume Peer mapping of old call - and copy some settings
         stealPeer(sessionToReplace);

         // Session to replace was found - end old session
         sessionToReplace->end();
         return;
      }      
   }

   InfoLog(B2BLOG_PREFIX << "onNewSession(ServerInviteSessionHandle): msg=" << LOG_MSG);

   // Note: remaining inbound call handling is done in onOffer and onOfferRequired
}

void
B2BSession::onFailure(ClientInviteSessionHandle h, const SipMessage& msg)
{
   // Note:  Teardown of peer is handled in destructor
   InfoLog(B2BLOG_PREFIX << "onFailure: msg=" << LOG_MSG);
}
      
void
B2BSession::onEarlyMedia(ClientInviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(B2BLOG_PREFIX << "onEarlyMedia: msg=" << LOG_MSG_WITH_SDP);
}

void
B2BSession::onProvisional(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onProvisional: msg=" << LOG_MSG);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            sis->provisional(msg.header(h_StatusLine).responseCode());
         }
      }
   }
}

void
B2BSession::onConnected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onConnected: msg=" << LOG_MSG);
   if(!isUACConnected())
   {
      // It is possible in forking scenarios to get multiple 200 responses, if this is 
      // our first 200 response, then this is the leg we accept, store the connected DialogId
      mUACConnectedDialogId = h->getDialogId();
      // Note:  each forked leg will update mInviteSessionHandle (in onNewSession call) - need to set mInviteSessionHandle for final answering leg on 200
      mInviteSessionHandle = h->getSessionHandle();  
   }
   else
   {
      // We already have a connected leg - end this one with a BYE
      h->end();
   }
}

void
B2BSession::onConnected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onConnected: msg=" << LOG_MSG);
}

void
B2BSession::onStaleCallTimeout(ClientInviteSessionHandle h)
{
   InfoLog(B2BLOG_PREFIX << "onStaleCallTimeout:");
   endPeer();
}

unsigned int
B2BSession::mapRejectionCodeForIChat(unsigned int statusCode)
{
   // iChat appears to display the following error codes nicely
   // 486 - Busy
   // 487 - Cancelled
   // 603 - Decline
   // 606 - Security Error
   switch(statusCode)
   {
   case 486:
   case 600:
      return 486;
   case 487:
      return 487;
   default:
      return 603;
   }
}

void
B2BSession::onTerminated(InviteSessionHandle h, InviteSessionHandler::TerminatedReason reason, const SipMessage* msg)
{
   Data reasonData;
   switch(reason)
   {
   case InviteSessionHandler::RemoteBye:
      reasonData = "received a BYE from peer";
      break;
   case InviteSessionHandler::RemoteCancel:
      reasonData = "received a CANCEL from peer";
      break;
   case InviteSessionHandler::Rejected:
      reasonData = "received a rejection from peer";
      break;
   case InviteSessionHandler::LocalBye:
      reasonData = "ended locally via BYE";
      break;
   case InviteSessionHandler::LocalCancel:
      reasonData = "ended locally via CANCEL";
      break;
   case InviteSessionHandler::Replaced:
      reasonData = "ended due to being replaced";
      break;
   case InviteSessionHandler::Referred:
      reasonData = "ended due to being reffered";
      break;
   case InviteSessionHandler::Error:
      reasonData = "ended due to an error";
      break;
   case InviteSessionHandler::Timeout:
      reasonData = "ended due to a timeout";
      break;
   default:
      resip_assert(false);
      break;
   }

   if(msg)
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated: reason=" << reasonData << ", msg=" << LOG_MSGP);
   }
   else
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated: reason=" << reasonData);
   }

   unsigned int statusCode = 603;
   if(msg)
   {
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }
   }

   // If this is a referred call and the refer is still around - then switch back to referrer (ie. failed transfer recovery)
   if(mReferringAppDialogSet.isValid() && mPeer)
   {
      B2BSession* session = (B2BSession*)mReferringAppDialogSet.get();
      session->stealPeer(this);
   }

   if(isStaleFork(h->getDialogId())) return;

   // If we have a peer that hasn't been accepted yet - then pass back terminated code, by calling reject now
   if(mPeer && mPeer->mInviteSessionHandle.isValid())
   {
      ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
      if(sis && !sis->isAccepted())
      {
         if(mPeer->mIChatEndpoint)
         {
            statusCode = mapRejectionCodeForIChat(statusCode);
         }
         sis->reject(statusCode);
      }
   }

   endPeer();
}

void
B2BSession::onRedirected(ClientInviteSessionHandle h, const SipMessage& msg)
{
   // We will recurse on redirect requests, so nothing to do here
   InfoLog(B2BLOG_PREFIX << "onRedirected: msg=" << LOG_MSG);
}

void
B2BSession::onAnswer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   InfoLog(B2BLOG_PREFIX << "onAnswer: msg=" << LOG_MSG_WITH_SDP);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         if(mPeer->mWaitingAnswerFromPeer)
         {
            mPeer->mWaitingAnswerFromPeer = false;
            if(mAnchorMedia)
            {
               // We need to answer with local sdp
               mPeer->provideLocalAnswer();
            }
            else
            {
               mPeer->mInviteSessionHandle->provideAnswer(sdp);
            }
         }
         else if(mPeer->mWaitingOfferFromPeer)
         {
            if(mAnchorMedia)
            {
               // We need to send a local sdp offer
               SdpContents localOffer;
               if(!buildLocalOffer(localOffer))
               {
                  ErrLog(<< "B2BSession::onAnswer: unable to build local offer.");
                  endPeer();
                  end();
                  return;
               }
               mPeer->mInviteSessionHandle->provideOffer(localOffer);
            }
            else
            {
               mPeer->mInviteSessionHandle->provideOffer(sdp);
            }
            mPeer->mWaitingOfferFromPeer = false;
         }

         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         { 
            sis->accept();
         }
      }
      else
      {
         if(!mPeer->mHasDialogSet && mPeer->mIChatWaitingToAccept)
         {
            // We are in a call setup phase for iChat -> SIP - peer is just an empty dialogset
            mPeer->acceptIChatCall();
         }
      }
   }
}

void
B2BSession::onOffer(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{         
   InfoLog(B2BLOG_PREFIX << "onOffer: msg=" << LOG_MSG_WITH_SDP);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mAnchorMedia)
      {
         // We need to answer with local sdp
         if(provideLocalAnswer())
         {
            ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mInviteSessionHandle.get());
            if(sis && !sis->isAccepted())
            { 
               sis->accept();
            }
         }
      }
      else if(mPeer->mInviteSessionHandle.isValid())
      {
         if(mPeer->mWaitingOfferFromPeer)
         {
            SdpContents fixedUpOffer;
            fixupSdp(sdp, fixedUpOffer);
            mPeer->mInviteSessionHandle->provideOffer(fixedUpOffer);
            mPeer->mWaitingOfferFromPeer = false;
            mWaitingAnswerFromPeer = true;
         }
         else
         {
            h->provideAnswer(h->getLocalSdp());
            /*  Experimental - appears to server no purpose
            if(sdp.session().media().front().exists("sendonly") ||
               sdp.session().media().front().exists("inactive"))
            {
               PlainContents msgBody("VCRemoteMuted:ON");
               mPeer->mInviteSessionHandle->message(msgBody);
            }
            else
            {
               PlainContents msgBody("VCRemoteMuted:OFF");
               mPeer->mInviteSessionHandle->message(msgBody);
            }*/
         }
      }
      else
      {
         resip_assert(false);
      }
   }
   else
   {
      // If we don't have a peer yet - we need to place a new call
      SdpContents fixedUpOffer;
      fixupSdp(sdp, fixedUpOffer);

      bool result;
      if(mIChatEndpoint)
      {
         try
         {
            // If we are an iChat endpoint then real To/From uri is actually in the display name of the To/From 
            // header.
            Uri to(Data("xmpp:") + msg.header(h_To).displayName());
            if(!msg.header(h_From).displayName().empty())
            {
               try
               {
                  NameAddr from(Data("sip:" + msg.header(h_From).displayName()));
                  if(!from.uri().user().empty())
                  {
                     from.displayName() = from.uri().user();
                  }
                  result = createNewPeer(to, from, &fixedUpOffer);
               }
               catch(resip::BaseException&)
               {
                  result = createNewPeer(to, msg.header(h_From), &fixedUpOffer);
               }
            }
            else
            {
               result = createNewPeer(to, msg.header(h_From), &fixedUpOffer);
            }
         }
         catch(resip::BaseException& e)
         {
            ErrLog(B2BLOG_PREFIX << "Error extracting real URI from iChat To header display name=" << msg.header(h_To) << ", error=" << e);
            result = false;
         }
      }
      else
      {
         result = createNewPeer(msg.header(h_To).uri(), msg.header(h_From),&fixedUpOffer);
      }
      if(!result)  
      {
         ErrLog(B2BLOG_PREFIX << "Failed to create new peer, rejecting call with 404.");
         h->reject(404);
      }
      else
      {
         mWaitingAnswerFromPeer = true;
      }
   }
}

void
B2BSession::onOfferRequired(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onOfferRequired: msg=" << LOG_MSG);

   if(isStaleFork(h->getDialogId())) return;

   if(mPeer)
   {
      if(mAnchorMedia)
      {
         // We need to offer with local sdp
         SdpContents sdp;
         if(!buildLocalOffer(sdp))
         {
            ErrLog(<< "B2BSession::onOfferRequired: unable to build local offer.");
            endPeer();
            end();
            return;
         }
         h->provideOffer(sdp);
      }
      else if(mPeer->mInviteSessionHandle.isValid())
      {
         mPeer->mInviteSessionHandle->requestOffer();
         mWaitingOfferFromPeer = true;
      }
      else
      {
         resip_assert(false);
      }
   }
   else
   {
      // If we don't have a peer yet - we need to place a new call
      if(!createNewPeer(msg.header(h_To).uri(), msg.header(h_From),0))  
      {
         ErrLog(B2BLOG_PREFIX << "Failed to create new peer, rejecting call with 404.");
         h->reject(404);
      }
      else
      {
         mWaitingOfferFromPeer = true;
      }
   }
}

void
B2BSession::onOfferRejected(InviteSessionHandle h, const SipMessage* msg)
{
   int statusCode = 488;
   WarningCategory* warning=0;

   if(isStaleFork(h->getDialogId())) return;

   if(msg)
   {
      InfoLog(B2BLOG_PREFIX << "onOfferRejected: msg=" << LOG_MSGP);
      if(msg->exists(h_Warnings))
      {
         warning = (WarningCategory*)&msg->header(h_Warnings).back();
      }
      if(msg->isResponse())
      {
         statusCode = msg->header(h_StatusLine).responseCode();
      }            
   }
   else
   {
      InfoLog(B2BLOG_PREFIX << "onOfferRejected:");
   }

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->reject(statusCode, warning);
         mPeer->mWaitingAnswerFromPeer = false;
      }
   }
}

void
B2BSession::onOfferRequestRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onOfferRequestRejected: msg=" << LOG_MSG);
   // This is called when we are waiting to resend a INVITE with no sdp after a glare condition, and we 
   // instead receive an inbound INVITE or UPDATE
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingOfferFromPeer)
      {
         // Return glare to peer
         mPeer->mInviteSessionHandle->reject(491);
         mPeer->mWaitingOfferFromPeer = false;
      }
   }
}

void
B2BSession::onRemoteSdpChanged(InviteSessionHandle h, const SipMessage& msg, const SdpContents& sdp)
{
   /// called when a modified SDP is received in a 2xx response to a
   /// session-timer reINVITE. Under normal circumstances where the response
   /// SDP is unchanged from current remote SDP no handler is called
   /// There is not much we can do about this.  If session timers are used then they are managed seperately per leg
   /// and we have no real mechanism to notify the other peer of new SDP without starting a new offer/answer negotiation
   InfoLog(B2BLOG_PREFIX << "onRemoteSdpChanged: msg=" << LOG_MSG_WITH_SDP);
}

void
B2BSession::onInfo(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onInfo: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && msg.getContents())
      {
         mPeer->mInviteSessionHandle->info(*msg.getContents());
         mWaitingNitAnswerFromPeer = true;
      }
   }
   else
   {
      h->acceptNIT();
   }
}

void
B2BSession::onInfoSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onInfoSuccess: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->acceptNIT(msg.header(h_StatusLine).responseCode(), msg.getContents());
      }
   }
}

void
B2BSession::onInfoFailure(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onInfoFailure: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->rejectNIT(msg.header(h_StatusLine).responseCode());
      }
   }
}

void
B2BSession::onRefer(InviteSessionHandle h, ServerSubscriptionHandle ss, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onRefer: msg=" << LOG_MSG);

   // If no peer (for some reason), then reject request
   if(!mPeer)
   {
      ss->send(ss->reject(403));
      return;
   }

   // Recurse on the REFER - do not pass to other leg
   try
   {
      // Accept the Refer
      ss->send(ss->accept(202 /* Refer Accepted */));

      B2BSession* newPeer = new B2BSession(mServer);

      SdpContents *pOffer = 0;
      SdpContents offer;
      if(mAnchorMedia)
      {
         buildLocalOffer(offer);
         pOffer = &offer;
      }
      else
      {
         mPeer->mWaitingOfferFromPeer = true;
      }

      // Map other leg to this new one
      newPeer->stealPeer(this);
      newPeer->mReferringAppDialogSet = getHandle();

      SharedPtr<SipMessage> invitemsg = mDum.makeInviteSessionFromRefer(msg, ss->getHandle(), pOffer, newPeer);
      mDum.send(invitemsg);
   }
   catch(BaseException &e)
   {
      WarningLog(B2BLOG_PREFIX << "onRefer exception: " << e);
   }
   catch(...)
   {
      WarningLog(B2BLOG_PREFIX << "onRefer unknown exception");
   }
}

void
B2BSession::onReferAccepted(InviteSessionHandle h, ClientSubscriptionHandle csh, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onReferAccepted: msg=" << LOG_MSG);
   end();  // Click-to-call refer request was accepted - end our call with the initiator
}

void
B2BSession::onReferRejected(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onReferRejected: msg=" << LOG_MSG);

   // Endpoint doesn't support REFER - Fallback to anchoring 
   /*
   mPeer = new B2BSession(mServer);
   mPeer->setPeer(this);
   mPeer->startClickToCallAnchorLeg(msg.header(h_From), mClickToCallDestination, mMediaRelayPort);
   if(mMediaRelayPort == 0)
   {
      mWaitingOfferFromPeer = true;
   }*/
}

bool 
B2BSession::doReferNoSub(const SipMessage& msg)
{
   // If no peer (for some reason), then return false
   if(!mPeer)
   {
      return false;
   }

   B2BSession* newPeer = new B2BSession(mServer);


   SdpContents *pOffer = 0;
   SdpContents offer;
   if(mAnchorMedia)
   {
      buildLocalOffer(offer);
      pOffer = &offer;
   }
   else
   {
      mPeer->mWaitingOfferFromPeer = true;
   }

   // Map other leg to this new one
   newPeer->stealPeer(this);
   newPeer->mReferringAppDialogSet = getHandle();

   // Build the Invite
   SharedPtr<SipMessage> invitemsg = mDum.makeInviteSessionFromRefer(msg, getUserProfile(), pOffer, newPeer);
   mDum.send(invitemsg);
   return true;
}

void
B2BSession::onReferNoSub(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onReferNoSub: msg=" << LOG_MSG);

   // If no peer (for some reason), then reject request
   if(!mPeer)
   {
      h->rejectReferNoSub(403);
      return;
   }

   // Accept the Refer
   h->acceptReferNoSub(202 /* Refer Accepted */);

   doReferNoSub(msg);
}

void
B2BSession::onMessage(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onMessage: msg=" << LOG_MSG);
   if(mPeer && !mIChatEndpoint)  // Note:  If iChat endpoint, then just respond to message, since iChat sends PING MESSAGES periodically and most phones don't respond
   {
      if(mPeer->mInviteSessionHandle.isValid() && msg.getContents())
      {
         mPeer->mInviteSessionHandle->message(*msg.getContents());
         mWaitingNitAnswerFromPeer = true;
      }
   }
   else
   {
      h->acceptNIT();
   }
}

void
B2BSession::onMessageSuccess(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onMessageSuccess: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->acceptNIT(msg.header(h_StatusLine).responseCode(), msg.getContents());
      }
   }
}

void
B2BSession::onMessageFailure(InviteSessionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onMessageFailure: msg=" << LOG_MSG);
   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid() && mPeer->mWaitingNitAnswerFromPeer)
      {
         mPeer->mInviteSessionHandle->rejectNIT(msg.header(h_StatusLine).responseCode());
      }
   }
}

void
B2BSession::onForkDestroyed(ClientInviteSessionHandle h)
{
   InfoLog(B2BLOG_PREFIX << "onForkDestroyed:");
}

////////////////////////////////////////////////////////////////////////////////
// DialogSetHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void 
B2BSession::onTrying(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onTrying: msg=" << LOG_MSG);
}

void 
B2BSession::onNonDialogCreatingProvisional(AppDialogSetHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onNonDialogCreatingProvisional: msg=" << LOG_MSG);

   if(isUACConnected()) return;

   if(mPeer)
   {
      if(mPeer->mInviteSessionHandle.isValid())
      {
         ServerInviteSession* sis = dynamic_cast<ServerInviteSession*>(mPeer->mInviteSessionHandle.get());
         if(sis && !sis->isAccepted())
         {
            sis->provisional(msg.header(h_StatusLine).responseCode());
         }
      }
   }
}

////////////////////////////////////////////////////////////////////////////////
// ClientSubscriptionHandler ///////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
void
B2BSession::onUpdatePending(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(B2BLOG_PREFIX << "onUpdatePending(ClientSubscriptionHandle): " << LOG_MSG);
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
B2BSession::onUpdateActive(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(B2BLOG_PREFIX << "onUpdateActive(ClientSubscriptionHandle): " << LOG_MSG);
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
B2BSession::onUpdateExtension(ClientSubscriptionHandle h, const SipMessage& msg, bool outOfOrder)
{
   InfoLog(B2BLOG_PREFIX << "onUpdateExtension(ClientSubscriptionHandle): " << LOG_MSG);
   if (msg.exists(h_Event) && msg.header(h_Event).value() == "refer")
   {
      h->acceptUpdate();
   }
   else
   {
      h->rejectUpdate(400, Data("Only notifies for refers are allowed."));
   }
}

void
B2BSession::onTerminated(ClientSubscriptionHandle h, const SipMessage* msg)
{
   if(msg)
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated(ClientSubscriptionHandle): " << LOG_MSGP);
   }
   else
   {
      InfoLog(B2BLOG_PREFIX << "onTerminated(ClientSubscriptionHandle)");
   }
   // Note:  Final notify is sometimes only passed in the onTerminated callback
   //if (notify.isRequest() && notify.exists(h_Event) && notify.header(h_Event).value() == "refer")
   //{
   //}
}

void
B2BSession::onNewSubscription(ClientSubscriptionHandle h, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onNewSubscription(ClientSubscriptionHandle): " << LOG_MSG);
}

int 
B2BSession::onRequestRetry(ClientSubscriptionHandle h, int retryMinimum, const SipMessage& msg)
{
   InfoLog(B2BLOG_PREFIX << "onRequestRetry(ClientSubscriptionHandle): " << LOG_MSG);
   return -1;
}

}

/* ====================================================================

 Copyright (c) 2009, SIP Spectrum, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of SIP Spectrum nor the names of its contributors 
    may be used to endorse or promote products derived from this 
    software without specific prior written permission. 

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
 LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
 A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
 SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
 LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
 THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
 OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 ==================================================================== */

