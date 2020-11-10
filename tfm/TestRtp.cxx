#include "tfm/TestRtp.hxx"
#include "tfm/RtpEvent.hxx"

#include "rutil/Logger.hxx"
#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/DnsUtil.hxx"
#include "resip/stack/SipMessage.hxx"

#include "Expect.hxx"
#include "CommonAction.hxx"

#include <ctime>
#include <functional>

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST


using namespace resip;
using namespace std;

//#define USE_XS_TIME

//#define HAS_PCAP

#ifdef HAS_PCAP
#include <pcap.h>
#endif

#define ETHERNET_HEADER_SIZE 14
#define IP_HEADER_SIZE 20
#define UDP_HEADER_SIZE 8
#define RTP_HEADER_SIZE 12
#define RTCP_HEADER_SIZE 8
#define OVERHEAD_SIZE (ETHERNET_HEADER_SIZE + IP_HEADER_SIZE + UDP_HEADER_SIZE)

#define SDP_USER "tfm"
#define SDP_SESSION_ID 1000
#define SDP_INITIAL_VERSION  1
#define SDP_SESSION_NAME "tfm call"

#define RTCP_SR    200
#define RTCP_RR    201
#define RTCP_SDES  202
#define RTCP_BYE   203
#define RTCP_APP   204

// RTP interval in ms
#define RTP_INTERVAL  20
// RTCP interval in seconds
#define RTCP_INTERVAL 1

// .bwc. Not used. Do we plan on ever uncommenting the code that uses this?
//static int
//timeval_subtract(timeval *result, timeval* x, timeval*y)
//{
//  /* Perform the carry for the later subtraction by updating y. */
//  if (x->tv_usec < y->tv_usec) {
//    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
//    y->tv_usec -= 1000000 * nsec;
//    y->tv_sec += nsec;
//  }
//  if (x->tv_usec - y->tv_usec > 1000000) {
//    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
//    y->tv_usec += 1000000 * nsec;
//    y->tv_sec -= nsec;
//  }
//
//  /* Compute the time remaining to wait.
//     tv_usec is certainly positive. */
//  result->tv_sec = x->tv_sec - y->tv_sec;
//  result->tv_usec = x->tv_usec - y->tv_usec;
//
//  /* Return 1 if result is negative. */
//  return x->tv_sec < y->tv_sec;
//}

PacketSenderStatistics::PacketSenderStatistics() :
   mSent(0)
{
}

void
PacketSenderStatistics::clear()
{
   mSent = 0;
}

PacketReceiverStatistics::PacketReceiverStatistics() :
   mReceived(0),
   mLost(0)

{
}

void
PacketReceiverStatistics::clear()
{
   mReceived = 0;
   mLost = 0;
}


///////////////////////////////////////////////////////////////////////////////
TestRtp::TestRtp() :
   mFdRtp(INVALID_SOCKET),
   mFdRtcp(INVALID_SOCKET),
   mFdVideoRtp(INVALID_SOCKET),
   mFdVideoRtcp(INVALID_SOCKET),
   mLocalAddrRtp("127.0.0.1", 8000, V4, UDP),
   mLocalAddrRtcp("127.0.0.1", 8001, V4, UDP),
   mLocalAddrVideoRtp("127.0.0.1", 8002, V4, UDP),
   mLocalAddrVideoRtcp("127.0.0.1", 8003, V4, UDP),
   mRemoteAddrRtp("0.0.0.0", 0, V4, UDP),
   mRemoteAddrRtcp("0.0.0.0", 0, V4, UDP),
   mRemoteSsrc(0),
   mRemoteCodec(0),
   mSenderStatistics(),
   mReceiverStatistics(),
   mHold2543(false),
   mDescribeWellKnownCodec(true),
   mPtime(0),
   mPacketsRtp(),
   mPacketsRtcp(),
   mSendPackets(false)
{
#ifndef WIN32
   mLocalSsrc = random();
//   mLocalSsrc = 0;
#else
   srand((unsigned int) time(NULL));
   mLocalSsrc = rand() % 64000;
#endif

   clean();
}

// ----------------------------------------------------------------------------
void
TestRtp::clean()
{
   mLocalSdp = std::make_shared<SdpContents>();
   mLocalSdp->session().name() = SDP_SESSION_NAME;
   mLocalSdp->session().version() = 0;
   mLocalSdp->session().origin()
      = SdpContents::Session::Origin(SDP_USER, SDP_SESSION_ID, SDP_INITIAL_VERSION, SdpContents::IP4, Data::Empty);

   mPtime = 0;
   mHold2543 = false;
   mDescribeWellKnownCodec = true;
   mSendPackets = false;

   addBasicAudioCodecs();
}

// ----------------------------------------------------------------------------
void
TestRtp::addBasicAudioCodecs()
{
   const Codec & codec1 = SdpContents::Session::Codec::ULaw_8000;
   const Codec & codec2 = SdpContents::Session::Codec::TelephoneEvent;

   addAudioCodec(codec1.getName(), codec1.payloadType(), codec1.getRate(), codec1.parameters());
   addAudioCodec(codec2.getName(), codec2.payloadType(), codec2.getRate(), codec2.parameters());
}

resip::Socket
TestRtp::openSocket(TransportType type)
{
   Socket fd;
   switch( type )
   {
      case UDP:
         fd = ::socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
         break;

      case TCP:
      case TLS:
      default:
         InfoLog(<< "Unsupported transport type");
         return INVALID_SOCKET;
   }

   if( INVALID_SOCKET == fd )
   {
      int e = getErrno();
      InfoLog(<< "Failed to create socket: " << strerror(e));
   }

   return fd;
}

void
TestRtp::open()
{
   mFdRtp = openSocket(mLocalAddrRtp.getType());
   mFdRtcp = openSocket(mLocalAddrRtcp.getType());

   DebugLog(<< "Create RTP/RTCP socket fd: " << (unsigned int) mFdRtp << "/" << (unsigned int) mFdRtcp);

   mFdVideoRtp = openSocket(mLocalAddrVideoRtp.getType());
   mFdVideoRtcp = openSocket(mLocalAddrVideoRtcp.getType());
}

void
TestRtp::close()
{
   closeSocket(mFdRtp);
   closeSocket(mFdRtcp);
   closeSocket(mFdVideoRtp);
   closeSocket(mFdVideoRtcp);
}

Data
TestRtp::getName() const
{
  Data buffer;
  {
     DataStream strm(buffer);
     strm << "RTP end point " << mLocalAddrRtp;
  }
  return buffer;
}

bool
TestRtp::isRtpPacket(const Data& packet, UInt32 ssrc) const
{
   if( packet.size() < RTP_HEADER_SIZE )
      return false;

   RtpHeader& header = *((RtpHeader*)(packet.data()));

   if( ntohl(header.mSsrc) != ssrc )
      return false;

   return true;
}

bool
TestRtp::isRtcpPacket(const Data& packet, UInt32 ssrc) const
{
   if( packet.size() < RTCP_HEADER_SIZE )
      return false;

   RtcpHeader& header = *((RtcpHeader*)(packet.data()));

   if( ntohl(header.mSsrc) != ssrc )
      return false;

   switch( header.mPayloadType )
   {
      case RTCP_SR:
      case RTCP_RR:
      case RTCP_SDES:
      case RTCP_BYE:
      case RTCP_APP:
         return true;
   }

   return true;
}

UInt16 
TestRtp::getSeqNo(const Packet& packet)
{
   RtpHeader& header = *((RtpHeader*)(packet.mData.data()));
   return ntohs(header.mSequenceNumber);
}

void
TestRtp::RtpPacketInfo()
{
   UInt16 first = getSeqNo(mPacketsRtp.front());
   UInt16 last = getSeqNo(mPacketsRtp.back());

   int received = (int) mPacketsRtp.size();
   InfoLog(<< "Received packates: " << received);
   InfoLog( << "First: " << first << ", Last: " << last);

   if( received <= 0 )
      return;

   int expected = (last - first + 1 + 65536) % 65536;
   InfoLog(<< "Expected packets: " << expected);
   InfoLog(<< "Loss rate: " << 100.0f * (expected - received)/(float)expected << "%");

/*
   struct timeval firstTime =  mPacketsRtp.front().mTimeStamp;
   struct timeval diff;
   list<Packet>::iterator it;
   int i = 0;
   for( it = mPacketsRtp.begin(); it != mPacketsRtp.end(); it++ )
   {
      timeval_subtract(&diff, &it->mTimeStamp, &firstTime);
      InfoLog(<< "Packet: " << i++ << ", Time: " << diff.tv_sec << "." << diff.tv_usec);
   }
*/
}

void
TestRtp::loadStream(const Data& file, UInt32 ssrc)
{
#ifdef HAS_PCAP
   char errbuff[PCAP_ERRBUF_SIZE];

   pcap_t* p = pcap_open_offline(file.c_str(), errbuff);

   if( !p )
   {
      InfoLog(<< "Error reading PCAP file " << file << ": " << errbuff);
      return;
   }

   while( 1 )
   {
      struct pcap_pkthdr h;
      const u_char* pkt = pcap_next(p, &h);

      if( NULL == pkt )
         break;

      int len = h.len;

      if( len < OVERHEAD_SIZE )
         continue;

      Packet packet(h.ts, Data(pkt + OVERHEAD_SIZE, len - OVERHEAD_SIZE));

      if( isRtpPacket(packet.mData, ssrc) )
         mPacketsRtp.push_back(packet);
      else if( isRtcpPacket(packet.mData, ssrc) )
         mPacketsRtcp.push_back(packet);
   };

   pcap_close(p);

   InfoLog( << "RTP/RTCP packets read: " << mPacketsRtp.size() << "/" << mPacketsRtcp.size());

   RtpPacketInfo();

#else
   InfoLog(<< "Reading pcap files disabled");
   resip_assert(0);
#endif
}

int
TestRtp::bindSocket(resip::Socket fd, Tuple& addr)
{
   if( ::bind( fd, &addr.getMutableSockaddr(), addr.length()) == SOCKET_ERROR )
   {
      int e = getErrno();
      if ( e == EADDRINUSE )
         ErrLog (<< "Address already in use " << addr);
      else
         ErrLog (<< "Could not bind socket to " << addr);

      return -1;
   }

   bool ok = makeSocketNonBlocking(fd);
   if( !ok )
   {
      ErrLog (<< "Could not make socket non-blocking ");
      return -1;
   }

   return 0;
}

void
TestRtp::bind()
{
   bindSocket(mFdRtp, mLocalAddrRtp);
   bindSocket(mFdRtcp, mLocalAddrRtcp);

   DebugLog(<< "Binding RTP socket to " << mLocalAddrRtp);
   DebugLog(<< "Binding RTCP socket to " << mLocalAddrRtcp);

   bindSocket(mFdVideoRtp, mLocalAddrVideoRtp);
   bindSocket(mFdVideoRtcp, mLocalAddrVideoRtcp);

   DebugLog(<< "Binding RTP socket to " << mLocalAddrVideoRtp);
   DebugLog(<< "Binding RTCP socket to " << mLocalAddrVideoRtcp);
}

Data
TestRtp::recvPacket(resip::Socket fd, Tuple& addr)
{
   char* buffer = new char[MaxBufferSize];
   socklen_t slen = addr.length();
   int len = recvfrom(fd, buffer, MaxBufferSize, 0, &addr.getMutableSockaddr(), &slen);
   if ( len == SOCKET_ERROR )
   {
      int e = getErrno();
      InfoLog(<< "Socket read error: " << strerror(e));
   }

   if (len == 0 || len == SOCKET_ERROR)
   {
      delete[] buffer;
      buffer=0;
      return Data::Empty;
   }

   return Data(Data::Take, buffer, len);
}

bool
TestRtp::parsePacket(const Data& data)
{
   if( data.size() < RTP_HEADER_SIZE )
   {
      InfoLog(<< "Invalid RTP message received of size: " << (unsigned int) data.size());
      return false;
   }

   // !jv! for now, assume that RTP header is 12 bytes
   const RtpHeader& header = *((const RtpHeader*)data.data());

   UInt8 remoteCodec = header.mPayloadType & 0x7f;
   bool matchAudio = false;
   bool matchVideo = false;

   // simple matching of payload type. Given the incoming RTP packet payload type, search our local codecs in SDP
   // to see if there's a match and whether it's for audio or video.
   //
   for (list<Medium>::iterator it = mLocalSdp->session().media().begin(); it != mLocalSdp->session().media().end(); it++)
   {
      for( list<Codec>::const_iterator j = it->codecs().begin(); j != it->codecs().end(); j++ )
      {
         if (remoteCodec == j->payloadType())
         {
            if (it->name() == SdpHelper::AudioMediaType)
               matchAudio = true;
            else if (it->name() == SdpHelper::VideoMediaType)
               matchVideo = true;
         }
      }
   }

   if( !(matchAudio || matchVideo) )
   {
      InfoLog(<< "Invalid payload type received: " << remoteCodec << " of size " << (unsigned int) data.size());
      return false;
   }

   UInt32 remoteSsrc = ntohl(header.mSsrc);
   if( 0 == mRemoteSsrc )
   {
      InfoLog(<< "TestRtp::parsePacket():  RTP strem started");
//      handleEvent(new RtpEvent(this, Rtp_StreamStarted));
      mRemoteSsrc = remoteSsrc;
      mRemoteCodec = remoteCodec;
   }

   if( remoteSsrc != mRemoteSsrc )
   {
      InfoLog(<< "TestRtp::parsePacket(): SSRC changed to " << remoteSsrc);
//      handleEvent(new RtpEvent(this, Rtp_SsrcChanged));
      mRemoteSsrc = remoteSsrc;
   }

   if( mRemoteCodec !=  remoteCodec )
   {
//      handleEvent(new RtpEvent(this, Rtp_CodecChanged));
      mRemoteCodec = remoteCodec;
   }

   mReceiverStatistics.mReceived++;

   return true;
}

void
TestRtp::overrideSsrc(Data& packet)
{
   RtpHeader& header = *((RtpHeader*)packet.data());
   header.mSsrc = htonl(mLocalSsrc);
}

void
TestRtp::sendPacket(resip::Socket fd, Tuple& dest, const Data& data)
{
   size_t count = sendto(fd, data.data(), (int) data.size(), 0, &dest.getSockaddr(), dest.length());

   if( count == (size_t)SOCKET_ERROR )
   {
      int e = getErrno();
      ErrLog(<< "Failed to send packet to " << dest << ": " << strerror(e));
   }
   else
   {
      if( count != data.size() )
         ErrLog(<< "Buffer overflow while sending packet to " << dest);
   }

   mSenderStatistics.mSent++;
}

// ----------------------------------------------------------------------------
std::shared_ptr<SdpContents>
TestRtp::getLocalSdp() const
{
   return getLocalSdp(MEDIA_ACTIVE, MEDIA_ACTIVE);
}

// ----------------------------------------------------------------------------
std::shared_ptr<SdpContents>
TestRtp::getLocalSdp(unsigned long audioAttr, unsigned long videoAttr) const
{
   SdpContents * contents = dynamic_cast<SdpContents*>(mLocalSdp->clone());
   mLocalSdp->session().origin().getVersion() += 1;
   if (! mSessionName.empty())
      mLocalSdp->session().name() = mSessionName;
   std::shared_ptr<SdpContents> sdp(contents);

   SdpContents::AddrType addrType = mLocalAddrRtp.ipVersion() == V4 ? SdpContents::IP4 :  SdpContents::IP6;
   Data addr = Tuple::inet_ntop(mLocalAddrRtp);
   sdp->session().origin().setAddress(addr, addrType);

   SdpContents::Session::Connection connection(addrType, addr);
   if (mHold2543 && (audioAttr & MEDIA_HOLD))
      connection.setAddress("0.0.0.0");
   sdp->session().connection() = connection;

   int audioPort = mLocalAddrRtp.getPort();
   int offset = 0;

   // for each m-line in the session, set here:
   // * port
   // * media-direction
   //
   for (list<Medium>::iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      Medium & mline = *it;
      unsigned long mediaAttr = MEDIA_NONE;
      if (mline.name() == SdpHelper::AudioMediaType)
         mediaAttr = audioAttr;
      else if (mline.name() == SdpHelper::VideoMediaType)
         mediaAttr = videoAttr;
      else
         continue;

      // port
      if ((mediaAttr & MEDIA_DISABLE))
      {
         mline.port() = 0;
      }
      else
      {
         mline.port() = audioPort + offset;
         offset += 2;
      }

      // media-direction
      if (! mHold2543)
      {
         if ((mediaAttr & MEDIA_HOLD))
            mline.addAttribute("sendonly");
         else if ((mediaAttr & MEDIA_ACTIVE))
            mline.addAttribute("sendrecv");
         else if ((mediaAttr & MEDIA_INACTIVE))
            mline.addAttribute("inactive");
         else if ((mediaAttr & MEDIA_HOLD_PEER))
            mline.addAttribute("recvonly");
      }

      if (mPtime && (mline.name() == SdpHelper::AudioMediaType))
         mline.addAttribute("ptime:" + Data(mPtime));
   }

   return sdp;
}

void
TestRtp::setLocalAddr(const Uri& addr)
{
   int port = addr.port();
   Tuple t(addr.host(), port, (DnsUtil::isIpV6Address(addr.host()) ? V6 : V4), UDP);
   mLocalAddrRtp = t;
   t.setPort(port + 1);
   mLocalAddrRtcp = t;
   t.setPort(port + 2);
   mLocalAddrVideoRtp = t;
   t.setPort(port + 3);
   mLocalAddrVideoRtcp = t;
}

void
TestRtp::setLocalAddr(const Data& addr)
{
   int port =  mLocalAddrRtp.getPort();
   Tuple t(addr, port, (DnsUtil::isIpV6Address(addr) ? V6 : V4), UDP);
   mLocalAddrRtp = t;
   t.setPort(port + 1);
   mLocalAddrRtcp = t;
   t.setPort(port + 2);
   mLocalAddrVideoRtp = t;
   t.setPort(port + 3);
   mLocalAddrVideoRtcp = t;
}

void
TestRtp::addCodec(
   const resip::Data& mediaName,
   const resip::Data& codecName,
   int payload,
   int rate,
   const resip::Data& parameters,
   int port,
   const char * proto)
{
   if (mediaName.empty())
      return;

   if (! mLocalSdp)
      mLocalSdp = std::make_shared<SdpContents>();

   Medium * medium = 0;
   // find the m-line based on media-type: 'audio'/'video', etc. in the REVERSE direction.
   for (list<Medium>::reverse_iterator it = mLocalSdp->session().media().rbegin(); it != mLocalSdp->session().media().rend(); it++)
   {
      if (it->name() == mediaName)
         medium = &(*it);
   }

   if (! medium)
   {
      // doesn't exist, add medium and codec
      Medium tmpMedium(mediaName, port, 0, (proto ? proto : SdpHelper::RtpAvpProtocol));
      mLocalSdp->session().addMedium(tmpMedium);
      medium = &mLocalSdp->session().media().back();
   }

   if (mDescribeWellKnownCodec)
   {
      Codec codec(codecName, payload, rate);
      codec.parameters() = parameters;
      medium->codecs().push_back(codec);
   }
   else
   {
      Data strPayload = Data::from(payload);
      medium->addFormat(strPayload);
      // describe telephone-event anyhow
      if (codecName == "telephone-event")
      {
         Data strRTPMap;
         {
            DataStream s(strRTPMap);
            s <<strPayload <<' ' <<codecName <<'/' <<rate;
         }
         medium->addAttribute("rtpmap", strRTPMap);
         if (! parameters.empty())
            medium->addAttribute("fmtp", (strPayload + " " + parameters));
      }
   }
}


void
TestRtp::addControlMLine()
{
   Medium medium(SdpHelper::ControlMediaType, 0, 0, SdpHelper::TcpMcProtocol);
   mLocalSdp->session().addMedium(medium);
}

void
TestRtp::removeCodecs(const resip::Data& mediaName)
{
   if (mLocalSdp)
   {
      if (mediaName.empty())
      {
         mLocalSdp->session().media().clear();
      }
      else
      {
         for (list<Medium>::iterator it = mLocalSdp->session().media().begin(); it != mLocalSdp->session().media().end();)
         {
            list<Medium>::iterator prev = it++;
            if (prev->name() == mediaName)
               mLocalSdp->session().media().erase(prev);
         }
      }
   }
}

void
TestRtp::setRemoteAddr(const Tuple& addr)
{
   mRemoteAddrRtp = addr;
   mRemoteAddrRtcp = addr;
   int port = mRemoteAddrRtp.getPort();
   mRemoteAddrRtcp.setPort(port + 1);
}

void
TestRtp::enableSendingDelegate(bool enable)
{
   DebugLog(<< "enableSendingDelegate()");
   mSendPackets = enable;
}


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

resip::Data SdpHelper::AudioMediaType = "audio";
resip::Data SdpHelper::VideoMediaType = "video";
resip::Data SdpHelper::ControlMediaType = "control";

resip::Data SdpHelper::RtpAvpProtocol = "RTP/AVP";
resip::Data SdpHelper::TcpMcProtocol = "tcp mc";

SdpHelper::MediaDirection
SdpHelper::getMediaDirectionFromString(const char * szMediaName)
{
   if (szMediaName)
   {
      Data dir(Data::Share, szMediaName);
      if (dir == "sendonly")  return MD_SendOnly;
      else if (dir == "recvonly")  return MD_RecvOnly;
      else if (dir == "sendrecv")  return MD_SendRecv;
      else if (dir == "inactive")  return MD_Inactive;
   }
   return MD_None;
}

SdpHelper::MediaDirection
SdpHelper::getMediaDirection(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if (!sdp || !szMediaName)
      return MD_None;
   for (list<Medium>::const_iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      if (it->name() == szMediaName)
      {
         if (it->exists(Data(Data::Share, "inactive")))  return MD_Inactive;
         else if (it->exists(Data(Data::Share, "recvonly")))  return MD_RecvOnly;
         else if (it->exists(Data(Data::Share, "sendonly")))  return MD_SendOnly;
         else if (it->exists(Data(Data::Share, "sendrecv")))  return MD_SendRecv;
      }
   }
   return MD_None;
}


bool
SdpHelper::hasIce(std::shared_ptr<SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
      return false;

    list<SdpContents::Session::Medium> media = sdp->session().media();
   // only consider first media
   list<SdpContents::Session::Medium>::const_iterator it = media.begin();
   if( it == media.end() )
      return false;

   if( it->exists("alt") )
      return true;

   if( it->exists("candidate") )
      return true;
 
   return false;
}

Data 
SdpHelper::getConnectionAddr(std::shared_ptr<resip::SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
   {
      return "0.0.0.0";
   }

   return sdp->session().connection().getAddress();
}

int
SdpHelper::getPort(std::shared_ptr<resip::SipMessage> msg, unsigned int index)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
   {
      return -1;
   }

   list<SdpContents::Session::Medium> media = sdp->session().media();
   if( media.size() < index + 1 )
      return -1;

   list<SdpContents::Session::Medium>::iterator it = media.begin();
   for( unsigned int i = 0; i < index; it++, i++ );
   
   return it->port();
}

int
SdpHelper::getPort(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if (!sdp || !szMediaName)
      return -1;
   for (list<Medium>::const_iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      if (it->name() == szMediaName)
         return it->port();
   }
   return -1;
}

void
SdpHelper::setPort(std::shared_ptr<resip::SdpContents> sdp, const char * szMediaName, unsigned long port)
{
   if (!sdp || !szMediaName)
      return;
   for (list<Medium>::iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      if (it->name() == szMediaName)
      {
         it->port() = port;
         break;
      }
   }
}

void SdpHelper::addAttr(
   std::shared_ptr<resip::SdpContents> sdp,
   const char * szMediaName,
   const char* szAttrField,
   const char* szAttrValue)
{
   if (!sdp || !szMediaName || !szAttrField || !szAttrValue)
      return;
   for (list<Medium>::iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      if (it->name() == szMediaName)
      {
         it->addAttribute(szAttrField, szAttrValue);
         break;
      }
   }
}

bool
SdpHelper::isMediaInactive(std::shared_ptr<SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
      return false;

    list<SdpContents::Session::Medium> media = sdp->session().media();
   // only consider first media
   list<SdpContents::Session::Medium>::const_iterator it = media.begin();
   if( it == media.end() )
      return false;

   if( it->exists("inactive") )
      return true;

   return false;
}

bool
SdpHelper::isMediaSendOnly(std::shared_ptr<SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
      return false;

   list<SdpContents::Session::Medium> media = sdp->session().media();
   // only consider first media
   list<SdpContents::Session::Medium>::const_iterator it = media.begin();
   if( it == media.end() )
      return false;

   if( it->exists("sendonly") )
      return true;

   return false;
}

bool
SdpHelper::isMediaRecvOnly(std::shared_ptr<SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
      return false;

   list<SdpContents::Session::Medium> media = sdp->session().media();
   // only consider first media
   list<SdpContents::Session::Medium>::const_iterator it = media.begin();
   if( it == media.end() )
      return false;

   if( it->exists("recvonly") )
      return true;

   return false;
}

unsigned int
SdpHelper::getMediaCount(std::shared_ptr<resip::SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if (! sdp)
      return 0;
   return (unsigned int) sdp->session().media().size();
}

// m-line
unsigned int
SdpHelper::getMLineCount(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName)
{
   unsigned int ret = 0;
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if (! sdp)
      return 0;
   for (list<Medium>::const_iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      if (it->name() == szMediaName)
         ret++;
   }
   return ret;
}

unsigned int
SdpHelper::getCodecsCount(std::shared_ptr<resip::SipMessage> msg, const char * szMediaName)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if (! sdp)
      return 0;
   for (list<Medium>::const_iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      if (it->name() == szMediaName)
         return (unsigned int) it->codecs().size();
   }
   return 0;
}

bool
SdpHelper::hasPayloadNumber(std::shared_ptr<resip::SipMessage> msg, int payloadNumber)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if (! sdp)
      return false;
   for (list<Medium>::const_iterator it = sdp->session().media().begin(); it != sdp->session().media().end(); it++)
   {
      for( list<Codec>::const_iterator jIt = it->codecs().begin(); jIt != it->codecs().end(); jIt++ )
      {
         if (jIt->payloadType() == payloadNumber)
            return true;
      }
   }
   return false;
}

bool
SdpHelper::isHold2543(std::shared_ptr<SipMessage> msg)
{
   SdpContents* sdp = dynamic_cast<SdpContents*>(msg->getContents());
   if( NULL == sdp )
      return false;

   SdpContents::Session::Connection connection = sdp->session().connection();
   Data addr = connection.getAddress();
   return addr == "0.0.0.0";
}

void
TestRtp::thread()
{
   // Three types of modes:
   // 1. Return each packet to sender with modified SSRC
   // 2. When a packets comes in from sender, send a packet from file
   // 3. Send packet from file in regular interval. Currently, this method
   //    sends packets out according to timestamp found in Ethereal.
   //    The current solution consumes all CPU since it spins and checks
   //    the time periodically

   // time_t lastRtcp = time(NULL);
   struct timeval lastPacketTime;
   if( !mPacketsRtp.empty() )
      lastPacketTime = mPacketsRtp.front().mTimeStamp;
#ifdef USE_XS_TIME
   double lastSendingTime = XsLib::XsGetClockTickUs();
#elif !defined(WIN32)
   struct timeval lastSendingTime;
   gettimeofday(&lastSendingTime, 0);
#endif

   while( !isShutdown() )
   {
      FdSet fdSet;
      fdSet.setRead(mFdRtp);
      fdSet.setRead(mFdRtcp);

      /*int ready = */fdSet.selectMilliSeconds(RTP_INTERVAL);
 
      if( fdSet.readyToRead(mFdRtp) )
      {
         Tuple from;
         Data data = recvPacket(mFdRtp, from);
         if( 0 == mRemoteAddrRtp.getPort() )
         {
            InfoLog(<< "Updating RTP address changed to " << from);
            mRemoteAddrRtp = from;
         }
         if( parsePacket(data) )
         {
//            overrideSsrc(data);
//            sendPacket(mFdRtp, mRemoteAddrRtp, data);

            if( mSendPackets && !mPacketsRtp.empty() )
            {
               Packet packet = mPacketsRtp.front();
               mPacketsRtp.pop_front();
               sendPacket(mFdRtp, mRemoteAddrRtp, packet.mData);
               if( 0 == (mPacketsRtp.size() % 100) )
                  InfoLog(<< (unsigned int) mPacketsRtp.size() << " more packets to send");
            }
         }
      }

/*
      // rtcp
      if( fdSet.readyToRead(mFdRtcp) )
      {
         Tuple from;
         Data data = recvPacket(mFdRtcp, from);
         if( !(mRemoteAddrRtcp == from) )
         {
            InfoLog(<< "Remote RTCP address changed to " << from);
            mRemoteAddrRtcp = from;
         }
      }
*/

#if 0
      if( mSendPackets )
      {
         if( !mPacketsRtp.empty() )
         {
//               DebugLog(<< "Checking time to send RTP packet");

/*
#ifdef USE_XS_TIME
               double currentTime = XsLib::XsGetClockTickUs();
               double elapsedTime = currentTime - lastSendingTime;
#else
               struct timeval currentTime;
               gettimeofday(&currentTime, 0);
               struct timeval elapsedTime;
               timeval_subtract(&elapsedTime, &currentTime, &lastSendingTime);
#endif
  
               struct timeval currentPacketTime = mPacketsRtp.front().mTimeStamp;
               struct timeval packetDiffTime;
               timeval_subtract(&packetDiffTime, &currentPacketTime, &lastPacketTime);

//               InfoLog( << "Packet diff: " << packetDiffTime.tv_sec << "." << packetDiffTime.tv_usec);

#ifdef USE_XS_TIME
//               InfoLog(<< elapsedTime);
               bool send = (packetDiffTime.tv_sec * 1000000 + packetDiffTime.tv_usec) >= elapsedTime;
#else
               struct timeval diff;
               bool send = (timeval_subtract(&diff, &elapsedTime, &packetDiffTime) == 0);
//               InfoLog( << "Elapsed: " << elapsedTime.tv_sec << "." << elapsedTime.tv_usec);
#endif
*/
               bool send = true;
               if( send )
               {
//                  DebugLog(<< "Sending RTP packet");

                  Packet packet = mPacketsRtp.front();
                  mPacketsRtp.pop_front();
                  sendPacket(mFdRtp, mRemoteAddrRtp, packet.mData);

/*               
                  lastPacketTime = currentPacketTime;
                  lastSendingTime = currentTime;
*/
               }
         }

/*
         if( 0 != ready )
#ifdef WIN32
            Sleep(RTP_INTERVAL);
#else
            usleep(RTP_INTERVAL * 1000);
#endif
*/

/*
         if( !mPacketsRtcp.empty() )
         {
            if( time(NULL) > lastRtcp + RTCP_INTERVAL )
            {
               DebugLog(<< "Sending RTCP packet");
               Packet packet = mPacketsRtcp.front();
               mPacketsRtcp.pop_front();
               sendPacket(mFdRtcp, mRemoteAddrRtcp, packet.mData);
               lastRtcp = time(NULL);
            }
         }
*/
      }
#endif
   }
}

TestEndPoint::ExpectBase*
TestRtp::expect(RtpEvent::Type type,
                 int timeoutMs,
                 ActionBase* expectAction)
{
   InfoLog( << "TestRtp::expect()");
   return new Expect(*this,
                     new EventMatcherSpecific<RtpEvent>(type),
                     timeoutMs,
                     expectAction);
}

ActionBase*
TestRtp::enableSending(bool enable)
{
   DebugLog(<< "TestRtp::enableSending()");
   return new CommonAction(this,
      "Enable sending",
       std::bind(&TestRtp::enableSendingDelegate, this, enable));
}
