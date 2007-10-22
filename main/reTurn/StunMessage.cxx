#include "StunMessage.hxx"

#include <rutil/Timer.hxx>
#include <rutil/Random.hxx>
#include <rutil/DataStream.hxx>
#include <rutil/MD5Stream.hxx>
#include <boost/crc.hpp>

typedef boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0x5354554e, true, true> stun_crc_32_type;

#ifdef USE_SSL
#include <openssl/hmac.h>
#endif

static bool verbose = false;

using namespace std;
using namespace resip;

// What should we set these
static const Data USERNAME_KEY("stunServerUsernameKey");
static const Data PASSWORD_KEY("stunServerPasswordKey");

namespace reTurn {

bool operator<(const UInt128& lhs, const UInt128& rhs)
{
   if(lhs.longpart[0] != rhs.longpart[0])
   {
      return lhs.longpart[0] < rhs.longpart[0];
   }
   else if(lhs.longpart[1] != rhs.longpart[1])
   {
      return lhs.longpart[1] < rhs.longpart[1];
   }
   else if(lhs.longpart[2] != rhs.longpart[2])
   {
      return lhs.longpart[2] < rhs.longpart[2];
   }

   return lhs.longpart[3] < rhs.longpart[3];
}

bool operator==(const UInt128& lhs, const UInt128& rhs)
{
   return lhs.longpart[0] == rhs.longpart[0] &&
          lhs.longpart[1] == rhs.longpart[1] &&
          lhs.longpart[2] == rhs.longpart[2] &&
          lhs.longpart[3] == rhs.longpart[3];
}

StunMessage::StunMessage(const StunTuple& localTuple,
                         const StunTuple& remoteTuple,
                         char* buf, unsigned int bufLen,
                         asio::ip::udp::socket* alternatePortSocket,
                         asio::ip::udp::socket* alternateIpSocket,
                         asio::ip::udp::socket* alternateIpPortSocket) :
   mLocalTuple(localTuple),
   mRemoteTuple(remoteTuple),   
   mAlternatePortSocket(alternatePortSocket),
   mAlternateIpSocket(alternateIpSocket),
   mAlternateIpPortSocket(alternateIpPortSocket),
   mBuffer(buf, bufLen)  // !slg! copies buffer from Socket buffer assuming for now that StunMessages will persist past one request/response transaction
                         //       could make this more efficient by having the transports allocate buffer dynamically and pass ownership over
{
   init();
   mIsValid = stunParseMessage(buf, bufLen);
   if(mIsValid) cout << mHeader << endl;
}

StunMessage::StunMessage() :
   mAlternatePortSocket(0),
   mAlternateIpSocket(0),
   mAlternateIpPortSocket(0),
   mIsValid(true)
{
   init();
}

StunMessage::StunMessage(const StunMessage& from)
{
   *this = from;
}

StunMessage& 
StunMessage::operator=(const StunMessage& rhs)
{
   if (this != &rhs)
   {
      assert(false);
   }

   return *this;
}

StunMessage::~StunMessage()
{
   if(mErrorCode.reason) delete mErrorCode.reason;
   if(mUsername) delete mUsername;
   if(mPassword) delete mPassword;
   if(mRealm) delete mRealm;
   if(mNonce) delete mNonce;
   if(mServer) delete mServer;
   if(mTurnData) delete mTurnData;
}

void 
StunMessage::init()
{
   mHasMappedAddress = false;
   mHasResponseAddress = false;
   mHasChangeRequest = false;
   mHasSourceAddress = false;
   mHasChangedAddress = false;
   mHasUsername = false;
   mHasNonce = false;
   mHasRealm = false;
   mHasPassword = false;
   mHasMessageIntegrity = false;
   mHasErrorCode = false;
   mHasUnknownAttributes = false;
   mHasReflectedFrom = false;
   mHasXorMappedAddress = false;
   mHasFingerprint = false;
   mHasServer = false;
   mHasAlternateServer = false;
   mHasRefreshInterval = false;
   mHasSecondaryAddress = false;
   mHasTurnChannelNumber = false;
   mHasTurnLifetime = false;
   mHasTurnAlternateServer = false;
   mHasTurnMagicCookie = false;
   mHasTurnBandwidth = false;
   mHasTurnDestinationAddress = false;
   mHasTurnRemoteAddress = false;
   mHasTurnData = false;
   mHasTurnRelayAddress = false;
   mHasTurnRequestedPortProps = false;
   mHasTurnRequestedTransport = false;
   mHasTurnRequestedIp = false;
   mHasTurnConnectStat = false;
   mErrorCode.reason = 0;
   mUsername = 0;
   mPassword = 0;
   mRealm = 0;
   mNonce = 0;
   mServer = 0;
   mTurnData = 0;
}

void 
StunMessage::createHeader(UInt16 stunclass, UInt16 method)
{
   mClass = stunclass;
   mMethod = method;

   // Assign a tid
   mHeader.id.magicCookie = StunMagicCookie;
   Data random = Random::getCryptoRandom(12);
   memcpy(&mHeader.id.tid, random.data(), sizeof(mHeader.id.tid));
}

void 
StunMessage::setErrorCode(unsigned short errorCode, const char* reason)
{
   assert(errorCode >= 100 && errorCode <= 699);
   mHasErrorCode = true;
   mErrorCode.errorClass = errorCode / 100;
   mErrorCode.number = errorCode % 100;
   if(mErrorCode.reason)
   {
      *mErrorCode.reason = reason;
   }
   else
   {
      mErrorCode.reason = new Data(reason);
   }
}

void 
StunMessage::setUsername(const char* username)
{
   mHasUsername = true;
   if(mUsername)
   {
      *mUsername = username;
   }
   else
   {
      mUsername = new Data(username);
   }
}

void 
StunMessage::setPassword(const char* password)
{
   mHasPassword = true;
   if(mPassword)
   {
      *mPassword = password;
   }
   else
   {
      mPassword = new Data(password);
   }
}
 
void 
StunMessage::setRealm(const char* realm)
{
   mHasRealm = true;
   if(mRealm)
   {
      *mRealm = realm;
   }
   else
   {
      mRealm = new Data(realm);
   }
}

void 
StunMessage::setNonce(const char* nonce)
{
   mHasNonce = true;
   if(mNonce)
   {
      *mNonce = nonce;
   }
   else
   {
      mNonce = new Data(nonce);
   }
}
   
void 
StunMessage::setServer(const char* server)
{
   mHasServer = true;
   if(mServer)
   {
      *mServer = server;
   }
   else
   {
      mServer = new Data(server);
   }
}
 
void 
StunMessage::setTurnData(const char* data, unsigned int len)
{
   mHasTurnData = true;
   if(mTurnData)
   {
      mTurnData->clear();
      mTurnData->append(data, len);
   }
   else
   {
      mTurnData = new Data(data, len);
   }
}

void 
StunMessage::applyXorToAddress(const StunAtrAddress& in, StunAtrAddress& out)
{
   if(&in != &out) memcpy(&out, &in, sizeof(out));

   out.port = out.port^(StunMessage::StunMagicCookie>>16); // Xor with most significate 16 bits of magic cookie
   if(out.family == IPv6Family) 
   {
      for(int i = 0; i < 4; i++)
      {
         out.addr.ipv6.longpart[i] = out.addr.ipv6.longpart[i]^mHeader.magicCookieAndTid.longpart[i];
      }
   }
   else
   {
      out.addr.ipv4 = out.addr.ipv4^StunMessage::StunMagicCookie;
   }
}

bool 
StunMessage::stunParseAtrAddress( char* body, unsigned int hdrLen,  StunAtrAddress& result )
{
   if ( hdrLen != 8 )
   {
      // clog << "hdrLen wrong for Address" <<endl;
      return false;
   }
   body++;  // Skip pad
   result.family = *body++;

   UInt16 nport;
   memcpy(&nport, body, 2); body+=2;
   result.port = ntohs(nport);

   if (result.family == IPv4Family)
   {		
      UInt32 naddr;
      memcpy(&naddr, body, 4); body+=4;
      result.addr.ipv4 = ntohl(naddr);
      return true;
   }
   else if (result.family == IPv6Family)
   {
      UInt128 naddr;
      memcpy(&naddr, body, 32); body+=32;
      /*
      for(int i = 0; i < 4; i++)
      {
         result.addr.ipv6.longpart[i] = ntohl(naddr.longpart[i]);
      }*/
      return true;
   }
   else
   {
      clog << "bad address family: " << result.family << endl;
   }
	
   return false;
}

bool 
StunMessage::stunParseAtrRequestedPortProps( char* body, unsigned int hdrLen,  TurnAtrRequestedPortProps& result )
{
   if ( hdrLen != 4 )
   {
      return false;
   }
   body++;  // Skip pad
   result.props = *body++ & 0x3;

   UInt16 nport;
   memcpy(&nport, body, 2); body+=2;
   result.port = ntohs(nport);
	
   return true;
}

bool 
StunMessage::stunParseAtrUInt32( char* body, unsigned int hdrLen,  UInt32& result )
{
   if ( hdrLen != 4 )
   {
      return false;
   }
   else
   {
      memcpy(&result, body, 4);
      result = ntohl(result);
      return true;
   }
}

bool 
StunMessage::stunParseAtrError( char* body, unsigned int hdrLen,  StunAtrError& result )
{
   body+=2;  // skip pad
   result.errorClass = *body++;
   result.number = *body++;
		
   result.reason = new resip::Data(resip::Data::Share, body, hdrLen-4);
   return true;
}

bool 
StunMessage::stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown& result )
{
   if ( hdrLen >= sizeof(result) )
   {
      return false;
   }
   else
   {
      if (hdrLen % 4 != 0) return false;
      result.numAttributes = hdrLen / 4;
      for (int i=0; i<result.numAttributes; i++)
      {
         memcpy(&result.attrType[i], body, 2); body+=2;
         result.attrType[i] = ntohs(result.attrType[i]);
      }
      return true;
   }
}

bool 
StunMessage::stunParseAtrIntegrity( char* body, unsigned int hdrLen,  StunAtrIntegrity& result )
{
   if ( hdrLen != 20)
   {
      //clog << "MessageIntegrity must be 20 bytes" << endl;
      return false;
   }
   else
   {
      memcpy(&result.hash, body, hdrLen);
      return true;
   }
}



bool
StunMessage::stunParseMessage( char* buf, unsigned int bufLen)
{
   if (verbose) clog << "Received stun message: " << bufLen << " bytes" << endl;
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
      clog << "Bad message, bufLen=" << bufLen << endl;
      return false;
   }
	
   memcpy(&mHeader, buf, sizeof(StunMsgHdr));
   mHeader.msgType = ntohs(mHeader.msgType);
   mHeader.msgLength = ntohs(mHeader.msgLength);
	
   if (mHeader.msgLength + sizeof(StunMsgHdr) != bufLen)
   {
      clog << "Message header length doesn't match message size: " << mHeader.msgLength << " - " << bufLen << endl;
      return false;
   }

   // Check if class and method are valid
   mClass = mHeader.msgType & 0x0110;
   mMethod = mHeader.msgType & 0x000F;
	
   // Look for stun magic cookie
   mHasMagicCookie = mHeader.id.magicCookie == StunMagicCookie;
   if(!mHasMagicCookie && verbose)
   {
      clog << "stun magic cookie not found." << endl;
   }

   char* body = buf + sizeof(StunMsgHdr);
   unsigned int size = mHeader.msgLength;
	
   if (verbose) clog << "bytes after header = " << size << endl;
	
   while ( size > 0 )
   {
      // !jf! should check that there are enough bytes left in the buffer
		
      StunAtrHdr* attr = reinterpret_cast<StunAtrHdr*>(body);
		
      unsigned int attrLen = ntohs(attr->length);
      unsigned int attrLenPad = attrLen % 4 == 0 ? 0 : 4 - (attrLen % 4);  // attrLen may not be on 4 byte boundary, in which case we need to pad to 4 bytes when advancing to next attribute
      int atrType = ntohs(attr->type);
		
      if (verbose) clog << "Found attribute type=" << atrType << " length=" << attrLen << endl;
      if ( attrLen+attrLenPad+4 > size ) 
      {
         clog << "claims attribute is larger than size of message " <<"(attribute type="<<atrType<<")"<< endl;
         return false;
      }
		
      body += 4; // skip the length and type in attribute header
      size -= 4;

      switch ( atrType )
      {
         case MappedAddress:
            mHasMappedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mMappedAddress )== false )
            {
               clog << "problem parsing MappedAddress" << endl;
               return false;
            }
            if (verbose) clog << "MappedAddress = " << mMappedAddress << endl;
            break;  

         case ResponseAddress:  // deprecated
            mHasResponseAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mResponseAddress )== false )
            {
               if (verbose) clog << "problem parsing ResponseAddress" << endl;
               return false;
            }
            if (verbose) clog << "ResponseAddress = " << mResponseAddress << endl;
            break;  
				
         case ChangeRequest:  // deprecated
            mHasChangeRequest = true;
            if (stunParseAtrUInt32( body, attrLen, mChangeRequest) == false)
            {
               if (verbose) clog << "problem parsing ChangeRequest" << endl;
               return false;
            }
            if (verbose) clog << "ChangeRequest = " << mChangeRequest << endl;
            break;
				
         case SourceAddress:  // deprecated
            mHasSourceAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mSourceAddress )== false )
            {
               if (verbose) clog << "problem parsing SourceAddress" << endl;
               return false;
            }
            if (verbose) clog << "SourceAddress = " << mSourceAddress << endl;
            break;  
				
         case ChangedAddress:  // deprecated
            mHasChangedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mChangedAddress )== false )
            {
               if (verbose) clog << "problem parsing ChangedAddress" << endl;
               return false;
            }
            if (verbose) clog << "ChangedAddress = " << mChangedAddress << endl;
            break;  
				
         case Username: 
            mHasUsername = true;
            mUsername = new resip::Data(resip::Data::Share, body, attrLen);
            if (verbose) clog << "Username = " << *mUsername << endl;
            break;
				
         case Password: 
            mHasPassword = true;
            mPassword = new resip::Data(resip::Data::Share, body, attrLen);
            if (verbose) clog << "Password = " << *mPassword << endl;
            break;
				
         case MessageIntegrity:
            mHasMessageIntegrity = true;
            if (stunParseAtrIntegrity( body, attrLen, mMessageIntegrity) == false)
            {
               if (verbose) clog << "problem parsing MessageIntegrity" << endl;
               return false;
            }
            //if (verbose) clog << "MessageIntegrity = " << mMessageIntegrity.hash << endl;
            break;
				
         case ErrorCode:
            mHasErrorCode = true;
            if (stunParseAtrError(body, attrLen, mErrorCode) == false)
            {
               if (verbose) clog << "problem parsing ErrorCode" << endl;
               return false;
            }
            if (verbose) clog << "ErrorCode = " << (int(mErrorCode.errorClass) * 100 + int(mErrorCode.number)) 
                              << " (" << *mErrorCode.reason << ")" << endl;					
            break;
				
         case UnknownAttribute:
            mHasUnknownAttributes = true;
            if (stunParseAtrUnknown(body, attrLen, mUnknownAttributes) == false)
            {
               if (verbose) clog << "problem parsing UnknownAttribute" << endl;
               return false;
            }
            // TODO output
            break;
				
         case ReflectedFrom:  // deprecated
            mHasReflectedFrom = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mReflectedFrom ) == false )
            {
               if (verbose) clog << "problem parsing ReflectedFrom" << endl;
               return false;
            }
            if (verbose) clog << "ReflectedFrom = " << mReflectedFrom << endl;
            break;  
				
         case Realm: 
            mHasRealm = true;
            mRealm = new resip::Data(resip::Data::Share, body, attrLen);
            if (verbose) clog << "Realm = " << *mRealm << endl;
            break;

         case Nonce: 
            mHasNonce = true;
            mNonce = new resip::Data(resip::Data::Share, body, attrLen);
            if (verbose) clog << "Nonce = " << *mNonce << endl;
            break;

         case XorMappedAddress_old:
         case XorMappedAddress:
            mHasXorMappedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mXorMappedAddress ) == false )
            {
               if (verbose) clog << "problem parsing XorMappedAddress" << endl;
               return false;
            }
            if (verbose) clog << "XorMappedAddress = " << mXorMappedAddress << endl;
            break;  				

         case Fingerprint:
            mHasFingerprint = true;
            if (stunParseAtrUInt32( body, attrLen, mFingerprint) == false)
            {
               return false;
            }
            if (verbose) clog << "Fingerprint = " << mFingerprint << endl;
            break;

         case Server: 
            mHasServer = true;
            mServer = new resip::Data(resip::Data::Share, body, attrLen);
            if (verbose) clog << "Server = " << *mServer << endl;
            break;

         case AlternateServer:
            mHasAlternateServer = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mAlternateServer ) == false )
            {
               if (verbose) clog << "problem parsing AlternateServer" << endl;
               return false;
            }
            if (verbose) clog << "AlternateServer = " << mAlternateServer << endl;
            break;  

         case RefreshInterval:
            mHasRefreshInterval = true;
            if (stunParseAtrUInt32( body, attrLen, mRefreshInterval) == false)
            {
               return false;
            }
            if (verbose) clog << "Refresh interval = " << mRefreshInterval << endl;
            break;

         case SecondaryAddress:
            mHasSecondaryAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mSecondaryAddress ) == false )
            {
               if (verbose) clog << "problem parsing secondaryAddress" << endl;
               return false;
            }
            if (verbose) clog << "SecondaryAddress = " << mSecondaryAddress << endl;
            break;  

         // TURN attributes

         case TurnChannelNumber:
            mHasTurnChannelNumber = true;
            {
               UInt32 channelNumber;
               if(stunParseAtrUInt32( body, attrLen, channelNumber) == false)
               {
                  return false;
               }
               mTurnChannelNumber = (channelNumber & 0xFF00000) >> 24;
            }
            if (verbose) clog << "Turn ChannelNumber = " << (int)mTurnChannelNumber << endl;
            break;

         case TurnLifetime:
            mHasTurnLifetime = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnLifetime) == false)
            {
               return false;
            }
            if (verbose) clog << "Turn Lifetime = " << mTurnLifetime << endl;
            break;

         case TurnAlternateServer:
            mHasTurnAlternateServer = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnAlternateServer ) == false )
            {
               return false;
            }
            if (verbose) clog << "Turn Alternate Server = " << mTurnAlternateServer << endl;
            break;

         case TurnMagicCookie:
            mHasTurnMagicCookie = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnMagicCookie) == false)
            {
               return false;
            }
            if (verbose) clog << "TurnMagicCookie (deprecated) = " << mTurnMagicCookie << endl;
            break;

         case TurnBandwidth:
            mHasTurnBandwidth = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnBandwidth) == false)
            {
               return false;
            }
            if (verbose) clog << "Turn Bandwidth = " << mTurnBandwidth << endl;
            break;

         case TurnDestinationAddress:
            mHasTurnDestinationAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnDestinationAddress ) == false )
            {
               return false;
            }
            if (verbose) clog << "Turn Destination Address = " << mTurnDestinationAddress << endl;
            break;

         case TurnRemoteAddress:
            mHasTurnRemoteAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnRemoteAddress ) == false )
            {
               return false;
            }
            if (verbose) clog << "Turn Remote Address = " << mTurnRemoteAddress << endl;
            break;

         //overlay on parse, ownership is buffer parsed from
         case TurnData:
            mHasTurnData = true;
            mTurnData = new resip::Data(resip::Data::Share, body, attrLen);
            break;

         case TurnRelayAddress:
            mHasTurnRelayAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnRelayAddress ) == false )
            {
               return false;
            }
            if (verbose) clog << "Turn Relay Address = " << mTurnRelayAddress << endl;
            break;

         case TurnRequestedPortProps:
            mHasTurnRequestedPortProps = true;
            if (stunParseAtrRequestedPortProps( body, attrLen, mTurnRequestedPortProps) == false)
            {
               return false;
            }
            if (verbose) clog << "Turn Requested Port Props = " << (int)mTurnRequestedPortProps.props << ", port = " << mTurnRequestedPortProps.port << endl;
            break;

         case TurnRequestedTransport:
            mHasTurnRequestedTransport = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnRequestedTransport) == false)
            {
               return false;
            }
            if (verbose) clog << "Turn Requested Transport = " << mTurnRequestedTransport << endl;
            break;

         case TurnRequestedIp:
            mHasTurnRequestedIp = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnRequestedIp ) == false )
            {
               return false;
            }
            if (verbose) clog << "Turn Requested IP Address = " << mTurnRequestedIp << endl;
            break;

         case TurnConnectStat:
            mHasTurnConnectStat = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnConnectStat) == false)
            {
               return false;
            }
            if (verbose) clog << "Turn Connect Stat = " << mTurnConnectStat << endl;
            break;
					
         default:
            if (verbose) clog << "Unknown attribute: " << atrType << endl;
            if ( atrType <= 0x7FFF ) 
            {
               return false;
            }
      }
		
      body += attrLen+attrLenPad;
      size -= attrLen+attrLenPad;
   }
    
   return true;
}

ostream& 
operator<< ( ostream& strm, const UInt128& r )
{
   strm << int(r.longpart[0]);
   for ( int i=1; i<4; i++ )
   {
      strm << ':' << int(r.longpart[i]);
   }
    
   return strm;
}

ostream& 
operator<<( ostream& strm, const StunMessage::StunAtrAddress& addr)
{
   if(addr.family == StunMessage::IPv6Family)
   {
      strm << "[";

      for(int i =0; i < 4; i++)
      {
         UInt32 ippart = addr.addr.ipv6.longpart[i];
         strm << ((int)(ippart>>16)&0xFFFF) << ":";
         strm << ((int)(ippart>>0)&0xFFFF) << (i == 3 ? "" : ":");
      }

      strm << "]:" << addr.port;
   }
   else
   {
      UInt32 ip = addr.addr.ipv4;
      strm << ((int)(ip>>24)&0xFF) << ".";
      strm << ((int)(ip>>16)&0xFF) << ".";
      strm << ((int)(ip>> 8)&0xFF) << ".";
      strm << ((int)(ip>> 0)&0xFF) ;
   	
      strm << ":" << addr.port;
   }
	
   return strm;
}

ostream&
operator<<(ostream& os, const StunMessage::StunMsgHdr& h)
{
   os << "STUN ";
   bool outputMethod=true;
   
   switch(h.msgType & 0x0110)
   { 
   case StunMessage::StunClassRequest:
      os << "Request: ";
      break;
   case StunMessage::StunClassIndication:
      os << "Indication: ";
      outputMethod = false;
      switch (h.msgType & 0x000F) 
      {
      case StunMessage::TurnSendInd:
         os << "TurnSendInd";
         break;
      case StunMessage::TurnDataInd:
         os << "TurnDataInd";
         break;
      case StunMessage::TurnChannelConfirmationInd:
         os << "TurnChannelConfirmationInd";
         break;
      case StunMessage::TurnConnectStatusInd:
         os << "TurnConnectStatusInd";
         break;
      default:
         os << "Unknown ind method (" << int(h.msgType & 0x000F) << ")";
         break;
      }
      break;
   case StunMessage::StunClassSuccessResponse:
      os << "Success Response: ";
      break;
   case StunMessage::StunClassErrorResponse:
      os << "Error Response: ";
      break;
   default:
      os << "Unknown class (" << int(h.msgType & 0x0110) << "): ";
   }

   if(outputMethod)
   {
      switch (h.msgType & 0x000F) 
      {
      case StunMessage::BindRequest:
         os << "BindingRequest";
         break;
      case StunMessage::SharedSecretRequest:
         os << "SharedSecretRequest";
         break;
		case StunMessage::TurnAllocateRequest:
            os << "TurnAllocateRequest";
			break;
		case StunMessage::TurnListenPermissionRequest:
            os << "TurnListenPermissionRequest";
			break;
		case StunMessage::TurnConnectRequest:
            os << "TurnConnectRequest";
			break;
      default:
         os << "Unknown method (" << int(h.msgType & 0x000F) << ")";
         break;
      }
   }

    os << ", id ";

    os << std::hex;
    for (unsigned int i = 0; i < 4; i++) {
        os << static_cast<int>(h.magicCookieAndTid.longpart[i]);
    }
    os << std::dec;

    return os;
}

char* 
StunMessage::encode16(char* buf, UInt16 data)
{
   UInt16 ndata = htons(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt16));
   return buf + sizeof(UInt16);
}

char* 
StunMessage::encode32(char* buf, UInt32 data)
{
   UInt32 ndata = htonl(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt32));
   return buf + sizeof(UInt32);
}

char* 
StunMessage::encode(char* buf, const char* data, unsigned int length)
{
   memcpy(buf, data, length);
   return buf + length;
}

char* 
StunMessage::encodeTurnData(char *ptr, const resip::Data* td)
{
   UInt16 padsize = (UInt16)td->size() % 4 == 0 ? 0 : 4 - ((UInt16)td->size() % 4);

   ptr = encode16(ptr, TurnData);
   ptr = encode16(ptr, (UInt16)td->size());
   memcpy(ptr, td->data(), td->size());
   ptr += td->size();
   
   return ptr+padsize;
}

char*
StunMessage::encodeAtrUInt32(char* ptr, UInt16 type, UInt32 value)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, value);
   return ptr;
}

char* 
StunMessage::encodeAtrAddress(char* ptr, UInt16 type, const StunAtrAddress& atr)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, atr.family == IPv6Family ? 20 : 8);
   *ptr++ = (UInt8)0;  // pad
   *ptr++ = atr.family;
   ptr = encode16(ptr, atr.port);
   if(atr.family == IPv6Family)
   {
      memcpy(ptr, &atr.addr.ipv6, sizeof(atr.addr.ipv6));
      ptr += sizeof(atr.addr.ipv6);
      /*
      for(int i = 0; i < 4; i++)
      {
         ptr = encode32(ptr, atr.addr.ipv6.longpart[i]);
      }
      */
   }
   else
   {
      ptr = encode32(ptr, atr.addr.ipv4);
   }
	
   return ptr;
}

char* 
StunMessage::encodeAtrError(char* ptr, const StunAtrError& atr)
{
   assert(atr.reason);
   UInt16 padsize = (unsigned int)atr.reason->size() % 4 == 0 ? 0 : 4 - ((unsigned int)atr.reason->size() % 4);

   ptr = encode16(ptr, ErrorCode);
   ptr = encode16(ptr, 4 + (UInt16)atr.reason->size() + padsize);  // The attribute length should be the real length - but this confuses RFC3489 implementations
   ptr = encode16(ptr, 0); // pad
   *ptr++ = atr.errorClass;
   *ptr++ = atr.number;
   ptr = encode(ptr, atr.reason->data(), (unsigned int)atr.reason->size());
   memset(ptr, 0, padsize);
   return ptr+padsize;
}

char* 
StunMessage::encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr)
{
   ptr = encode16(ptr, UnknownAttribute);
   ptr = encode16(ptr, 2+2*atr.numAttributes);
   for (int i=0; i<atr.numAttributes; i++)
   {
      ptr = encode16(ptr, atr.attrType[i]);
   }
   return ptr;
}

char* 
StunMessage::encodeAtrString(char* ptr, UInt16 type, const Data* atr)
{
   assert(atr);
   UInt16 padsize = (UInt16)atr->size() % 4 == 0 ? 0 : 4 - ((UInt16)atr->size() % 4);
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, (UInt16)atr->size()+padsize);  // The attribute length should be the real length - but this confuses RFC3489 implementations
   ptr = encode(ptr, atr->data(), (unsigned int)atr->size());
   memset(ptr, 0, padsize);
   return ptr + padsize;
}

char* 
StunMessage::encodeAtrIntegrity(char* ptr, const StunAtrIntegrity& atr)
{
   ptr = encode16(ptr, MessageIntegrity);
   ptr = encode16(ptr, 20);
   ptr = encode(ptr, atr.hash, sizeof(atr.hash));
   return ptr;
}

char* 
StunMessage::encodeAtrRequestedPortProps(char* ptr, const TurnAtrRequestedPortProps& atr)
{
   ptr = encode16(ptr, TurnRequestedPortProps);
   ptr = encode16(ptr, 4);
   *ptr++ = (UInt8)0;  // pad
   *ptr++ = atr.props & 0x3;
   ptr = encode16(ptr, atr.port);
   return ptr;
}

unsigned int
StunMessage::stunEncodeMessage(char* buf, unsigned int bufLen) 
{
   assert(bufLen >= sizeof(StunMsgHdr));
   char* ptr = buf;

   if (verbose) clog << "Encoding stun message: " << endl;

   mHeader.msgType = mClass | mMethod;

   ptr = encode16(ptr, mHeader.msgType);
   char* lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode(ptr, reinterpret_cast<const char*>(&mHeader.id), sizeof(mHeader.id));

   if (mHasTurnMagicCookie)
   {
      if (verbose) clog << "Encoding TurnMagicCookie: " << mTurnMagicCookie << endl;
      ptr = encodeAtrUInt32(ptr, TurnMagicCookie, mTurnMagicCookie);
   }

   if (mHasMappedAddress)
   {
      if (verbose) clog << "Encoding MappedAddress: " << mMappedAddress << endl;
      ptr = encodeAtrAddress (ptr, MappedAddress, mMappedAddress);
   }
   if (mHasResponseAddress)
   {
      if (verbose) clog << "Encoding ResponseAddress: " << mResponseAddress << endl;
      ptr = encodeAtrAddress(ptr, ResponseAddress, mResponseAddress);
   }
   if (mHasChangeRequest)
   {
      if (verbose) clog << "Encoding ChangeRequest: " << mChangeRequest << endl;
      ptr = encodeAtrUInt32(ptr, ChangeRequest, mChangeRequest);
   }
   if (mHasSourceAddress)
   {
      if (verbose) clog << "Encoding SourceAddress: " << mSourceAddress << endl;
      ptr = encodeAtrAddress(ptr, SourceAddress, mSourceAddress);
   }
   if (mHasChangedAddress)
   {
      if (verbose) clog << "Encoding ChangedAddress: " << mChangedAddress << endl;
      ptr = encodeAtrAddress(ptr, ChangedAddress, mChangedAddress);
   }
   if (mHasUsername)
   {
      if (verbose) clog << "Encoding Username: " << *mUsername << endl;
      ptr = encodeAtrString(ptr, Username, mUsername);
   }
   if (mHasPassword)
   {
      if (verbose) clog << "Encoding Password: " << *mPassword << endl;
      ptr = encodeAtrString(ptr, Password, mPassword);
   }
   if (mHasErrorCode)
   {
      if (verbose) clog << "Encoding ErrorCode: class=" 
			<< int(mErrorCode.errorClass)  
			<< " number=" << int(mErrorCode.number) 
			<< " reason=" 
			<< *mErrorCode.reason 
			<< endl;
		
      ptr = encodeAtrError(ptr, mErrorCode);
   }
   if (mHasUnknownAttributes)
   {
      if (verbose) clog << "Encoding UnknownAttribute: ???" << endl;
      ptr = encodeAtrUnknown(ptr, mUnknownAttributes);
   }
   if (mHasReflectedFrom)
   {
      if (verbose) clog << "Encoding ReflectedFrom: " << mReflectedFrom << endl;
      ptr = encodeAtrAddress(ptr, ReflectedFrom, mReflectedFrom);
   }
   if (mHasRealm)
   {
      if (verbose) clog << "Encoding Realm: " << *mRealm << endl;
      ptr = encodeAtrString(ptr, Realm, mRealm);
   }
   if (mHasNonce)
   {
      if (verbose) clog << "Encoding Nonce: " << *mNonce << endl;
      ptr = encodeAtrString(ptr, Nonce, mNonce);
   }
   if (mHasXorMappedAddress)
   {
      if (verbose) clog << "Encoding XorMappedAddress: " << mXorMappedAddress << endl;
      ptr = encodeAtrAddress(ptr, XorMappedAddress, mXorMappedAddress);
   }
   if (mHasServer)
   {
      if (verbose) clog << "Encoding Server: " << *mServer << endl;
      ptr = encodeAtrString(ptr, Server, mServer);
   }
   if (mHasAlternateServer)
   {
      if (verbose) clog << "Encoding Alternate Server: " << mAlternateServer << endl;
      ptr = encodeAtrAddress(ptr, AlternateServer, mAlternateServer);
   }
   if (mHasRefreshInterval)
   {
      if (verbose) clog << "Encoding Refresh Interval: " << mRefreshInterval << endl;
      ptr = encodeAtrUInt32(ptr, RefreshInterval, mRefreshInterval);
   }
   if (mHasSecondaryAddress)
   {
      if (verbose) clog << "Encoding SecondaryAddress: " << mSecondaryAddress << endl;
      ptr = encodeAtrAddress(ptr, SecondaryAddress, mSecondaryAddress);
   }

   if (mHasTurnChannelNumber)
   {
      if (verbose) clog << "Encoding Turn ChannelNumber: " << (int)mTurnChannelNumber << endl;
      ptr = encodeAtrUInt32(ptr, TurnChannelNumber, UInt32(mTurnChannelNumber << 24));
   }
   if (mHasTurnLifetime)
   {
      if (verbose) clog << "Encoding Turn Lifetime: " << mTurnLifetime << endl;
      ptr = encodeAtrUInt32(ptr, TurnLifetime, mTurnLifetime);
   }
   if (mHasTurnAlternateServer)
   {
      if (verbose) clog << "Encoding Turn AlternateServer: " << mTurnAlternateServer << endl;
      ptr = encodeAtrAddress(ptr, TurnAlternateServer, mTurnAlternateServer);
   }
   if (mHasTurnBandwidth)
   {
      if (verbose) clog << "Encoding Turn Bandwidth: " << mTurnBandwidth << endl;
      ptr = encodeAtrUInt32(ptr, TurnBandwidth, mTurnBandwidth);
   }   
   if (mHasTurnDestinationAddress)
   {
      if (verbose) clog << "Encoding Turn DestinationAddress: " << mTurnDestinationAddress << endl;
      ptr = encodeAtrAddress (ptr, TurnDestinationAddress, mTurnDestinationAddress);
   }
   if (mHasTurnRemoteAddress)
   {
      if (verbose) clog << "Encoding Turn RemoteAddress: " << mTurnRemoteAddress << endl;
      ptr = encodeAtrAddress (ptr, TurnRemoteAddress, mTurnRemoteAddress);
   }
   if (mHasTurnData)
   {
      if (verbose) clog << "Encoding TurnData (not shown)" << endl;
      ptr = encodeTurnData (ptr, mTurnData);
   }
   if (mHasTurnRelayAddress)
   {
      if (verbose) clog << "Encoding Turn RelayAddress: " << mTurnRelayAddress << endl;
      ptr = encodeAtrAddress (ptr, TurnRelayAddress, mTurnRelayAddress);
   }
   if (mHasTurnRequestedPortProps)
   {
      if (verbose) clog << "Encoding Turn RequestedPortProps: " << (int)mTurnRequestedPortProps.props << ", port=" << mTurnRequestedPortProps.port << endl;
      ptr = encodeAtrRequestedPortProps(ptr, mTurnRequestedPortProps);
   }   
   if (mHasTurnRequestedTransport)
   {
      if (verbose) clog << "Encoding Turn RequestedTransport: " << mTurnRequestedTransport << endl;
      ptr = encodeAtrUInt32(ptr, TurnRequestedTransport, mTurnRequestedTransport);
   }   
   if (mHasTurnRequestedIp)
   {
      if (verbose) clog << "Encoding Turn RequestedIp: " << mTurnRequestedIp << endl;
      ptr = encodeAtrAddress (ptr, TurnRequestedIp, mTurnRequestedIp);
   }
   if (mHasTurnConnectStat)
   {
      if (verbose) clog << "Encoding Turn Connect Stat: " << mTurnConnectStat << endl;
      ptr = encodeAtrUInt32(ptr, TurnConnectStat, mTurnConnectStat);
   }   

   // Update Length in header now - needed in message integrity and fingerprint calculations
   UInt16 msgSize = ptr - buf - sizeof(StunMsgHdr);
   if(mHasMessageIntegrity) msgSize += 24;  // 20 (attribute value) + 4 (attribute header)
   if(mHasFingerprint) msgSize += 8;        // 4 (attribute value) + 4 (attribute header)
   encode16(lengthp, msgSize);

   if (mHasMessageIntegrity)
   {
      if (verbose) clog << "HMAC with key: " << mHmacKey << endl;

      // allocate space for message integrity attribute (hash + attribute type + size)
      char* ptrMessageIntegrity = ptr;
	   ptr += 24;  // 20 (attribute value) + 4 (attribute header)
   
      StunAtrIntegrity integrity;
      // pad with zeros prior to calculating message integrity attribute	   
      int padding = 0;
      int len = ptrMessageIntegrity - buf;
      if (len % 64)
      {
         padding = 64 - (len % 64);
         memset(ptrMessageIntegrity, 0, padding);
      }
      //cout << "**** Calculating SHA1 for buffer of size: " << len+padding << endl;
      computeHmac(integrity.hash, buf, len + padding, mHmacKey.c_str(), (int)mHmacKey.size());
	   ptr = encodeAtrIntegrity(ptrMessageIntegrity, integrity);
   }

   // add finger print if required
   if (mHasFingerprint)
   {
      //cout << "***** Calculating fingerprint for data of size " << msgSize - 8 << endl;
      stun_crc_32_type stun_crc;
      stun_crc.process_bytes(buf, ptr-buf); // Calculate CRC across entire message, except the fingerprint attribute
      UInt32 fingerprint = stun_crc.checksum();
      ptr = encodeAtrUInt32(ptr, Fingerprint, fingerprint);
   }

   if (verbose) clog << endl;
   return int(ptr - buf);
}

unsigned int
StunMessage::stunEncodeFramedMessage(char* buf, unsigned int bufLen)
{
   unsigned short size = (unsigned short)stunEncodeMessage(&buf[4], bufLen-4);

   // Add Frame Header info
   buf[0] = 0; // Channel 0 for Stun Messages
   buf[1] = 0; // reserved
   memcpy(&buf[2], (void*)&size, 2);  // size is not needed if udp - but should be harmless
   return size+4;
}

#ifndef USE_SSL
void
StunMessage::computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   strncpy(hmac,"hmac-not-implemented",20);
}
#else
void
StunMessage::computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   unsigned int resultSize=20;
   HMAC(EVP_sha1(), 
        key, sizeKey, 
        reinterpret_cast<const unsigned char*>(input), length, 
        reinterpret_cast<unsigned char*>(hmac), &resultSize);
   assert(resultSize == 20);
}
#endif

void
StunMessage::createUsernameAndPassword()
{
   UInt64 time = resip::Timer::getTimeSecs();
   time -= (time % 20*60);  // rounded time - current time modulo 20 minutes
   //UInt64 hitime = time >> 32;
   //UInt64 lotime = time & 0xFFFFFFFF;

   mHasUsername = true;
   if(!mUsername)
   {
      mUsername = new Data;
   }
   assert(mUsername);

   if(mRemoteTuple.getAddress().is_v6())
   {
      *mUsername = Data(mRemoteTuple.getAddress().to_v6().to_bytes().c_array(), mRemoteTuple.getAddress().to_v6().to_bytes().size()).base64encode() + ":";
   }
   else
   {
      *mUsername = Data(mRemoteTuple.getAddress().to_v4().to_bytes().c_array(), mRemoteTuple.getAddress().to_v4().to_bytes().size()).base64encode() + ":";
   }
   unsigned int port = mRemoteTuple.getPort();
   *mUsername += Data((char*)&port, sizeof(unsigned int)).base64encode() + ":"; 
   *mUsername += resip::Random::getCryptoRandomHex(8) + ":";   // 8 bytes, 64 bits of randomness
   *mUsername += Data((char*)&time, sizeof(time)).hex() + ":";
   char hmac[20];
   computeHmac(hmac, mUsername->data(), (int)mUsername->size(), USERNAME_KEY.data(), (int)USERNAME_KEY.size());
   *mUsername += Data(hmac, sizeof(hmac)).hex();
	
   assert( mUsername->size()%4 == 0 );
   	
   if (verbose) clog << "computed username=" << *mUsername << endl;

   // Compute Password
   mHasPassword = true;

   if(!mPassword)
   {
      mPassword = new Data;
   }
   assert(mPassword);
   generateShortTermPasswordForUsername(*mPassword);

   if (verbose) clog << "computed password=" << *mPassword << endl;
}
 
void
StunMessage::generateShortTermPasswordForUsername(Data& password)
{
   char hmac[20];
   assert(mHasUsername && mUsername);
   computeHmac(hmac, mUsername->data(), (int)mUsername->size(), PASSWORD_KEY.data(), (int)PASSWORD_KEY.size());
   password = Data(hmac, sizeof(hmac)).hex();
}

void
StunMessage::getAddressFromUsername(asio::ip::address& address, unsigned int& port)
{
   assert(mHasUsername);
   assert(mUsername && mUsername->size() >= 92);
   assert(mUsername->size() == 92 || mUsername->size() == 108);

   if(mUsername->size() > 92)  // if over a certain size, then contains IPv6 address
   {
      Data addressPart(Data::Share, mUsername->data(), 24); 
      addressPart = addressPart.base64decode();
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.c_array(), addressPart.data(), bytes.size());
      asio::ip::address_v6 addressv6(bytes);
      address = addressv6;

      Data portPart(Data::Share, mUsername->data()+25, 4);
      portPart = portPart.base64decode();
      memcpy(&port, portPart.data(), sizeof(port));
   }
   else
   {
      Data addressPart(Data::Share, mUsername->data(), 8);  
      addressPart = addressPart.base64decode();
      asio::ip::address_v4::bytes_type bytes;
      memcpy(bytes.c_array(), addressPart.data(), bytes.size());
      asio::ip::address_v4 addressv4(bytes);
      address = addressv4;     

      Data portPart(Data::Share, mUsername->data()+9, 4);
      portPart = portPart.base64decode();
      memcpy(&port, portPart.data(), sizeof(port));
   }
}

void
StunMessage::calculateHmacKey(Data& hmacKey, const Data& longtermAuthenticationPassword)
{
   assert(mHasUsername);

   if(mHasRealm)  // Longterm authenicationmode
   {
      MD5Stream r;
      // remove quotes from username and realm
      Data username(*mUsername);
      Data realm(*mRealm);
      username.replace("\"", "");
      realm.replace("\"", "");
      r << username << ":" << realm << ":" << longtermAuthenticationPassword;
      hmacKey = r.getHex();
   }
   else
   {
      generateShortTermPasswordForUsername(hmacKey);
   }
}

bool 
StunMessage::checkMessageIntegrity(const Data& hmacKey)
{
   if(mHasMessageIntegrity)
   {
      unsigned char hmac[20];
#ifdef USE_SSL
      unsigned int hmacSize=20;

      // pad with zeros prior to calculating message integrity attribute	   
      int len = (int)mBuffer.size()-20-4-(mHasFingerprint?8:0);  // remove last TLV (Message Integrity TLV) from HMAC calculation and Fingerprint TLV (if present)
      int padding = len % 64 == 0 ? 0 : 64 - (len % 64);
      Data buffer(mBuffer.data(), len+padding);  // .slg. this creates a temp copy of the buffer so that we can pad it
      memset((void*)(buffer.data() + len), 0, padding);  // Zero out padding area 

      //cout << "**** Calculating SHA1 for buffer of size: " << (unsigned int)buffer.size() << endl;
      HMAC(EVP_sha1(), 
         hmacKey.c_str(), 
         (unsigned int)hmacKey.size(), 
         reinterpret_cast<const unsigned char*>(buffer.data()), 
         buffer.size(),  
         hmac, 
         &hmacSize);
      assert(hmacSize == 20);
#endif

      if (memcmp(mMessageIntegrity.hash, hmac, 20) == 0)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   else
   {
      // No message integrity attribute present - return true
      return true;
   }
}

bool 
StunMessage::checkFingerprint()
{
   if(mHasFingerprint)
   {
      //cout << "***** Calculating fingerprint for data of size " << mBuffer.size() - 8 << endl;
      stun_crc_32_type stun_crc;
      stun_crc.process_bytes(mBuffer.data(), mBuffer.size()-8); // Calculate CRC across entire message, except the fingerprint attribute
      if(stun_crc.checksum() == mFingerprint)
      {
         return true;
      }
      else
      {
         return false;
      }
   }
   return true;
}

}


/* ====================================================================

 Original contribution Copyright (C) 2007 Plantronics, Inc.
 Provided under the terms of the Vovida Software License, Version 2.0.

 The Vovida Software License, Version 2.0 
 
 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:
 
 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in
    the documentation and/or other materials provided with the
    distribution. 
 
 THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 DAMAGE.

 ==================================================================== */

