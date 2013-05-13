#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asio.hpp>
#include <boost/function.hpp>
#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Timer.hxx>
#include <rutil/Lock.hxx>

#include "FlowManagerSubsystem.hxx"
#include "ReflowErrorCode.hxx"
#include "Flow.hxx"
#include "MediaStream.hxx"
#include "FlowHandler.hxx"
#ifdef USE_SSL
#ifdef USE_DTLS
#include "FlowDtlsSocketContext.hxx"
#endif
#endif

using namespace flowmanager;
using namespace resip;
using namespace dtls;
using namespace std;

#define MAX_RECEIVE_FIFO_DURATION 10 // seconds
#define MAX_RECEIVE_FIFO_SIZE (100 * MAX_RECEIVE_FIFO_DURATION)  // 1000 = 1 message every 10 ms for 10 seconds - appropriate for RTP
#define ICMP_GRACE_PERIOD_SECONDS 5 // allow ICMP errors for this many seconds before onReceiveFailure
#define CONNECTIVITY_CHECK_MAX_RETRANSMITS 20 // 7 is the default for Rc spec'd in RFC 5389
#define CONNECTIVITY_CHECK_RETRANS_INTERVAL_MS 100 // http://tools.ietf.org/html/rfc5245#section-16

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

char* srtp_error_string(err_status_t error)
{
   switch(error)
   {
   case err_status_ok:  
      return "nothing to report";
      break;
   case err_status_fail:
      return "unspecified failure";
      break;
   case err_status_bad_param:
      return "unsupported parameter";
      break;
   case err_status_alloc_fail:
      return "couldn't allocate memory";
      break;
   case err_status_dealloc_fail:
      return "couldn't deallocate properly";
      break;
   case err_status_init_fail:
      return "couldn't initialize";
      break;
   case err_status_terminus:
      return "can't process as much data as requested";
      break;
   case err_status_auth_fail:
      return "authentication failure";
      break;
   case err_status_cipher_fail:
      return "cipher failure";
      break;
   case err_status_replay_fail:
      return "replay check failed (bad index)";
      break;
   case err_status_replay_old:
      return "replay check failed (index too old)";
      break;
   case err_status_algo_fail:
      return "algorithm failed test routine";
      break;
   case err_status_no_such_op:
      return "unsupported operation";
      break;
   case err_status_no_ctx:
      return "no appropriate context found";
      break;
   case err_status_cant_check:
      return "unable to perform desired validation";
      break;
   case err_status_key_expired:
      return "can't use key any more";
      break;
   case err_status_socket_err:
      return "error in use of socket";
      break;
   case err_status_signal_err:
      return "error in use POSIX signals";
      break;
   case err_status_nonce_bad:
      return "nonce check failed";
      break;
   case err_status_read_fail:
      return "couldn't read data";
      break;
   case err_status_write_fail:
      return "couldn't write data";
      break;
   case err_status_parse_err:
      return "error pasring data";
      break;
   case err_status_encode_err:
      return "error encoding data";
      break;
   case err_status_semaphore_err:
      return "error while using semaphores";
      break;
   case err_status_pfkey_err:
      return "error while using pfkey";
      break;
   default:
      return "unrecognized error";
   }
}

Flow::Flow(asio::io_service& ioService,
#ifdef USE_SSL
           asio::ssl::context& sslContext,
#endif
           unsigned int componentId,
           const StunTuple& localBinding, 
           MediaStream& mediaStream) 
  : mIOService(ioService),
    mConnectivityCheckTimer(mIOService),
#ifdef USE_SSL
    mSslContext(sslContext),
#endif
    mComponentId(componentId),
    mLocalBinding(localBinding), 
    mMediaStream(mediaStream),
    mAllocationProps(StunMessage::PropsNone),
    mReservationToken(0),
    mFlowState(Unconnected),
    mReceivedDataFifo(MAX_RECEIVE_FIFO_DURATION,MAX_RECEIVE_FIFO_SIZE),
    mHandler(NULL),
    mActiveDestinationSet(false),
    mConnectivityChecksPending(false),
    mIceComplete(false),
    mIceRole(Flow::IceRole_Unknown),
    mPeerRflxCandidatePriority(0)
{
}

Flow::~Flow() 
{
}

// this is only called from MediaStream::initializeImpl(..)
void
Flow::initialize()
{
   InfoLog(<< "Flow: flow created for " << mLocalBinding << "  ComponentId=" << mComponentId);

   switch(mLocalBinding.getTransportType())
   {
   case StunTuple::UDP:
      mTurnSocket.reset(new TurnAsyncUdpSocket(mIOService, this, mLocalBinding.getAddress(), mLocalBinding.getPort()));
      break;
   case StunTuple::TCP:
      mTurnSocket.reset(new TurnAsyncTcpSocket(mIOService, this, mLocalBinding.getAddress(), mLocalBinding.getPort()));
      break;
#ifdef USE_SSL
   case StunTuple::TLS:
      mTurnSocket.reset(new TurnAsyncTlsSocket(mIOService, 
                                               mSslContext, 
                                               false, // validateServerCertificateHostname - TODO - make this configurable
                                               this, 
                                               mLocalBinding.getAddress(), 
                                               mLocalBinding.getPort()));
#endif
      break;
   default:
      // Bad Transport type!
      assert(false);
   }

   if(mTurnSocket.get() && 
      mMediaStream.mNatTraversalMode != MediaStream::NoNatTraversal && 
      !mMediaStream.mStunUsername.empty() && 
      !mMediaStream.mStunPassword.empty())
   {
      mTurnSocket->setUsernameAndPassword(mMediaStream.mStunUsername.c_str(), mMediaStream.mStunPassword.c_str(), false);
   }
}

// this is only called from MediaStream::shutdownImpl(..)
void 
Flow::shutdown()
   {
      Lock lock(mMutex);
   if (mTurnSocket.get())
      {
      mTurnSocket->disableTurnAsyncHandler();
      mTurnSocket->close();
      }
   mConnectivityCheckTimer.cancel();

#ifdef USE_SSL
#ifdef USE_DTLS
   // Cleanup DtlsSockets
      std::map<reTurn::StunTuple, dtls::DtlsSocket*>::iterator it;
      for(it = mDtlsSockets.begin(); it != mDtlsSockets.end(); it++)
      {
         delete it->second;
      }
#endif //USE_DTLS
#endif //USE_SSL

   InfoLog(<< "Flow: flow destroyed for " << mLocalBinding << "  ComponentId=" << mComponentId);
}

void
Flow::setHandlerImpl(FlowHandler* handler, resip::Condition& cv)
{
   Lock lock(mMutex);
   mHandler = handler;
   cv.signal();
   }

void
Flow::setHandler(FlowHandler* handler)
{
   Lock lock(mMutex);
   Condition cv;
   mIOService.post(boost::bind(&Flow::setHandlerImpl, this, handler, boost::ref(cv)));
   cv.wait(mMutex);
}

void 
Flow::activateFlow(UInt64 reservationToken)
{
   mReservationToken = reservationToken;
   activateFlow(StunMessage::PropsNone);
}

void 
Flow::activateFlow(UInt8 allocationProps)
{
   mAllocationProps = allocationProps;

   if(mTurnSocket.get())
   {
      if(mMediaStream.mNatTraversalMode != MediaStream::NoNatTraversal &&
         !mMediaStream.mNatTraversalServerHostname.empty())
      {
         changeFlowState(ConnectingServer);
         mTurnSocket->connect(mMediaStream.mNatTraversalServerHostname.c_str(), 
                              mMediaStream.mNatTraversalServerPort,
                              mLocalBinding.getAddress().is_v6());
      }
      else if (mMediaStream.mNatTraversalMode != MediaStream::NoNatTraversal &&
         mMediaStream.mNatTraversalServerHostname.empty())
      {
         // pretend we're connected and got our binding
         mReflexiveTuple = mLocalBinding;
         changeFlowState(Ready);
         mMediaStream.onFlowReady(mComponentId);
      }
      else 
      {
         changeFlowState(Ready);
         mMediaStream.onFlowReady(mComponentId);
      }
   }
}

unsigned int 
Flow::getSelectSocketDescriptor()
{
   return mFakeSelectSocketDescriptor.getSocketDescriptor();
}

unsigned int 
Flow::getSocketDescriptor()
{
   if(mTurnSocket.get() != 0)
   {
      return mTurnSocket->getSocketDescriptor();
   }
   else
   {
      return 0;
   }
}

// Turn Send Methods
void
Flow::send(boost::shared_ptr<reTurn::DataBuffer>& buffer)
{
   mIOService.post(boost::bind(&Flow::sendImpl, this, buffer));
}

void
Flow::sendImpl(boost::shared_ptr<reTurn::DataBuffer>& buffer)
{
   assert(mTurnSocket.get());
   if(isReady() && mActiveDestinationSet)
   {
      if(processSendData(buffer, mTurnSocket->getConnectedAddress(), mTurnSocket->getConnectedPort()))
      {
         // !jjg! this if-else will need to be reworked (along with the one in setActiveDestinationImpl)
         // before we can possibly support relay candidates for ICE
         if (mMediaStream.mNatTraversalMode == MediaStream::TurnAllocation)
         {
            mTurnSocket->sendFramed(buffer);
   }
   else
   {
            mTurnSocket->sendUnframed(buffer);
}
      }    
    }
   else
   {
      WarningLog(<< "Flow::send(..) failed in state " << mFlowState);
      onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::InvalidState, asio::error::misc_category));
   }
}

// Note: this fn is used to send raw data to the far end, without attempting to SRTP encrypt it - ie. used for sending DTLS traffic
void
Flow::rawSendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
{
   assert(mTurnSocket.get());
   mTurnSocket->sendTo(address, port, buffer, size);
}


bool
Flow::processSendData(boost::shared_ptr<reTurn::DataBuffer>& buffer, const asio::ip::address& address, unsigned short port)
{
   if(mMediaStream.mSRTPSessionOutCreated)
   {
      char* data = buffer->mutableData();
      int size = buffer->size();
      err_status_t status = mMediaStream.srtpProtect((void*)data, (int*)&size, mComponentId == RTCP_COMPONENT_ID);
      buffer->mutableSize() = size;
      if(status != err_status_ok)
      {
         ErrLog(<< "Unable to SRTP protect the packet, error code=" << status << "(" << srtp_error_string(status) << ")  ComponentId=" << mComponentId);
         onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::SRTPError, asio::error::misc_category));
         return false;
      }
   }
#ifdef USE_SSL
#ifdef USE_DTLS
   else
   {
      DtlsSocket* dtlsSocket = getDtlsSocket(StunTuple(mLocalBinding.getTransportType(), address, port));
      if(dtlsSocket)
      {
         if(((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->isSrtpInitialized())
         {
            int size = buffer->size();
            err_status_t status = ((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->srtpProtect((char*)buffer->mutableData(), (int*)&size, mComponentId == RTCP_COMPONENT_ID);
            buffer->mutableSize() = size;
            if(status != err_status_ok)
            {
               ErrLog(<< "Unable to SRTP protect the packet, error code=" << status << "(" << srtp_error_string(status) << ")  ComponentId=" << mComponentId);
               onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::SRTPError, asio::error::misc_category));
               return false;
            }
         }
         else
         {
            //WarningLog(<< "Unable to send packet yet - handshake is not completed yet, ComponentId=" << mComponentId);
            onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::InvalidState, asio::error::misc_category));
            return false;
         }
      }
      else if (mMediaStream.mSRTPEnabled)
      {
         return false;
      }
   }   
#endif //USE_DTLS
#endif //USE_SSL
   
   return true;
}

void
Flow::asyncReceive()
{
   mIOService.post(boost::bind(&Flow::asyncReceiveImpl, this));
}

void
Flow::asyncReceiveImpl()
{
   mTurnSocket->turnReceive();
}

asio::error_code 
Flow::processReceivedData(char* buffer, unsigned int& size, ReceivedData* receivedData, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;
   unsigned int receivedsize = receivedData->mData->size();

   // SRTP Unprotect (if required)
   if(mMediaStream.mSRTPSessionInCreated)
   {
      err_status_t status = mMediaStream.srtpUnprotect((void*)receivedData->mData->data(), (int*)&receivedsize, mComponentId == RTCP_COMPONENT_ID);
      if(status != err_status_ok)
      {
         ErrLog(<< "Unable to SRTP unprotect the packet (componentid=" << mComponentId << "), error code=" << status << "(" << srtp_error_string(status) << ")");
         memset((void*)receivedData->mData->data(), 0, receivedData->mData->size());
         //errorCode = asio::error_code(flowmanager::SRTPError, asio::error::misc_category);
      }
   }
#ifdef USE_SSL
#ifdef USE_DTLS
   else
   {
      DtlsSocket* dtlsSocket = getDtlsSocket(StunTuple(mLocalBinding.getTransportType(), receivedData->mAddress, receivedData->mPort));
      if(dtlsSocket)
      {
         if(((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->isSrtpInitialized())
         {
            err_status_t status = ((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->srtpUnprotect((void*)receivedData->mData->mutableData(), (int*)&receivedsize, mComponentId == RTCP_COMPONENT_ID);
            if(status != err_status_ok)
            {
               ErrLog(<< "Unable to SRTP unprotect the packet (componentid=" << mComponentId << "), error code=" << status << "(" << srtp_error_string(status) << ")");
               memset((void*)receivedData->mData->data(), 0, receivedData->mData->size());
               //errorCode = asio::error_code(flowmanager::SRTPError, asio::error::misc_category);
            }
         }
         else
         {
            //WarningLog(<< "Unable to send packet yet - handshake is not completed yet, ComponentId=" << mComponentId);
            errorCode = asio::error_code(flowmanager::InvalidState, asio::error::misc_category);
         }
      }
   }
#endif //USE_DTLS
#endif //USE_SSL
   if(!errorCode)
   {
      if(size >= receivedsize)
      {
         size = receivedsize;
         memcpy(buffer, receivedData->mData->data(), size);
         //InfoLog(<< "Received a buffer of size=" << receivedData->mData.size());
      }
      else
      {
         // Receive buffer too small
         InfoLog(<< "Receive buffer too small for data size=" << receivedsize << "  ComponentId=" << mComponentId);
         errorCode = asio::error_code(flowmanager::BufferTooSmall, asio::error::misc_category);
      }
      if(sourceAddress)
      {
         *sourceAddress = receivedData->mAddress;
      }
      if(sourcePort)
      {
         *sourcePort = receivedData->mPort;
      }
   }
   return errorCode;
}

void 
Flow::setIceRole(bool controlling)
{
   mIOService.post(boost::bind(&Flow::setIceRoleImpl, this, controlling));
}

void
Flow::setIceRoleImpl(bool controlling)
{
   mIceRole = controlling ? Flow::IceRole_Controlling : Flow::IceRole_Controlled;
}

void Flow::setPeerReflexiveCandidatePriority(UInt32 priority)
{
   mIOService.post(boost::bind(&Flow::setPeerReflexiveCandidatePriorityImpl, this, priority));
}

void 
Flow::setActiveDestination(const resip::Data& address, unsigned short port, const std::vector<reTurn::IceCandidate>& candidates)
{
   mIOService.post(boost::bind(&Flow::setActiveDestinationImpl, this, address, port, candidates));
}

void 
Flow::setActiveDestinationImpl(const resip::Data& address, unsigned short port, const std::vector<reTurn::IceCandidate>& candidates)
{
   if(mTurnSocket.get())
   {
      if(mMediaStream.mNatTraversalMode != MediaStream::TurnAllocation)
      {         
         if (mMediaStream.mNatTraversalMode == MediaStream::Ice && candidates.size() > 0)
         {
            if (!mIceComplete)
            {
               bool isRtpFlow = (mMediaStream.getRtpFlow() == this);

               std::vector<reTurn::IceCandidate>::const_iterator candIt = candidates.begin();
               for (; candIt != candidates.end(); ++candIt)
               {
                  // first attempt to find and update any candidates that we may have inserted due to receiving
                  // BIND requests before receiving the SDP answer
                  bool alreadyInCheckList = false;
                  std::vector<IceCandidatePair>::iterator candPairIt = mIceCheckList.begin();
                  for (; candPairIt != mIceCheckList.end(); ++candPairIt)
                  {
                     if (candPairIt->mRemoteCandidate.getTransportAddr() == candIt->getTransportAddr())
                     {
                        alreadyInCheckList = true;
                        candPairIt->mRemoteCandidate = *candIt;
                        candPairIt->mState = IceCandidatePair::Waiting;
                        break;
                     }
                  }

                  if (!alreadyInCheckList)
                  {
                     DebugLog(<< "adding ice candidate pair for remote candidate " << candIt->getTransportAddr() << " to ICE check list");
                     IceCandidatePair candidatePair;
                     candidatePair.mLocalCandidate = IceCandidate(mLocalBinding, IceCandidate::CandidateType_Host, 0, Data::Empty, mComponentId, StunTuple());
                     candidatePair.mRemoteCandidate = *candIt;
                     candidatePair.mState = (isRtpFlow? IceCandidatePair::Waiting : IceCandidatePair::Frozen);
                     mIceCheckList.push_back(candidatePair);
                  }
               }

               if (mFlowState == Ready)
               {
                  if (isRtpFlow)
                  {
                     DebugLog(<< "scheduling initial ICE connectivity checks for RTP flow");
                     changeFlowState(CheckingConnectivity);
                     scheduleConnectivityChecks();
                  }
                  else
                  {
                     DebugLog(<< "setting RTCP flow to CheckingConnectivity state");
                     changeFlowState(CheckingConnectivity);
                  }
               }
               else
               {
                  DebugLog(<< "delaying ICE connectivity checks until flow state is Ready; current state is " << flowStateToString(mFlowState));
                  mConnectivityChecksPending = true;
               }
            }
         }
         else
         {
         changeFlowState(Connecting);
            mTurnSocket->connect(address.c_str(), port, mLocalBinding.getAddress().is_v6());
         }
      }
      else
      {
         mTurnSocket->setActiveDestination(asio::ip::address::from_string(address.c_str()), port);
      }
      mActiveDestinationSet = true;
   }
}

void
Flow::scheduleConnectivityChecks()
{
   InfoLog(<< "Schedule ICE connectivity checks for " << (mComponentId == RTP_COMPONENT_ID ? "RTP" : "RTCP") << " Flow");
   if(mTurnSocket.get() && mMediaStream.mNatTraversalMode == MediaStream::Ice)
   {
      mConnectivityCheckTimer.cancel();
      // !jjg! todo: follow the formula for Ta in http://tools.ietf.org/html/draft-ietf-mmusic-ice-19#section-16
      mConnectivityCheckTimer.expires_from_now(boost::posix_time::milliseconds(20));
      mConnectivityCheckTimer.async_wait(boost::bind(&Flow::onConnectivityCheckTimer, this, asio::placeholders::error));
   }
}

void
Flow::setOutgoingIceUsernameAndPassword(const resip::Data& username, const resip::Data& password)
{
   if (!username.empty() && !password.empty())
   {
      if (username != mOutgoingIceUsername || password != mOutgoingIcePassword)
      {
         mIceComplete = false;
      }
      mOutgoingIceUsername = username;
      mOutgoingIcePassword = password;
   }
}

void
Flow::setLocalIcePassword(const resip::Data& password)
{
   if (!password.empty())
   {
      mTurnSocket->setLocalPassword(password.c_str());
   }
}

void
Flow::onConnectivityCheckTimer(const asio::error_code& error)
{
   if (error == asio::error::operation_aborted)
   {
      return;
      }

   if (mFlowState == CheckingConnectivity)
   {
      DebugLog(<< "set username/password for ICE: " << mOutgoingIceUsername << ", " << mOutgoingIcePassword);
      mTurnSocket->setUsernameAndPassword(mOutgoingIceUsername.c_str(), mOutgoingIcePassword.c_str(), true);

      // the RTP flow is the only one that does a connectivity check at this point,
      // since it and the RTCP flow share the same foundation
      std::vector<IceCandidatePair>::iterator candPairIt = mIceCheckList.begin();
      for (; candPairIt != mIceCheckList.end(); ++candPairIt)
      {
         const reTurn::IceCandidate& c = candPairIt->mRemoteCandidate;
         if (candPairIt->mState == IceCandidatePair::Waiting)
         {
            candPairIt->mState = IceCandidatePair::InProgress;
            if (c.getTransportAddr() == mLocalBinding) // .jjg. it is possible that one of the remote candidiates
                                                       // has the same IP address and port as us; just skip the check if so
            {
               DebugLog(<< "remote candidate has the same IP:port as our local binding; skip this one " << c.getTransportAddr().getAddress().to_string() << ":" << c.getTransportAddr().getPort());
               candPairIt->mState = IceCandidatePair::Failed;
   }
   else
            {
               // !jjg! todo: also pass in something to indicate the retransmission interval RTO
               mTurnSocket->connectivityCheck(
                  c.getTransportAddr(), 
                  mPeerRflxCandidatePriority, 
                  mIceRole == Flow::IceRole_Controlling, 
                  mIceRole == Flow::IceRole_Controlled,
                  mIceRole == Flow::IceRole_Controlled ? CONNECTIVITY_CHECK_MAX_RETRANSMITS*2 : CONNECTIVITY_CHECK_MAX_RETRANSMITS,
                  CONNECTIVITY_CHECK_RETRANS_INTERVAL_MS);
               DebugLog(<< "checking connectivity to remote candidate " << c.getTransportAddr().getAddress().to_string() << ":" << c.getTransportAddr().getPort());
               break;
            }
         }
      }

      // make the timer fire Ta seconds from now to do a check for the next
      // candidate in the Waiting state (if any)
      scheduleConnectivityChecks();
   }
}

#ifdef USE_SSL
#ifdef USE_DTLS
void 
Flow::startDtlsClient(const char* address, unsigned short port)
{
   mIOService.post(boost::bind(&Flow::startDtlsClientImpl, this, std::string(address), port));
}

void
Flow::startDtlsClientImpl(const std::string& address, unsigned short port)
{
   createDtlsSocketClient(StunTuple(mLocalBinding.getTransportType(), asio::ip::address::from_string(address), port));
}

void 
Flow::setRemoteSDPFingerprint(const resip::Data& fingerprint)
{
   mIOService.post(boost::bind(&Flow::setRemoteSDPFingerprintImpl, this, fingerprint));
}

void
Flow::setRemoteSDPFingerprintImpl(const resip::Data& fingerprint)
{
   mRemoteSDPFingerprint = fingerprint;

   // Check all existing DtlsSockets and tear down those that don't match
   std::map<reTurn::StunTuple, dtls::DtlsSocket*>::iterator it;
   for(it = mDtlsSockets.begin(); it != mDtlsSockets.end(); it++)
   {
      if(it->second->handshakeCompleted() && 
         !it->second->checkFingerprint(fingerprint.c_str(), fingerprint.size()))
      {
         InfoLog(<< "Marking Dtls socket bad with non-matching fingerprint!");
         ((FlowDtlsSocketContext*)it->second->getSocketContext())->fingerprintMismatch();
      }
   }
}

const resip::Data 
Flow::getRemoteSDPFingerprint() 
{ 
   return mRemoteSDPFingerprint; 
}
#endif // USE_DTLS
#endif // USE_SSL


// !jjg! needs to be re-implemented with a thread-safe interface
//void Flow::setOnBeforeSocketClosedFp(boost::function<void(unsigned int)> fp)
//{
//   Lock lock(mMutex);
//
//   if(!mTurnSocket.get())
//      return;
//
//   mTurnSocket->setOnBeforeSocketClosedFp(fp);
//}

const StunTuple& 
Flow::getLocalTuple() 
{ 
   return mLocalBinding; 
}  

StunTuple
Flow::getSessionTuple()
{
   if(mMediaStream.mNatTraversalMode == MediaStream::TurnAllocation)
   {
      assert(mFlowState == Ready);
      return mRelayTuple;
   }
   else if(mMediaStream.mNatTraversalMode == MediaStream::Ice)
   {
      assert(mFlowState == Ready || mFlowState == Connected);
      std::vector<IceCandidatePair>::iterator candIt = mIceCheckList.begin();
      for (; candIt != mIceCheckList.end(); ++candIt)
      {
         if (candIt->mState == IceCandidatePair::Succeeded)
         {
            return candIt->mLocalCandidate.getTransportAddr();
         }
      }
      if (mReflexiveTuple.getTransportType() != StunTuple::None)
      {
         return mReflexiveTuple; // should not get here
      }
      return mLocalBinding;
   }
   else if(mMediaStream.mNatTraversalMode == MediaStream::StunBindDiscovery)
   {
      assert(mFlowState == Ready);
      return mReflexiveTuple;
   }
   return mLocalBinding;
}

reTurn::IceCandidate 
Flow::getLocalNominatedIceCandidate() const
{
   std::vector<IceCandidatePair>::const_iterator candIt = mIceCheckList.begin();
   for (; candIt != mIceCheckList.end(); ++candIt)
   {
      if (candIt->mState == IceCandidatePair::Succeeded)
      {
         return candIt->mLocalCandidate;
      }
   }
   return reTurn::IceCandidate();
}

reTurn::IceCandidate 
Flow::getRemoteNominatedIceCandidate() const
{
   std::vector<IceCandidatePair>::const_iterator candIt = mIceCheckList.begin();
   for (; candIt != mIceCheckList.end(); ++candIt)
   {
      if (candIt->mState == IceCandidatePair::Succeeded)
      {
         return candIt->mRemoteCandidate;
      }
   }
   return reTurn::IceCandidate();
}

StunTuple
Flow::getRelayTuple() 
{ 
   assert(mFlowState == Ready);
   return mRelayTuple; 
}  

StunTuple
Flow::getReflexiveTuple() 
{ 
   assert(mFlowState == Ready);
   return mReflexiveTuple; 
} 

UInt64 
Flow::getReservationToken()
{
   assert(mFlowState == Ready);
   return mReservationToken; 
}

void 
Flow::onConnectSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port)
{
   InfoLog(<< "Flow::onConnectSuccess: socketDesc=" << socketDesc << ", address=" << address.to_string() << ", port=" << port << ", componentId=" << mComponentId);

   // Start candidate discovery
   switch(mMediaStream.mNatTraversalMode)
   {
   case MediaStream::StunBindDiscovery:
   case MediaStream::Ice:
      if(mFlowState == ConnectingServer)
      {
         changeFlowState(Binding);
         mTurnSocket->bindRequest();
      }
      else
      {
         changeFlowState(Ready);
         if (mMediaStream.mNatTraversalMode == MediaStream::Ice && mIceComplete)
         {
            std::vector<IceCandidatePair>::iterator candIt = mIceCheckList.begin();
            for (; candIt != mIceCheckList.end(); ++candIt)
            {
               if (candIt->mState == IceCandidatePair::Succeeded)
               {
                  mMediaStream.onFlowIceComplete(mComponentId, (mIceRole == IceRole_Controlling));
                  break;
               }
            }
            mIceRole = IceRole_Unknown;
         }
      }
      break;
   case MediaStream::TurnAllocation:
      changeFlowState(Allocating);
      mTurnSocket->createAllocation(TurnAsyncSocket::UnspecifiedLifetime,
                                    TurnAsyncSocket::UnspecifiedBandwidth,
                                    mAllocationProps, 
                                    mReservationToken != 0 ? mReservationToken : TurnAsyncSocket::UnspecifiedToken,
                                    StunTuple::UDP);   // Always relay as UDP
      break;
   case MediaStream::NoNatTraversal:
   default:
      changeFlowState(Ready);
      mMediaStream.onFlowReady(mComponentId);
      break;
   }
}

void 
Flow::onConnectFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onConnectFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << ", componentId=" << mComponentId);
   changeFlowState(Unconnected);
   mMediaStream.onFlowError(mComponentId, e.value());  // TODO define different error code?
}


void 
Flow::onSharedSecretSuccess(unsigned int socketDesc, const char* username, unsigned int usernameSize, const char* password, unsigned int passwordSize)
{
   InfoLog(<< "Flow::onSharedSecretSuccess: socketDesc=" << socketDesc << ", username=" << username << ", password=" << password << ", componentId=" << mComponentId);
}

void 
Flow::onSharedSecretFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onSharedSecretFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
}

void 
Flow::onBindSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& stunServerTuple)
{
   InfoLog(<< "Flow::onBindingSuccess: socketDesc=" << socketDesc << ", reflexive=" << reflexiveTuple << ", componentId=" << mComponentId << ", stunServer=" << stunServerTuple);

   if (mFlowState == CheckingConnectivity && mMediaStream.getRtpFlow() == this)
   {
      // this candidate is a good one! use it
      mConnectivityCheckTimer.cancel();
      std::vector<IceCandidatePair>::iterator candIt = mIceCheckList.begin();
      std::auto_ptr<IceCandidatePair> nominatedCandidatePair;
      bool newPeerReflexiveCandidateDiscovered = false;
      for (; candIt != mIceCheckList.end(); ++candIt)
      {
         if (candIt->mRemoteCandidate.getTransportAddr() == stunServerTuple)
         {
            if (candIt->mLocalCandidate.getTransportAddr() == reflexiveTuple)
            {
               nominatedCandidatePair.reset(new IceCandidatePair(*candIt));
               candIt->mState = IceCandidatePair::Succeeded;
            }
            else
            {
               newPeerReflexiveCandidateDiscovered = true;
            }
            break;
         }
      }
      if (nominatedCandidatePair.get() == NULL && newPeerReflexiveCandidateDiscovered)
      {
         IceCandidatePair newPeerRflxCandidate;
         newPeerRflxCandidate.mRemoteCandidate = candIt->mRemoteCandidate;
         newPeerRflxCandidate.mLocalCandidate = IceCandidate(reflexiveTuple, IceCandidate::CandidateType_Prflx, 0, resip::Data::Empty, mComponentId, mLocalBinding);
         newPeerRflxCandidate.mState = IceCandidatePair::Succeeded;
         mIceCheckList.push_back(newPeerRflxCandidate);
         nominatedCandidatePair.reset(new IceCandidatePair(newPeerRflxCandidate));
      }
      
      if (nominatedCandidatePair.get() != NULL)
      {
         changeFlowState(Connecting);
         mTurnSocket->connect(stunServerTuple.getAddress().to_string(), stunServerTuple.getPort(), mLocalBinding.getAddress().is_v6());
         DebugLog(<< "connecting RTP flow to " << stunServerTuple.getAddress().to_string() << ":" << stunServerTuple.getPort());
         //mIceRole = IceRole_Unknown;
         mIceComplete = true;

         // time to make the RTCP flow do its connectivity check, using the candidate with the same foundation
         // as that of the RTP candidate that has just succeeded
         candIt = mMediaStream.getRtcpFlow()->mIceCheckList.begin();
         for (; candIt != mMediaStream.getRtcpFlow()->mIceCheckList.end(); ++candIt)
         {
            if (candIt->mRemoteCandidate.getFoundation() == nominatedCandidatePair->mRemoteCandidate.getFoundation())
            {
               mMediaStream.getRtcpFlow()->changeFlowState(CheckingConnectivity);

               DebugLog(<< "set username/password for ICE (RTCP flow): " << mOutgoingIceUsername << ", " << mOutgoingIcePassword);
               mMediaStream.getRtcpFlow()->mTurnSocket->setUsernameAndPassword(mOutgoingIceUsername.c_str(), mOutgoingIcePassword.c_str(), true);

               // .jjg. do the connectivity check here directly rather than
               // going through the onConnectivityCheckTimer(), since we are just trying
               // this single candidate which is pretty much guaranteed to work...  
               reTurn::IceCandidate& c = candIt->mRemoteCandidate;
               candIt->mState = IceCandidatePair::InProgress;
               mMediaStream.getRtcpFlow()->mTurnSocket->connectivityCheck(
                  c.getTransportAddr(), 
                  mMediaStream.getRtcpFlow()->mPeerRflxCandidatePriority, 
                  mMediaStream.getRtcpFlow()->mIceRole == Flow::IceRole_Controlling, 
                  mMediaStream.getRtcpFlow()->mIceRole == Flow::IceRole_Controlled,
                  CONNECTIVITY_CHECK_MAX_RETRANSMITS,
                  CONNECTIVITY_CHECK_RETRANS_INTERVAL_MS);
               DebugLog(<< "(RTCP flow) checking connectivity to remote candidate " << c.getTransportAddr().getAddress().to_string() << ":" << c.getTransportAddr().getPort());
            }
         }
      }
   }
   else if (mFlowState == CheckingConnectivity && mMediaStream.getRtcpFlow() == this)
   {
      // this candidate is a good one! use it
      std::vector<IceCandidatePair>::iterator candIt = mIceCheckList.begin();
      std::auto_ptr<IceCandidatePair> nominatedCandidatePair;
      bool newPeerReflexiveCandidateDiscovered = false;
      for (; candIt != mIceCheckList.end(); ++candIt)
      {
         if (candIt->mRemoteCandidate.getTransportAddr() == stunServerTuple)
         {
            if (candIt->mLocalCandidate.getTransportAddr() == reflexiveTuple)
            {
               nominatedCandidatePair.reset(new IceCandidatePair(*candIt));
               candIt->mState = IceCandidatePair::Succeeded;
            }
            else
            {
               newPeerReflexiveCandidateDiscovered = true;
            }
            break;
         }
      }
      if (nominatedCandidatePair.get() == NULL && newPeerReflexiveCandidateDiscovered)
      {
         IceCandidatePair newPeerRflxCandidate;
         newPeerRflxCandidate.mRemoteCandidate = candIt->mRemoteCandidate;
         newPeerRflxCandidate.mLocalCandidate = IceCandidate(reflexiveTuple, IceCandidate::CandidateType_Prflx, 0, resip::Data::Empty, mComponentId, mLocalBinding);
         newPeerRflxCandidate.mState = IceCandidatePair::Succeeded;
         mIceCheckList.push_back(newPeerRflxCandidate);
         nominatedCandidatePair.reset(new IceCandidatePair(newPeerRflxCandidate));
      }

      if (nominatedCandidatePair.get() != NULL)
      {
         changeFlowState(Connecting);
         mTurnSocket->connect(stunServerTuple.getAddress().to_string(), stunServerTuple.getPort(), mLocalBinding.getAddress().is_v6());
         DebugLog(<< "connecting RTCP flow to " << stunServerTuple.getAddress().to_string() << ":" << stunServerTuple.getPort());
         //mIceRole = IceRole_Unknown;
         mIceComplete = true;
      }
   }
   else if (mMediaStream.mNatTraversalMode == MediaStream::Ice && 
      (mFlowState == Connecting || mFlowState == Connected || mFlowState == Ready))
   {
      // ignore; we probably had multiple ICE candidates that succeeded, and we've already picked
      // the first successful one
      DebugLog(<< "ignoring onBindSuccess() in state " << flowStateToString(mFlowState));
   }
   else
   {
      mReflexiveTuple = reflexiveTuple;
   changeFlowState(Ready);
   mMediaStream.onFlowReady(mComponentId);

      // this block is necessary because setActiveDestination(..) is called BEFORE our server reflexive
      // candidate is disconvered (i.e. before we get to the previous three lines of code) ...
      // when we are UAS for the transaction, onFlowReady results in our answer going out HOWEVER
      // setActiveDestination is NOT called again -- so we need to manually kick-start ICE connectivity checks
      if (mConnectivityChecksPending)
      {
         if (mMediaStream.getRtpFlow() == this && mMediaStream.getRtcpFlow() == NULL)
         {
            changeFlowState(CheckingConnectivity);
            scheduleConnectivityChecks();
            mConnectivityChecksPending = false;
}
         else if (mMediaStream.getRtcpFlow() == this && mMediaStream.getRtpFlow()->isReady())
         {
            mMediaStream.getRtpFlow()->changeFlowState(CheckingConnectivity);
            mMediaStream.getRtpFlow()->scheduleConnectivityChecks();
            changeFlowState(CheckingConnectivity);
            mMediaStream.getRtpFlow()->mConnectivityChecksPending = false;
            mConnectivityChecksPending = false;
         }
         else if (mMediaStream.getRtpFlow() == this && mMediaStream.getRtcpFlow()->isReady())
         {
            changeFlowState(CheckingConnectivity);
            scheduleConnectivityChecks();
            mConnectivityChecksPending = false;
            mMediaStream.getRtcpFlow()->changeFlowState(CheckingConnectivity);
            mMediaStream.getRtcpFlow()->mConnectivityChecksPending = false;
         }
      }
   }
}

void 
Flow::onBindFailure(unsigned int socketDesc, const asio::error_code& e, const StunTuple& stunServerTuple)
{
   WarningLog(<< "Flow::onBindingFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
   if (mMediaStream.mNatTraversalMode == MediaStream::Ice && mFlowState == CheckingConnectivity)
   {
      unsigned int numFailed = 0;
      unsigned int numSucceeded = 0;
      std::vector<IceCandidatePair>::iterator candIt = mIceCheckList.begin();
      for (; candIt != mIceCheckList.end(); ++candIt)
      {
         if (candIt->mRemoteCandidate.getTransportAddr() == stunServerTuple)
         {
            candIt->mState = IceCandidatePair::Failed;
         }
         if (candIt->mState == IceCandidatePair::Failed)
         {
            numFailed++;
         }
         else if (candIt->mState == IceCandidatePair::Succeeded)
         {
            numSucceeded++;
         }
      }

      if ((numSucceeded == 0) && (numFailed > 0) && (numFailed == mIceCheckList.size()))
      {
         InfoLog(<< (mComponentId == RTP_COMPONENT_ID ? "RTP" : "RTCP") << ": all " << numFailed << " remote candidates have failed; no more remote candidates to try");
         mConnectivityCheckTimer.cancel();
         changeFlowState(Connected); // ICE failed, but we can still use the flow;
                                     // use Connected state for now so that we don't stream media
                                     // until setActiveDestination is called
         mMediaStream.onFlowIceFailed(mComponentId, (mIceRole == IceRole_Controlling));
         if (mMediaStream.getRtcpFlow())
         {
            mMediaStream.getRtcpFlow()->changeFlowState(Connected);
            mMediaStream.onFlowIceFailed(mMediaStream.getRtcpFlow()->mComponentId, (mIceRole == IceRole_Controlling));
         }
      } 
   }
   else if (mMediaStream.mNatTraversalMode == MediaStream::Ice && 
      (mFlowState == Connecting || mFlowState == Connected || mFlowState == Ready))
   {
      // we are good to go, but we get a bind failure?
      // just ignore this condition, since RTP is probably flowing at this point
      DebugLog(<< "ignoring onBindFailure() in state " << flowStateToString(mFlowState));
   }
   else
   {
   changeFlowState(Connected);
   mMediaStream.onFlowError(mComponentId, e.value());  // TODO define different error code?
}
}

void 
Flow::onIncomingBindRequestProcessed(unsigned int socketDest, const StunTuple& sourceTuple, const reTurn::StunMessage& bindRequest)
{
   InfoLog(<< "Flow::onIncomingBindRequestProcessed, socketDest=" << socketDest << ", sourceTuple=" << sourceTuple);
   // this is essentially a 'triggered check'; the other side sent a connectivity check to us
   // from sourceTuple, so we want to immediately send one back to it (if we haven't already)
   // so that we can speed up convergeance
   if (mMediaStream.mNatTraversalMode == MediaStream::Ice && !mOutgoingIceUsername.empty())
   {
      std::vector<IceCandidatePair>::iterator candIt = mIceCheckList.begin();
      bool candidateExists = false;
      for (; candIt != mIceCheckList.end(); ++candIt)
      {
         if (candIt->mRemoteCandidate.getTransportAddr() == sourceTuple)
         {
            candidateExists = true;
            if (candIt->mState == IceCandidatePair::Waiting || candIt->mState == IceCandidatePair::Failed)
            {
               DebugLog(<< "set username/password for ICE: " << mOutgoingIceUsername << ", " << mOutgoingIcePassword);
               mTurnSocket->setUsernameAndPassword(mOutgoingIceUsername.c_str(), mOutgoingIcePassword.c_str(), true);

               candIt->mState = IceCandidatePair::InProgress;
               const reTurn::IceCandidate& c = candIt->mRemoteCandidate;
               mTurnSocket->connectivityCheck( // !jjg! FIXME: race condition here, ICE might already be complete!
                  c.getTransportAddr(), 
                  mPeerRflxCandidatePriority, 
                  mIceRole == Flow::IceRole_Controlling || !bindRequest.mHasIceControlling, 
                  mIceRole == Flow::IceRole_Controlled || bindRequest.mHasIceControlling,
                  CONNECTIVITY_CHECK_MAX_RETRANSMITS,
                  CONNECTIVITY_CHECK_RETRANS_INTERVAL_MS);
               DebugLog(<< "checking connectivity to remote candidate " << c.getTransportAddr().getAddress().to_string() << ":" << c.getTransportAddr().getPort());
            }
            break;
         }
      }

      // handle the case where we get a connectivity check from the remote end before
      // setActiveDestination(..) is called
      if (!candidateExists)
      {
         InfoLog(<< "creating placeholder candidate for sourceTuple " << sourceTuple);
         // create a 'placeholder' candidate at the top of the check list;
         // this should get updated once we get the remote candidates in the SDP answer
         // @see Flow::setActiveDestination(..)
         IceCandidatePair candidatePair;
         candidatePair.mLocalCandidate = IceCandidate(mLocalBinding, IceCandidate::CandidateType_Host, 0, Data::Empty, mComponentId, StunTuple());
         candidatePair.mRemoteCandidate = IceCandidate(sourceTuple, IceCandidate::CandidateType_Unknown, 0, Data::Empty, 0, StunTuple());
         candidatePair.mState = IceCandidatePair::Waiting;
         mIceCheckList.push_back(candidatePair);
      }
   }
   else
   {
      DebugLog(<< "can't do triggered check; mMediaStream.mNatTraversalMode == " << mMediaStream.mNatTraversalMode << ", mOutgoingIceUsername == " << mOutgoingIceUsername);
   }
}

void 
Flow::onAllocationSuccess(unsigned int socketDesc, const StunTuple& reflexiveTuple, const StunTuple& relayTuple, unsigned int lifetime, unsigned int bandwidth, UInt64 reservationToken)
{
   InfoLog(<< "Flow::onAllocationSuccess: socketDesc=" << socketDesc << 
      ", reflexive=" << reflexiveTuple << 
      ", relay=" << relayTuple <<
      ", lifetime=" << lifetime <<
      ", bandwidth=" << bandwidth << 
      ", reservationToken=" << reservationToken <<
      ", componentId=" << mComponentId);
   {
      mReflexiveTuple = reflexiveTuple; 
      mRelayTuple = relayTuple;
      mReservationToken = reservationToken;
   }
   changeFlowState(Ready);
   mMediaStream.onFlowReady(mComponentId);
}

void 
Flow::onAllocationFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onAllocationFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
   changeFlowState(Connected);
   mMediaStream.onFlowError(mComponentId, e.value());  // TODO define different error code?
}

void 
Flow::onRefreshSuccess(unsigned int socketDesc, unsigned int lifetime)
{
   InfoLog(<< "Flow::onRefreshSuccess: socketDesc=" << socketDesc << ", lifetime=" << lifetime << ", componentId=" << mComponentId);
   if(lifetime == 0)
   {
      changeFlowState(Connected);
   }
}

void 
Flow::onRefreshFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onRefreshFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
}

void 
Flow::onSetActiveDestinationSuccess(unsigned int socketDesc)
{
   InfoLog(<< "Flow::onSetActiveDestinationSuccess: socketDesc=" << socketDesc << ", componentId=" << mComponentId);
}

void 
Flow::onSetActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onSetActiveDestinationFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
}

void 
Flow::onClearActiveDestinationSuccess(unsigned int socketDesc)
{
   InfoLog(<< "Flow::onClearActiveDestinationSuccess: socketDesc=" << socketDesc << ", componentId=" << mComponentId);
}

void 
Flow::onClearActiveDestinationFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onClearActiveDestinationFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
}

void 
Flow::onSendSuccess(unsigned int socketDesc)
{
   //InfoLog(<< "Flow::onSendSuccess: socketDesc=" << socketDesc);
}

void 
Flow::onSendFailure(unsigned int socketDesc, const asio::error_code& e)
{
   if(e.value() == InvalidState)
   {
      // Note:  if setActiveDestination is called it can take some time to "connect" the socket to the destination
      //        and send requests during this time, will be discarded - this can be considered normal
      InfoLog(<< "Flow::onSendFailure: socketDesc=" << socketDesc << " socket is not in correct state to send yet, componentId=" << mComponentId );
   }
   else
   {
      WarningLog(<< "Flow::onSendFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
   }
}

void 
Flow::onReceiveSuccess(unsigned int socketDesc, const asio::ip::address& address, unsigned short port, boost::shared_ptr<reTurn::DataBuffer>& data)
{
   StackLog(<< "Flow::onReceiveSuccess: socketDesc=" << socketDesc << ", fromAddress=" << address.to_string() << ", fromPort=" << port << ", size=" << data->size() << ", componentId=" << mComponentId);

#ifdef USE_SSL
#ifdef USE_DTLS
   // Check if packet is a dtls packet - if so then process it
   // Note:  Stun messaging should be picked off by the reTurn library - so we only need to tell the difference between DTLS and SRTP here
   if(DtlsFactory::demuxPacket((const unsigned char*) data->data(), data->size()) == DtlsFactory::dtls)
   {
      StunTuple endpoint(mLocalBinding.getTransportType(), address, port);
      DtlsSocket* dtlsSocket = getDtlsSocket(endpoint);
      if(!dtlsSocket)
      {
         // If don't have a socket already for this endpoint and we are receiving data, then assume we are the server side of the DTLS connection
         dtlsSocket = createDtlsSocketServer(endpoint);
      }
      if(dtlsSocket)
      { 
         dtlsSocket->handlePacketMaybe((const unsigned char*) data->data(), data->size());
      }

      // Packet was a DTLS packet - do not queue for app
      return;
   }
#endif 
#endif
   if (mHandler)
   {
      bool usingSRTP = 
#ifdef USE_SSL
#ifdef USE_DTLS
         mMediaStream.mDtlsFactory || 
#endif
#endif
         mMediaStream.mSRTPSessionInCreated;
      asio::error_code errorCode;
      if (usingSRTP) // only do this if we're using SRTP (saves on a memcpy)
      {
         unsigned int ncbuff_size = data->size() * 2; // a guesstimate
         char* ncbuff = new char[ncbuff_size];
         memset(ncbuff, 0, ncbuff_size);

         ReceivedData recvData(address, port, data);
         errorCode = processReceivedData(ncbuff, ncbuff_size, &recvData);
         if (!errorCode)
         {
            // The size of the data may have changed. Make sure we create a
            // new DataBuffer with the new contents after decoding. Use this
            // to forward to the system.
            boost::shared_ptr<reTurn::DataBuffer> newBuf(reTurn::DataBuffer::own(ncbuff, ncbuff_size));
            data.swap(newBuf);
         }
      }
      else if (mMediaStream.mSRTPEnabled && !mMediaStream.mSRTPSessionInCreated)
      {
         // we haven't processed the remote party's answer yet, so we haven't
         // setup our inbound SRTP session and can't decode this data; toss it
         WarningLog(<< "discarding incoming SRTP since we don't have an inbound SRTP session yet");
         return;
      }

      if (!errorCode)
      {
         mHandler->onReceiveSuccess(this, socketDesc, address, port, data);
      }
      else
      {
         mHandler->onReceiveFailure(this, socketDesc, errorCode);
      }
   }
   else
   {
   if(!mReceivedDataFifo.add(new ReceivedData(address, port, data), ReceivedDataFifo::EnforceTimeDepth))
   {
      WarningLog(<< "Flow::onReceiveSuccess: TimeLimitFifo is full - discarding data!  componentId=" << mComponentId);
   }
   else
   {
      mFakeSelectSocketDescriptor.send();
   }
}
}

void 
Flow::onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onReceiveFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId);

   if ((e.value() == asio::error::connection_refused || e.value() == asio::error::connection_reset)
       && mLocalBinding.getTransportType() == StunTuple::UDP)
   {
      // .jjg. keep receiving -- after endless support tickets where ICMP errors were leading to Bria
      // hanging up (even with a grace period), I've decided it's better to just let 'er keep trucking along
      // as though nothing is wrong ...  BUT we'll still call the onReceiveFailure callback so that apps can 
      // customize their behaviour in this regard
      mTurnSocket->turnReceive();

}

   if (mHandler)
{
      mHandler->onReceiveFailure(this, socketDesc, e);
   }
}

void 
Flow::changeFlowState(FlowState newState)
{
   InfoLog(<< "Flow::changeState: oldState=" << flowStateToString(mFlowState) << ", newState=" << flowStateToString(newState) << ", componentId=" << mComponentId);
   mFlowState = newState;
}

char*
Flow::flowStateToString(FlowState state)
{
   switch(state)
   {
   case Unconnected:
      return "Unconnected";
   case ConnectingServer:
      return "ConnectingServer";
   case Connecting:
      return "Connecting";
   case Binding:
      return "Binding";
   case Allocating:
      return "Allocating";
   case Connected:
      return "Connected";
   case Ready:
      return "Ready";
   case CheckingConnectivity:
      return "CheckingConnectivity";
   default:
      assert(false);
      return "Unknown";
   }
}

#ifdef USE_SSL
#ifdef USE_DTLS
DtlsSocket* 
Flow::getDtlsSocket(const StunTuple& endpoint)
{
   std::map<reTurn::StunTuple, dtls::DtlsSocket*>::iterator it = mDtlsSockets.find(endpoint);
   if(it != mDtlsSockets.end())
   {
      return it->second;
   }
   return 0;
}

DtlsSocket* 
Flow::createDtlsSocketClient(const StunTuple& endpoint)
{
   DtlsSocket* dtlsSocket = getDtlsSocket(endpoint);
   if(!dtlsSocket && mMediaStream.mDtlsFactory)
   {
      InfoLog(<< "Creating DTLS Client socket, componentId=" << mComponentId);
      std::auto_ptr<DtlsSocketContext> socketContext(new FlowDtlsSocketContext(*this, endpoint.getAddress(), endpoint.getPort()));
      dtlsSocket = mMediaStream.mDtlsFactory->createClient(socketContext);
      dtlsSocket->startClient();
      mDtlsSockets[endpoint] = dtlsSocket;
   }
   return dtlsSocket;
}

DtlsSocket* 
Flow::createDtlsSocketServer(const StunTuple& endpoint)
{
   DtlsSocket* dtlsSocket = getDtlsSocket(endpoint);
   if(!dtlsSocket && mMediaStream.mDtlsFactory)
   {
      InfoLog(<< "Creating DTLS Server socket, componentId=" << mComponentId);
      std::auto_ptr<DtlsSocketContext> socketContext(new FlowDtlsSocketContext(*this, endpoint.getAddress(), endpoint.getPort()));
      dtlsSocket = mMediaStream.mDtlsFactory->createServer(socketContext);
      mDtlsSockets[endpoint] = dtlsSocket;
   }

   return dtlsSocket;
}
#endif // USE_DTLS
#endif // USE_SSL

/* ====================================================================

 Copyright (c) 2007-2008, Plantronics, Inc.
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are 
 met:

 1. Redistributions of source code must retain the above copyright 
    notice, this list of conditions and the following disclaimer. 

 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution. 

 3. Neither the name of Plantronics nor the names of its contributors 
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
