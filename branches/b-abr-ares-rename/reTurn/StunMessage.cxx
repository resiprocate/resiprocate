#include "StunMessage.hxx"

#include <rutil/Timer.hxx>
#include <rutil/Random.hxx>
#include <rutil/DataStream.hxx>
#include <rutil/MD5Stream.hxx>
#include <boost/crc.hpp>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

typedef boost::crc_optimal<32, 0x04C11DB7, 0xFFFFFFFF, 0x5354554e, true, true> stun_crc_32_type;

#ifdef USE_SSL
#include <openssl/hmac.h>
#endif

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
                         char* buf, unsigned int bufLen) :
   mLocalTuple(localTuple),
   mRemoteTuple(remoteTuple),   
   mBuffer(buf, bufLen)  // !slg! copies buffer from Socket buffer assuming for now that StunMessages will persist past one request/response transaction
                         //       could make this more efficient by having the transports allocate buffer dynamically and pass ownership over
{
   init();
   mIsValid = stunParseMessage(buf, bufLen);
  
   if(mIsValid)
   {
      DebugLog(<< "Successfully parsed StunMessage: " << mHeader);
   }
}

StunMessage::StunMessage() :
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
   mHasTurnPeerAddress = false;
   mHasTurnData = false;
   mHasTurnRelayAddress = false;
   mHasTurnRequestedProps = false;
   mHasTurnRequestedTransport = false;
   mHasTurnReservationToken = false;
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

void 
StunMessage::setStunAtrAddressFromTuple(StunAtrAddress& address, const StunTuple& tuple)
{
   address.port = tuple.getPort();
   if(tuple.getAddress().is_v6())
   {
      address.family = StunMessage::IPv6Family;  
      memcpy(&address.addr.ipv6, tuple.getAddress().to_v6().to_bytes().c_array(), sizeof(address.addr.ipv6));
   }
   else
   {
      address.family = StunMessage::IPv4Family;  
      address.addr.ipv4 = tuple.getAddress().to_v4().to_ulong();   
   }
}

void 
StunMessage::setTupleFromStunAtrAddress(StunTuple& tuple, const StunAtrAddress& address)
{
   tuple.setPort(address.port);
   if(address.family == StunMessage::IPv6Family)
   {
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.c_array(), &address.addr.ipv6, bytes.size());
      asio::ip::address_v6 addr(bytes);
      tuple.setAddress(addr);
   }
   else
   {
      asio::ip::address_v4 addr(address.addr.ipv4);
      tuple.setAddress(addr);
   }
}

bool
StunMessage::stunParseAtrXorAddress( char* body, unsigned int hdrLen, StunAtrAddress& result )
{
   bool ret = stunParseAtrAddress(body, hdrLen, result);
   if(ret)
   {
      applyXorToAddress(result, result);
   }
   return ret;
}

bool 
StunMessage::stunParseAtrAddress( char* body, unsigned int hdrLen, StunAtrAddress& result )
{
   if ( hdrLen != 8 )
   {
      WarningLog(<< "hdrLen wrong for Address");
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
      WarningLog(<< "bad address family: " << result.family);
   }
	
   return false;
}

bool 
StunMessage::stunParseAtrRequestedProps( char* body, unsigned int hdrLen,  TurnAtrRequestedProps& result )
{
   if ( hdrLen != 4 )
   {
      WarningLog(<< "hdrLen wrong for RequestedProps");
      return false;
   }
   result.propType = *body;  // copy first 8 bits into propType
	
   return true;
}

bool 
StunMessage::stunParseAtrUInt32( char* body, unsigned int hdrLen,  UInt32& result )
{
   if ( hdrLen != 4 )
   {
      WarningLog(<< "hdrLen wrong for UInt32 attribute");
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
StunMessage::stunParseAtrUInt64( char* body, unsigned int hdrLen,  UInt64& result )
{
   if ( hdrLen != 8 )
   {
      WarningLog(<< "hdrLen wrong for UInt64 attribute");
      return false;
   }
   else
   {
      memcpy(&result, body, 8);
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
      WarningLog(<< "hdrLen wrong for Unknown attribute");
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
      WarningLog(<< "hdrLen wrong for message integrity");
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
   DebugLog(<< "Received stun message: " << bufLen << " bytes");
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
      WarningLog(<< "Bad message, bufLen=" << bufLen);
      return false;
   }
	
   memcpy(&mHeader, buf, sizeof(StunMsgHdr));
   mHeader.msgType = ntohs(mHeader.msgType);
   mHeader.msgLength = ntohs(mHeader.msgLength);
	
   if (mHeader.msgLength + sizeof(StunMsgHdr) != bufLen)
   {
      WarningLog(<< "Message header length doesn't match message size: " << mHeader.msgLength << " - " << bufLen);
      return false;
   }

   // Check if class and method are valid
   mClass = mHeader.msgType & 0x0110;
   mMethod = mHeader.msgType & 0x000F;
	
   // Look for stun magic cookie
   mHasMagicCookie = mHeader.id.magicCookie == StunMagicCookie;
   if(!mHasMagicCookie)
   {
      StackLog(<< "stun magic cookie not found.");
   }

   char* body = buf + sizeof(StunMsgHdr);
   unsigned int size = mHeader.msgLength;
	
   StackLog(<< "bytes after header = " << size);
	
   while ( size > 0 )
   {
      // !jf! should check that there are enough bytes left in the buffer
		
      StunAtrHdr* attr = reinterpret_cast<StunAtrHdr*>(body);
		
      unsigned int attrLen = ntohs(attr->length);
      // attrLen may not be on 4 byte boundary, in which case we need to pad to 4 bytes when advancing to next attribute
      // Note:  Later RFC3489bis documents re-instated that all stun attributes must be on the 4 byte boundary, so this
      //        attrLenPad is not-really necessary.  Leaving it here for now, since it doesn't hurt.
      unsigned int attrLenPad = attrLen % 4 == 0 ? 0 : 4 - (attrLen % 4);  
      int atrType = ntohs(attr->type);
		
      StackLog(<< "Found attribute type=" << atrType << " length=" << attrLen);
      if ( attrLen+attrLenPad+4 > size ) 
      {
         WarningLog(<< "claims attribute is larger than size of message " <<"(attribute type="<<atrType<<")");
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
               WarningLog(<< "problem parsing MappedAddress");
               return false;
            }
            StackLog(<< "MappedAddress = " << mMappedAddress);
            break;  

         case ResponseAddress:  // deprecated
            mHasResponseAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mResponseAddress )== false )
            {
               WarningLog(<< "problem parsing ResponseAddress");
               return false;
            }
            StackLog(<< "ResponseAddress = " << mResponseAddress);
            break;  
				
         case ChangeRequest:  // deprecated
            mHasChangeRequest = true;
            if (stunParseAtrUInt32( body, attrLen, mChangeRequest) == false)
            {
               WarningLog(<< "problem parsing ChangeRequest");
               return false;
            }
            StackLog(<< "ChangeRequest = " << mChangeRequest);
            break;
				
         case SourceAddress:  // deprecated
            mHasSourceAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mSourceAddress )== false )
            {
               WarningLog(<< "problem parsing SourceAddress");
               return false;
            }
            StackLog(<< "SourceAddress = " << mSourceAddress);
            break;  
				
         case ChangedAddress:  // deprecated
            mHasChangedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mChangedAddress )== false )
            {
               WarningLog(<< "problem parsing ChangedAddress");
               return false;
            }
            StackLog(<< "ChangedAddress = " << mChangedAddress);
            break;  
				
         case Username: 
            mHasUsername = true;
            mUsername = new resip::Data(resip::Data::Share, body, attrLen);
            StackLog(<< "Username = " << *mUsername);
            break;
				
         case Password: 
            mHasPassword = true;
            mPassword = new resip::Data(resip::Data::Share, body, attrLen);
            StackLog(<< "Password = " << *mPassword);
            break;
				
         case MessageIntegrity:
            mHasMessageIntegrity = true;
            if (stunParseAtrIntegrity( body, attrLen, mMessageIntegrity) == false)
            {
               WarningLog(<< "problem parsing MessageIntegrity");
               return false;
            }
            //StackLog(<< "MessageIntegrity = " << mMessageIntegrity.hash);
            break;
				
         case ErrorCode:
            mHasErrorCode = true;
            if (stunParseAtrError(body, attrLen, mErrorCode) == false)
            {
               WarningLog(<< "problem parsing ErrorCode");
               return false;
            }
            StackLog(<< "ErrorCode = " << (int(mErrorCode.errorClass) * 100 + int(mErrorCode.number)) 
                              << " (" << *mErrorCode.reason << ")");
            break;
				
         case UnknownAttribute:
            mHasUnknownAttributes = true;
            if (stunParseAtrUnknown(body, attrLen, mUnknownAttributes) == false)
            {
               WarningLog(<< "problem parsing UnknownAttribute");
               return false;
            }
            // TODO output
            break;
				
         case ReflectedFrom:  // deprecated
            mHasReflectedFrom = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mReflectedFrom ) == false )
            {
               WarningLog(<< "problem parsing ReflectedFrom");
               return false;
            }
            StackLog(<< "ReflectedFrom = " << mReflectedFrom);
            break;  
				
         case Realm: 
            mHasRealm = true;
            mRealm = new resip::Data(resip::Data::Share, body, attrLen);
            StackLog(<< "Realm = " << *mRealm);
            break;

         case Nonce: 
            mHasNonce = true;
            mNonce = new resip::Data(resip::Data::Share, body, attrLen);
            StackLog(<< "Nonce = " << *mNonce);
            break;

         case XorMappedAddress_old:
         case XorMappedAddress:
            mHasXorMappedAddress = true;
            if ( stunParseAtrXorAddress(  body,  attrLen,  mXorMappedAddress ) == false )
            {
               WarningLog(<< "problem parsing XorMappedAddress");
               return false;
            }
            StackLog(<< "XorMappedAddress = " << mXorMappedAddress);
            break;  				

         case Fingerprint:
            mHasFingerprint = true;
            if (stunParseAtrUInt32( body, attrLen, mFingerprint) == false)
            {
               WarningLog(<< "problem parsing Fingerprint");
               return false;
            }
            StackLog(<< "Fingerprint = " << mFingerprint);
            break;

         case Server: 
            mHasServer = true;
            mServer = new resip::Data(resip::Data::Share, body, attrLen);
            StackLog(<< "Server = " << *mServer);
            break;

         case AlternateServer:
            mHasAlternateServer = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mAlternateServer ) == false )
            {
               WarningLog(<< "problem parsing AlternateServer");
               return false;
            }
            StackLog(<< "AlternateServer = " << mAlternateServer);
            break;  

         case RefreshInterval:
            mHasRefreshInterval = true;
            if (stunParseAtrUInt32( body, attrLen, mRefreshInterval) == false)
            {
               WarningLog(<< "problem parsing refresh interval");
               return false;
            }
            StackLog(<< "Refresh interval = " << mRefreshInterval);
            break;

         case SecondaryAddress:
            mHasSecondaryAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mSecondaryAddress ) == false )
            {
               WarningLog(<< "problem parsing secondaryAddress");
               return false;
            }
            StackLog(<< "SecondaryAddress = " << mSecondaryAddress);
            break;  

         // TURN attributes

         case TurnChannelNumber:
            mHasTurnChannelNumber = true;
            {
               UInt32 channelNumber;
               if(stunParseAtrUInt32( body, attrLen, channelNumber) == false)
               {
                  WarningLog(<< "problem parsing channel number");
                  return false;
               }
               mTurnChannelNumber = (channelNumber & 0xFFFF0000) >> 16;
            }
            StackLog(<< "Turn ChannelNumber = " << mTurnChannelNumber);
            break;

         case TurnLifetime:
            mHasTurnLifetime = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnLifetime) == false)
            {
               WarningLog(<< "problem parsing turn lifetime");
               return false;
            }
            StackLog(<< "Turn Lifetime = " << mTurnLifetime);
            break;

         case TurnAlternateServer:
            mHasTurnAlternateServer = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnAlternateServer ) == false )
            {
               WarningLog(<< "problem parsing turn alternate server");
               return false;
            }
            StackLog(<< "Turn Alternate Server = " << mTurnAlternateServer);
            break;

         case TurnMagicCookie:
            mHasTurnMagicCookie = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnMagicCookie) == false)
            {
               WarningLog(<< "problem parsing turn magic cookie");
               return false;
            }
            StackLog(<< "TurnMagicCookie (deprecated) = " << mTurnMagicCookie);
            break;

         case TurnBandwidth:
            mHasTurnBandwidth = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnBandwidth) == false)
            {
               WarningLog(<< "problem parsing turn bandwidth");
               return false;
            }
            StackLog(<< "Turn Bandwidth = " << mTurnBandwidth);
            break;

         case TurnDestinationAddress:
            mHasTurnDestinationAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  mTurnDestinationAddress ) == false )
            {
               WarningLog(<< "problem parsing turn destination address");
               return false;
            }
            StackLog(<< "Turn Destination Address = " << mTurnDestinationAddress);
            break;

         case TurnPeerAddress:
            mHasTurnPeerAddress = true;
            if ( stunParseAtrXorAddress(  body,  attrLen,  mTurnPeerAddress ) == false )
            {
               WarningLog(<< "problem parsing turn peer address");
               return false;
            }
            StackLog(<< "Turn Remote Address = " << mTurnPeerAddress);
            break;

         //overlay on parse, ownership is buffer parsed from
         case TurnData:
            mHasTurnData = true;
            mTurnData = new resip::Data(resip::Data::Share, body, attrLen);
            break;

         case TurnRelayAddress:
            mHasTurnRelayAddress = true;
            if ( stunParseAtrXorAddress(  body,  attrLen,  mTurnRelayAddress ) == false )
            {
               WarningLog(<< "problem parsing turn relay address");
               return false;
            }
            StackLog(<< "Turn Relay Address = " << mTurnRelayAddress);
            break;

         case TurnRequestedProps:
            mHasTurnRequestedProps = true;
            if (stunParseAtrRequestedProps( body, attrLen, mTurnRequestedProps) == false)
            {
               WarningLog(<< "problem parsing turn requested props");
               return false;
            }
            StackLog(<< "Turn Requested Props = " << (int)mTurnRequestedProps.propType);
            break;

         case TurnRequestedTransport:
            mHasTurnRequestedTransport = true;
            UInt32 requestedTransport;
            if (stunParseAtrUInt32( body, attrLen, requestedTransport) == false)
            {
               WarningLog(<< "problem parsing turn requested transport");
               return false;
            }
            mTurnRequestedTransport = requestedTransport >> 24;
            StackLog(<< "Turn Requested Transport = " << (int)mTurnRequestedTransport);
            break;

         case TurnReservationToken:
            mHasTurnReservationToken = true;
            if ( stunParseAtrUInt64(  body,  attrLen,  mTurnReservationToken ) == false )
            {
               WarningLog(<< "problem parsing turn reservation token");
               return false;
            }
            StackLog(<< "Turn Reservation Token = " << mTurnReservationToken);
            break;

         case TurnConnectStat:
            mHasTurnConnectStat = true;
            if (stunParseAtrUInt32( body, attrLen, mTurnConnectStat) == false)
            {
               WarningLog(<< "problem parsing turn connect stat");
               return false;
            }
            StackLog(<< "Turn Connect Stat = " << mTurnConnectStat);
            break;
					
         default:
            InfoLog(<< "Unknown attribute: " << atrType);
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

EncodeStream& 
operator<< ( EncodeStream& strm, const UInt128& r )
{
   strm << int(r.longpart[0]);
   for ( int i=1; i<4; i++ )
   {
      strm << ':' << int(r.longpart[i]);
   }
    
   return strm;
}

EncodeStream& 
operator<<( EncodeStream& strm, const StunMessage::StunAtrAddress& addr)
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

EncodeStream&
operator<<(EncodeStream& os, const StunMessage::StunMsgHdr& h)
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
      case StunMessage::TurnSendMethod:
         os << "Send";
         break;
      case StunMessage::TurnDataMethod:
         os << "Data";
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
      case StunMessage::BindMethod:
         os << "Bind";
         break;
      case StunMessage::SharedSecretMethod:
         os << "SharedSecret";
         break;
		case StunMessage::TurnAllocateMethod:
            os << "Allocate";
			break;
		case StunMessage::TurnRefreshMethod:
            os << "Refresh";
			break;
		case StunMessage::TurnChannelBindMethod:
            os << "ChannelBind";
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
StunMessage::encodeAtrUInt64(char* ptr, UInt16 type, UInt64 value)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 8);
   memcpy(ptr, reinterpret_cast<void*>(&value), sizeof(UInt64));
   return ptr + sizeof(UInt64);
}

char*
StunMessage::encodeAtrXorAddress(char* ptr, UInt16 type, const StunAtrAddress& atr)
{
   StunAtrAddress xorAtr;
   applyXorToAddress(atr, xorAtr);
   return encodeAtrAddress(ptr, type, xorAtr);
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
   ptr = encode16(ptr, 4 + (UInt16)atr.reason->size() + padsize); 
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
   ptr = encode16(ptr, (UInt16)atr->size()+padsize);  
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
StunMessage::encodeAtrRequestedProps(char* ptr, const TurnAtrRequestedProps& atr)
{
   ptr = encode16(ptr, TurnRequestedProps);
   ptr = encode16(ptr, 4);
   *ptr++ = atr.propType;  
   *ptr++ = 0; // pad
   ptr = encode16(ptr, 0); // pad
   return ptr;
}

unsigned int
StunMessage::stunEncodeMessage(char* buf, unsigned int bufLen) 
{
   assert(bufLen >= sizeof(StunMsgHdr));
   char* ptr = buf;

   StackLog(<< "Encoding stun message: ");

   mHeader.msgType = mClass | mMethod;

   ptr = encode16(ptr, mHeader.msgType);
   char* lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode(ptr, reinterpret_cast<const char*>(&mHeader.id), sizeof(mHeader.id));

   if (mHasTurnMagicCookie)
   {
      StackLog(<< "Encoding TurnMagicCookie: " << mTurnMagicCookie);
      ptr = encodeAtrUInt32(ptr, TurnMagicCookie, mTurnMagicCookie);
   }

   if (mHasMappedAddress)
   {
      StackLog(<< "Encoding MappedAddress: " << mMappedAddress);
      ptr = encodeAtrAddress (ptr, MappedAddress, mMappedAddress);
   }
   if (mHasResponseAddress)
   {
      StackLog(<< "Encoding ResponseAddress: " << mResponseAddress);
      ptr = encodeAtrAddress(ptr, ResponseAddress, mResponseAddress);
   }
   if (mHasChangeRequest)
   {
      StackLog(<< "Encoding ChangeRequest: " << mChangeRequest);
      ptr = encodeAtrUInt32(ptr, ChangeRequest, mChangeRequest);
   }
   if (mHasSourceAddress)
   {
      StackLog(<< "Encoding SourceAddress: " << mSourceAddress);
      ptr = encodeAtrAddress(ptr, SourceAddress, mSourceAddress);
   }
   if (mHasChangedAddress)
   {
      StackLog(<< "Encoding ChangedAddress: " << mChangedAddress);
      ptr = encodeAtrAddress(ptr, ChangedAddress, mChangedAddress);
   }
   if (mHasUsername)
   {
      StackLog(<< "Encoding Username: " << *mUsername);
      ptr = encodeAtrString(ptr, Username, mUsername);
   }
   if (mHasPassword)
   {
      StackLog(<< "Encoding Password: " << *mPassword);
      ptr = encodeAtrString(ptr, Password, mPassword);
   }
   if (mHasErrorCode)
   {
      StackLog(<< "Encoding ErrorCode: "
			<< int(mErrorCode.errorClass)  
			<< " number=" << int(mErrorCode.number) 
			<< " reason=" 
			<< *mErrorCode.reason);
		
      ptr = encodeAtrError(ptr, mErrorCode);
   }
   if (mHasUnknownAttributes)
   {
      StackLog(<< "Encoding UnknownAttribute: ???");
      ptr = encodeAtrUnknown(ptr, mUnknownAttributes);
   }
   if (mHasReflectedFrom)
   {
      StackLog(<< "Encoding ReflectedFrom: " << mReflectedFrom);
      ptr = encodeAtrAddress(ptr, ReflectedFrom, mReflectedFrom);
   }
   if (mHasRealm)
   {
      StackLog(<< "Encoding Realm: " << *mRealm);
      ptr = encodeAtrString(ptr, Realm, mRealm);
   }
   if (mHasNonce)
   {
      StackLog(<< "Encoding Nonce: " << *mNonce);
      ptr = encodeAtrString(ptr, Nonce, mNonce);
   }
   if (mHasXorMappedAddress)
   {
      StackLog(<< "Encoding XorMappedAddress: " << mXorMappedAddress);
      ptr = encodeAtrXorAddress(ptr, XorMappedAddress, mXorMappedAddress);
   }
   if (mHasServer)
   {
      StackLog(<< "Encoding Server: " << *mServer);
      ptr = encodeAtrString(ptr, Server, mServer);
   }
   if (mHasAlternateServer)
   {
      StackLog(<< "Encoding Alternate Server: " << mAlternateServer);
      ptr = encodeAtrAddress(ptr, AlternateServer, mAlternateServer);
   }
   if (mHasRefreshInterval)
   {
      StackLog(<< "Encoding Refresh Interval: " << mRefreshInterval);
      ptr = encodeAtrUInt32(ptr, RefreshInterval, mRefreshInterval);
   }
   if (mHasSecondaryAddress)
   {
      StackLog(<< "Encoding SecondaryAddress: " << mSecondaryAddress);
      ptr = encodeAtrAddress(ptr, SecondaryAddress, mSecondaryAddress);
   }

   if (mHasTurnChannelNumber)
   {
      StackLog(<< "Encoding Turn ChannelNumber: " << mTurnChannelNumber);
      ptr = encodeAtrUInt32(ptr, TurnChannelNumber, UInt32(mTurnChannelNumber << 16));
   }
   if (mHasTurnLifetime)
   {
      StackLog(<< "Encoding Turn Lifetime: " << mTurnLifetime);
      ptr = encodeAtrUInt32(ptr, TurnLifetime, mTurnLifetime);
   }
   if (mHasTurnAlternateServer)
   {
      StackLog(<< "Encoding Turn AlternateServer: " << mTurnAlternateServer);
      ptr = encodeAtrAddress(ptr, TurnAlternateServer, mTurnAlternateServer);
   }
   if (mHasTurnBandwidth)
   {
      StackLog(<< "Encoding Turn Bandwidth: " << mTurnBandwidth);
      ptr = encodeAtrUInt32(ptr, TurnBandwidth, mTurnBandwidth);
   }   
   if (mHasTurnDestinationAddress)
   {
      StackLog(<< "Encoding Turn DestinationAddress: " << mTurnDestinationAddress);
      ptr = encodeAtrAddress (ptr, TurnDestinationAddress, mTurnDestinationAddress);
   }
   if (mHasTurnPeerAddress)
   {
      StackLog(<< "Encoding Turn PeerAddress: " << mTurnPeerAddress);
      ptr = encodeAtrXorAddress (ptr, TurnPeerAddress, mTurnPeerAddress);
   }
   if (mHasTurnData)
   {
      StackLog(<< "Encoding TurnData (not shown)");
      ptr = encodeTurnData (ptr, mTurnData);
   }
   if (mHasTurnRelayAddress)
   {
      StackLog(<< "Encoding Turn RelayAddress: " << mTurnRelayAddress);
      ptr = encodeAtrXorAddress (ptr, TurnRelayAddress, mTurnRelayAddress);
   }
   if (mHasTurnRequestedProps)
   {
      StackLog(<< "Encoding Turn RequestedProps: " << (int)mTurnRequestedProps.propType);
      ptr = encodeAtrRequestedProps(ptr, mTurnRequestedProps);
   }   
   if (mHasTurnRequestedTransport)
   {
      StackLog(<< "Encoding Turn RequestedTransport: " << (int)mTurnRequestedTransport);
      ptr = encodeAtrUInt32(ptr, TurnRequestedTransport, UInt32(mTurnRequestedTransport << 24));
   }   
   if (mHasTurnReservationToken)
   {
      StackLog(<< "Encoding Turn ReservationToken: " << mTurnReservationToken);
      ptr = encodeAtrUInt64 (ptr, TurnReservationToken, mTurnReservationToken);
   }
   if (mHasTurnConnectStat)
   {
      StackLog(<< "Encoding Turn Connect Stat: " << mTurnConnectStat);
      ptr = encodeAtrUInt32(ptr, TurnConnectStat, mTurnConnectStat);
   }   

   // Update Length in header now - needed in message integrity and fingerprint calculations
   UInt16 msgSize = ptr - buf - sizeof(StunMsgHdr);
   if(mHasMessageIntegrity) msgSize += 24;  // 20 (attribute value) + 4 (attribute header)
   if(mHasFingerprint) msgSize += 8;        // 4 (attribute value) + 4 (attribute header)
   encode16(lengthp, msgSize);

   if (mHasMessageIntegrity)
   {
      StackLog(<< "HMAC with key: " << mHmacKey);

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
      StackLog(<< "Adding message integrity: buffer size=" << len+padding << ", hmacKey=" << mHmacKey);
      computeHmac(integrity.hash, buf, len + padding, mHmacKey.c_str(), (int)mHmacKey.size());
	   ptr = encodeAtrIntegrity(ptrMessageIntegrity, integrity);
   }

   // add finger print if required
   if (mHasFingerprint)
   {
      StackLog(<< "Calculating fingerprint for data of size " << ptr-buf);
      stun_crc_32_type stun_crc;
      stun_crc.process_bytes(buf, ptr-buf); // Calculate CRC across entire message, except the fingerprint attribute
      UInt32 fingerprint = stun_crc.checksum();
      ptr = encodeAtrUInt32(ptr, Fingerprint, fingerprint);
   }

   return int(ptr - buf);
}

unsigned int
StunMessage::stunEncodeFramedMessage(char* buf, unsigned int bufLen)
{
   unsigned short size = (unsigned short)stunEncodeMessage(&buf[4], bufLen-4);

   // Add Frame Header info
   buf[0] = 0; // Channel 0 for Stun Messages
   buf[1] = 0; 
   UInt16 frameSize = htons(size);
   memcpy(&buf[2], (void*)&frameSize, 2);  // size is not needed if udp - but should be harmless
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
   	
   StackLog(<< "computed username=" << *mUsername);

   // Compute Password
   mHasPassword = true;

   if(!mPassword)
   {
      mPassword = new Data;
   }
   assert(mPassword);
   generateShortTermPasswordForUsername(*mPassword);

   StackLog(<< "computed password=" << *mPassword);
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
StunMessage::getTupleFromUsername(StunTuple& tuple)
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
      tuple.setAddress(addressv6);

      unsigned int port;
      Data portPart(Data::Share, mUsername->data()+25, 4);
      portPart = portPart.base64decode();
      memcpy(&port, portPart.data(), sizeof(port));
      tuple.setPort(port);
   }
   else
   {
      Data addressPart(Data::Share, mUsername->data(), 8);  
      addressPart = addressPart.base64decode();
      asio::ip::address_v4::bytes_type bytes;
      memcpy(bytes.c_array(), addressPart.data(), bytes.size());
      asio::ip::address_v4 addressv4(bytes);
      tuple.setAddress(addressv4);

      unsigned int port;
      Data portPart(Data::Share, mUsername->data()+9, 4);
      portPart = portPart.base64decode();
      memcpy(&port, portPart.data(), sizeof(port));
      tuple.setPort(port);
   }
}

void
StunMessage::calculateHmacKey(Data& hmacKey, const Data& longtermAuthenticationPassword)
{
   assert(mHasUsername);

   if(mHasRealm)  // Longterm authenicationmode
   {
      MD5Stream r;
      r << *mUsername << ":" << *mRealm << ":" << longtermAuthenticationPassword;
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

      // pad with zeros prior to calculating message integrity attribute	   
      int len = (int)mBuffer.size()-20-4-(mHasFingerprint?8:0);  // remove last TLV (Message Integrity TLV) from HMAC calculation and Fingerprint TLV (if present)
      int padding = len % 64 == 0 ? 0 : 64 - (len % 64);
      Data buffer(mBuffer.data(), len+padding);  // .slg. this creates a temp copy of the buffer so that we can pad it
      memset((void*)(buffer.data() + len), 0, padding);  // Zero out padding area 

      StackLog(<< "Checking message integrity: buffer size=" << (unsigned int)buffer.size() << ", hmacKey=" << hmacKey);
      computeHmac((char*)hmac, buffer.data(), buffer.size(), hmacKey.c_str(), hmacKey.size());

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
      StackLog(<< "Calculating fingerprint to check for data of size " << mBuffer.size() - 8);
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
