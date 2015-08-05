#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <asio.hpp>
#ifdef USE_SSL
#include <asio/ssl.hpp>
#endif
#include <boost/function.hpp>

#include <rutil/Log.hxx>
#include <rutil/Logger.hxx>
#include <rutil/Timer.hxx>
#include <rutil/Lock.hxx>

#include "FlowManagerSubsystem.hxx"
#include "ErrorCode.hxx"
#include "Flow.hxx"
#include "MediaStream.hxx"
#include "FlowDtlsSocketContext.hxx"

using namespace flowmanager;
using namespace resip;

#ifdef USE_SSL
using namespace dtls;
#endif 

using namespace std;

#define MAX_RECEIVE_FIFO_DURATION 10 // seconds
#define MAX_RECEIVE_FIFO_SIZE (100 * MAX_RECEIVE_FIFO_DURATION)  // 1000 = 1 message every 10 ms for 10 seconds - appropriate for RTP

#define RESIPROCATE_SUBSYSTEM FlowManagerSubsystem::FLOWMANAGER

const char* srtp_error_string(err_status_t error)
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
#ifdef USE_SSL
    mSslContext(sslContext),
#endif
    mComponentId(componentId),
    mLocalBinding(localBinding), 
    mMediaStream(mediaStream),
    mAllocationProps(StunMessage::PropsNone),
    mReservationToken(0),
    mFlowState(Unconnected),
    mReceivedDataFifo(MAX_RECEIVE_FIFO_DURATION,MAX_RECEIVE_FIFO_SIZE)
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
      resip_assert(false);
   }

   if(mTurnSocket.get() && 
      mMediaStream.mNatTraversalMode != MediaStream::NoNatTraversal && 
      !mMediaStream.mStunUsername.empty() && 
      !mMediaStream.mStunPassword.empty())
   {
      mTurnSocket->setUsernameAndPassword(mMediaStream.mStunUsername.c_str(), mMediaStream.mStunPassword.c_str(), false);
   }
}

Flow::~Flow() 
{
   InfoLog(<< "Flow: flow destroyed for " << mLocalBinding << "  ComponentId=" << mComponentId);


#ifdef USE_SSL
   // Cleanup DtlsSockets
   {
      Lock lock(mMutex);
      std::map<reTurn::StunTuple, dtls::DtlsSocket*>::iterator it;
      for(it = mDtlsSockets.begin(); it != mDtlsSockets.end(); it++)
      {
         delete it->second;
      }
   }
 #endif //USE_SSL

   // Cleanup TurnSocket
   if(mTurnSocket.get())
   {
      mTurnSocket->disableTurnAsyncHandler();
      mTurnSocket->close();  
   }
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
                              mMediaStream.mNatTraversalServerPort);
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
Flow::send(char* buffer, unsigned int size)
{
   resip_assert(mTurnSocket.get());
   if(isReady())
   {
      if(processSendData(buffer, size, mTurnSocket->getConnectedAddress(), mTurnSocket->getConnectedPort()))
      {
         mTurnSocket->send(buffer, size);
      }
   }
   else
   {
      onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::InvalidState, asio::error::misc_category));
   }
}

void
Flow::sendTo(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int size)
{
   resip_assert(mTurnSocket.get());
   if(isReady())
   {
      if(processSendData(buffer, size, address, port))
      {
         mTurnSocket->sendTo(address, port, buffer, size);
      }    
    }
   else
   {
      onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::InvalidState, asio::error::misc_category));
   }
}

// Note: this fn is used to send raw data to the far end, without attempting to SRTP encrypt it - ie. used for sending DTLS traffic
void
Flow::rawSendTo(const asio::ip::address& address, unsigned short port, const char* buffer, unsigned int size)
{
   resip_assert(mTurnSocket.get());
   mTurnSocket->sendTo(address, port, buffer, size);
}


bool
Flow::processSendData(char* buffer, unsigned int& size, const asio::ip::address& address, unsigned short port)
{
   if(mMediaStream.mSRTPSessionOutCreated)
   {
      err_status_t status = mMediaStream.srtpProtect((void*)buffer, (int*)&size, mComponentId == RTCP_COMPONENT_ID);
      if(status != err_status_ok)
      {
         ErrLog(<< "Unable to SRTP protect the packet, error code=" << status << "(" << srtp_error_string(status) << ")  ComponentId=" << mComponentId);
         onSendFailure(mTurnSocket->getSocketDescriptor(), asio::error_code(flowmanager::SRTPError, asio::error::misc_category));
         return false;
      }
   }
#ifdef USE_SSL
   else
   {
      Lock lock(mMutex);
      DtlsSocket* dtlsSocket = getDtlsSocket(StunTuple(mLocalBinding.getTransportType(), address, port));
      if(dtlsSocket)
      {
         if(((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->isSrtpInitialized())
         {
            err_status_t status = ((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->srtpProtect((void*)buffer, (int*)&size, mComponentId == RTCP_COMPONENT_ID);
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
   }   
#endif //USE_SSL
   
   return true;
}



// Receive Methods
asio::error_code 
Flow::receiveFrom(const asio::ip::address& address, unsigned short port, char* buffer, unsigned int& size, unsigned int timeout)
{
   bool done = false;
   asio::error_code errorCode;

   UInt64 startTime = Timer::getTimeMs();
   unsigned int recvTimeout;
   while(!done)
   {
      // We define timeout of 0 differently then TimeLimitFifo - we want 0 to mean no-block at all
      if(timeout == 0 && mReceivedDataFifo.empty())
      {
         // timeout
         return asio::error_code(flowmanager::ReceiveTimeout, asio::error::misc_category);
      }

      recvTimeout = timeout ? (unsigned int)(timeout - (Timer::getTimeMs() - startTime)) : 0;
      if(timeout != 0 && recvTimeout <= 0)
      {
         // timeout
         return asio::error_code(flowmanager::ReceiveTimeout, asio::error::misc_category);
      }
      ReceivedData* receivedData = mReceivedDataFifo.getNext(recvTimeout);  
      if(receivedData)
      {
         mFakeSelectSocketDescriptor.receive();

         // discard any data not from address/port requested
         if(address == receivedData->mAddress && port == receivedData->mPort)
         {
            errorCode = processReceivedData(buffer, size, receivedData);
            done = true;
         }
         delete receivedData;
      }
      else
      {
         // timeout
         errorCode = asio::error_code(flowmanager::ReceiveTimeout, asio::error::misc_category);
         done = true;
      }
   }
   return errorCode;
}

asio::error_code 
Flow::receive(char* buffer, unsigned int& size, unsigned int timeout, asio::ip::address* sourceAddress, unsigned short* sourcePort)
{
   asio::error_code errorCode;

   //InfoLog(<< "Flow::receive called with buffer size=" << size << ", timeout=" << timeout);
   // We define timeout of 0 differently then TimeLimitFifo - we want 0 to mean no-block at all
   if(timeout == 0 && mReceivedDataFifo.empty())
   {
      // timeout
      InfoLog(<< "Receive timeout (timeout==0 and fifo empty)!");
      return asio::error_code(flowmanager::ReceiveTimeout, asio::error::misc_category);
   }
   if(mReceivedDataFifo.empty())
   {
      WarningLog(<< "Receive called when there is no data available!  ComponentId=" << mComponentId);
   }

   ReceivedData* receivedData = mReceivedDataFifo.getNext(timeout);   
   if(receivedData)
   {
      mFakeSelectSocketDescriptor.receive();
      errorCode = processReceivedData(buffer, size, receivedData, sourceAddress, sourcePort);
      delete receivedData;
   }
   else
   {
      // timeout
      InfoLog(<< "Receive timeout!  ComponentId=" << mComponentId);
      errorCode = asio::error_code(flowmanager::ReceiveTimeout, asio::error::misc_category);
   }
   return errorCode;
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
         //errorCode = asio::error_code(flowmanager::SRTPError, asio::error::misc_category);
      }
   }
#ifdef USE_SSL
   else
   {
      Lock lock(mMutex);
      DtlsSocket* dtlsSocket = getDtlsSocket(StunTuple(mLocalBinding.getTransportType(), receivedData->mAddress, receivedData->mPort));
      if(dtlsSocket)
      {
         if(((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->isSrtpInitialized())
         {
            err_status_t status = ((FlowDtlsSocketContext*)dtlsSocket->getSocketContext())->srtpUnprotect((void*)receivedData->mData->data(), (int*)&receivedsize, mComponentId == RTCP_COMPONENT_ID);
            if(status != err_status_ok)
            {
               ErrLog(<< "Unable to SRTP unprotect the packet (componentid=" << mComponentId << "), error code=" << status << "(" << srtp_error_string(status) << ")");
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
#endif //USE_SSL
   if(!errorCode)
   {
      if(size > receivedsize)
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
Flow::setActiveDestination(const char* address, unsigned short port)
{
   if(mTurnSocket.get())
   {
      if(mMediaStream.mNatTraversalMode != MediaStream::TurnAllocation)
      {         
         changeFlowState(Connecting);
         mTurnSocket->connect(address, port);
      }
      else
      {
         mTurnSocket->setActiveDestination(asio::ip::address::from_string(address), port);

      }
   }
   else
      WarningLog(<<"No TURN Socket, can't send media to destination");
}

#ifdef USE_SSL
void 
Flow::startDtlsClient(const char* address, unsigned short port)
{
   Lock lock(mMutex);
   createDtlsSocketClient(StunTuple(mLocalBinding.getTransportType(), asio::ip::address::from_string(address), port));
}
#endif 

void 
Flow::setRemoteSDPFingerprint(const resip::Data& fingerprint)
{
   Lock lock(mMutex);
   mRemoteSDPFingerprint = fingerprint;

#ifdef USE_SSL
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
#endif //USE_SSL
}

const resip::Data 
Flow::getRemoteSDPFingerprint() 
{ 
   Lock lock(mMutex);
   return mRemoteSDPFingerprint; 
}

const StunTuple& 
Flow::getLocalTuple() 
{ 
   return mLocalBinding; 
}  

StunTuple
Flow::getSessionTuple()
{
   resip_assert(mFlowState == Ready);
   Lock lock(mMutex);

   if(mMediaStream.mNatTraversalMode == MediaStream::TurnAllocation)
   {
      return mRelayTuple;
   }
   else if(mMediaStream.mNatTraversalMode == MediaStream::StunBindDiscovery)
   {
      return mReflexiveTuple;
   }
   return mLocalBinding;
}

StunTuple
Flow::getRelayTuple() 
{ 
   resip_assert(mFlowState == Ready);
   Lock lock(mMutex);
   return mRelayTuple; 
}  

StunTuple
Flow::getReflexiveTuple() 
{ 
   resip_assert(mFlowState == Ready);
   Lock lock(mMutex);
   return mReflexiveTuple; 
} 

UInt64 
Flow::getReservationToken()
{
   resip_assert(mFlowState == Ready);
   Lock lock(mMutex);
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
      if(mFlowState == ConnectingServer)
      {
         changeFlowState(Binding);
         mTurnSocket->bindRequest();
      }
      else
      {
         changeFlowState(Ready);
         mMediaStream.onFlowReady(mComponentId);
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
   InfoLog(<< "Flow::onBindingSuccess: socketDesc=" << socketDesc << ", reflexive=" << reflexiveTuple << ", componentId=" << mComponentId);
   {
      Lock lock(mMutex);
      mReflexiveTuple = reflexiveTuple;
   }
   changeFlowState(Ready);
   mMediaStream.onFlowReady(mComponentId);
}
void 
Flow::onBindFailure(unsigned int socketDesc, const asio::error_code& e, const StunTuple& stunServerTuple)
{
   WarningLog(<< "Flow::onBindingFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
   changeFlowState(Connected);
   mMediaStream.onFlowError(mComponentId, e.value());  // TODO define different error code?
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
      Lock lock(mMutex);
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
Flow::onChannelBindRequestSent(unsigned int socketDesc, unsigned short channelNumber)
{
   InfoLog(<< "Flow::onChannelBindRequestSent: socketDesc=" << socketDesc << ", channelNumber=" << channelNumber << ", componentId=" << mComponentId);
}

void 
Flow::onChannelBindSuccess(unsigned int socketDesc, unsigned short channelNumber)
{
   InfoLog(<< "Flow::onChannelBindSuccess: socketDesc=" << socketDesc << ", channelNumber=" << channelNumber << ", componentId=" << mComponentId);
}

void 
Flow::onChannelBindFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onChannelBindFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId );
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
   DebugLog(<< "Flow::onReceiveSuccess: socketDesc=" << socketDesc << ", fromAddress=" << address.to_string() << ", fromPort=" << port << ", size=" << data->size() << ", componentId=" << mComponentId);

#ifdef USE_SSL
   // Check if packet is a dtls packet - if so then process it
   // Note:  Stun messaging should be picked off by the reTurn library - so we only need to tell the difference between DTLS and SRTP here
   if(DtlsFactory::demuxPacket((const unsigned char*) data->data(), data->size()) == DtlsFactory::dtls)
   {
      Lock lock(mMutex);

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

   if(!mReceivedDataFifo.add(new ReceivedData(address, port, data), ReceivedDataFifo::EnforceTimeDepth))
   {
      WarningLog(<< "Flow::onReceiveSuccess: TimeLimitFifo is full - discarding data!  componentId=" << mComponentId);
   }
   else
   {
      mFakeSelectSocketDescriptor.send();
   }
}

void 
Flow::onReceiveFailure(unsigned int socketDesc, const asio::error_code& e)
{
   WarningLog(<< "Flow::onReceiveFailure: socketDesc=" << socketDesc << " error=" << e.value() << "(" << e.message() << "), componentId=" << mComponentId);

   // Make sure we keep receiving if we get an ICMP error on a UDP socket
   if(e.value() == asio::error::connection_reset && mLocalBinding.getTransportType() == StunTuple::UDP)
   {
      resip_assert(mTurnSocket.get());
      mTurnSocket->turnReceive();
   }
}

void 
Flow::onIncomingBindRequestProcessed(unsigned int socketDesc, const StunTuple& sourceTuple)
{
   InfoLog(<< "Flow::onIncomingBindRequestProcessed: socketDesc=" << socketDesc << ", sourceTuple=" << sourceTuple );
   // TODO - handle
}

void 
Flow::changeFlowState(FlowState newState)
{
   InfoLog(<< "Flow::changeState: oldState=" << flowStateToString(mFlowState) << ", newState=" << flowStateToString(newState) << ", componentId=" << mComponentId);
   mFlowState = newState;
}

const char*
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
   default:
      resip_assert(false);
      return "Unknown";
   }
}

#ifdef USE_SSL
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

#endif 

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
