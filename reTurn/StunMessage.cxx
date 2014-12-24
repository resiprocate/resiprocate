
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include <boost/crc.hpp>

#include "StunMessage.hxx"

#include <rutil/compat.hxx>
#include <rutil/Timer.hxx>
#include <rutil/Random.hxx>
#include <rutil/DataStream.hxx>
#include <rutil/MD5Stream.hxx>
#include <rutil/WinLeakCheck.hxx>
#include <rutil/Logger.hxx>
#include "ReTurnSubsystem.hxx"

#define RESIPROCATE_SUBSYSTEM ReTurnSubsystem::RETURN

#ifdef USE_SSL
#include <openssl/hmac.h>
#endif

using namespace std;
using namespace resip;

// What should we set these to?  Used for short term password generation.
static const Data USERNAME_KEY("stunServerUsernameKey");
static const Data PASSWORD_KEY("stunServerPasswordKey");

#define MAX_USERNAME_BYTES          512
#define MAX_PASSWORD_BYTES          512
#define MAX_REALM_BYTES             763
#define MAX_NONCE_BYTES             763
#define MAX_SOFTWARE_BYTES          763
#define MAX_ERRORCODE_REASON_BYTES  763

#define STUN_CRC_FINAL_XOR 0x5354554e

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
      resip_assert(false);
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
   if(mSoftware) delete mSoftware;
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
   mHasSoftware = false;
   mHasAlternateServer = false;
   mHasSecondaryAddress = false;
   mHasTurnChannelNumber = false;
   mHasTurnLifetime = false;
   mHasTurnBandwidth = false;
   mHasTurnData = false;
   mHasTurnXorRelayedAddress = false;
   mHasTurnEvenPort = false;
   mHasTurnRequestedTransport = false;
   mHasTurnDontFragment = false;
   mHasTurnReservationToken = false;
   mHasTurnConnectStat = false;
   mCntTurnXorPeerAddress = 0;
   mHasTurnRequestedAddressFamily = false;
   mHasIcePriority = false;
   mIcePriority = 0;
   mHasIceUseCandidate = false;
   mHasIceControlled = false;
   mIceControlledTieBreaker = 0;
   mHasIceControlling = false;
   mIceControllingTieBreaker = 0;
   mErrorCode.reason = 0;
   mUsername = 0;
   mPassword = 0;
   mRealm = 0;
   mNonce = 0;
   mSoftware = 0;
   mTurnData = 0;
   mMessageIntegrityMsgLength = 0;
   mUnknownRequiredAttributes.numAttributes = 0;
}

void 
StunMessage::createHeader(UInt16 stunclass, UInt16 method)
{
   mClass = stunclass;
   mMethod = method;

   // Assign a tid
   mHeader.id.magicCookie = htonl(StunMagicCookie);
   Data random = Random::getCryptoRandom(12);
   memcpy(&mHeader.id.tid, random.data(), sizeof(mHeader.id.tid));
}

void 
StunMessage::setErrorCode(unsigned short errorCode, const char* reason)
{
   resip_assert(errorCode >= 100 && errorCode <= 699);
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
StunMessage::setSoftware(const char* software)
{
   mHasSoftware = true;
   if(mSoftware)
   {
      *mSoftware = software;
   }
   else
   {
      mSoftware = new Data(software);
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
StunMessage::setIcePriority(UInt32 priority)
{
   mHasIcePriority = true;
   mIcePriority = priority;
}

void 
StunMessage::setIceUseCandidate()
{
   mHasIceUseCandidate = true;
}

void 
StunMessage::setIceControlled()
{
   mHasIceControlled = true;
   const resip::Data& tieBreaker = resip::Random::getCryptoRandom(8);
   memcpy(&mIceControlledTieBreaker, tieBreaker.begin(), sizeof(mIceControlledTieBreaker));
}

void 
StunMessage::setIceControlling()
{
   mHasIceControlling = true;
   const resip::Data& tieBreaker = resip::Random::getCryptoRandom(8);
   memcpy(&mIceControllingTieBreaker, tieBreaker.begin(), sizeof(mIceControllingTieBreaker));
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
         // Note:  MagicCookieAndTid are stored in network byte order
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
      // Note:  addr.ipv6 is stored in network byte order
      address.family = StunMessage::IPv6Family;  
      memcpy(&address.addr.ipv6, tuple.getAddress().to_v6().to_bytes().data(), sizeof(address.addr.ipv6));
   }
   else
   {
      // Note:  addr.ipv4 is stored in host byte order
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
      // Note:  addr.ipv6 is stored in network byte order
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.data(), &address.addr.ipv6, bytes.size());
      asio::ip::address_v6 addr(bytes);
      tuple.setAddress(addr);
   }
   else
   {
      // Note:  addr.ipv4 is stored in host byte order
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
   if ( hdrLen != 8 /* ipv4 size */ && hdrLen != 20 /* ipv6 size */ )
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
      memcpy(&naddr, body, sizeof(UInt32)); body+=sizeof(UInt32);
      result.addr.ipv4 = ntohl(naddr);
      // Note:  addr.ipv4 is stored in host byte order
      return true;
   }
   else if (result.family == IPv6Family)
   {
      memcpy(&result.addr.ipv6, body, sizeof(result.addr.ipv6)); body+=sizeof(result.addr.ipv6);
      // Note:  addr.ipv6 is stored in host byte order
      return true;
   }
   else
   {
      WarningLog(<< "bad address family: " << result.family);
   }
	
   return false;
}

bool 
StunMessage::stunParseAtrEvenPort( char* body, unsigned int hdrLen,  TurnAtrEvenPort& result )
{
   if ( hdrLen != 1 )
   {
      WarningLog(<< "hdrLen wrong for EvenPort");
      return false;
   }
   result.propType = *body & 0x80;  // copy first 8 bits into propType - but only look at highest order
	
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
      result = ntoh64(result);
      return true;
   }
}

bool 
StunMessage::stunParseAtrError( char* body, unsigned int hdrLen,  StunAtrError& result )
{
   body+=2;  // skip pad
   result.errorClass = *body++ & 0x7;
   result.number = *body++;
	
   int reasonLen = (hdrLen -4) > MAX_ERRORCODE_REASON_BYTES ? MAX_ERRORCODE_REASON_BYTES : hdrLen-4;
   result.reason = new resip::Data(resip::Data::Share, body, reasonLen);
   return true;
}

bool 
StunMessage::stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown& result )
{
   if ( hdrLen >= sizeof(result) )
   {
      WarningLog(<< "hdrLen wrong for Unknown attribute or too many unknown attributes present");
      return false;
   }
   else
   {
      if (hdrLen % 2 != 0) return false;
      result.numAttributes = hdrLen / 2;
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
   StackLog(<< "Received stun message: " << bufLen << " bytes");
	
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
      WarningLog(<< "Message header length (" << mHeader.msgLength << ") + header size (" <<  sizeof(StunMsgHdr) << ") doesn't match message size (" << bufLen << ")"); 
      return false;
   }

   // Check if class and method are valid
   mClass = mHeader.msgType & 0x0110;
   mMethod = mHeader.msgType & 0x000F;
	
   // Look for stun magic cookie
   mHasMagicCookie = mHeader.id.magicCookie == htonl(StunMagicCookie);
   if(!mHasMagicCookie)
   {
      StackLog(<< "stun magic cookie not found.");
   }

   char* body = buf + sizeof(StunMsgHdr);
   unsigned int size = mHeader.msgLength;
   mUnknownRequiredAttributes.numAttributes = 0;
	
   StackLog(<< "bytes after header = " << size);
	
   while ( size > 0 )
   {
      // !jf! should check that there are enough bytes left in the buffer
		
      StunAtrHdr* attr = reinterpret_cast<StunAtrHdr*>(body);
		
      unsigned int attrLen = ntohs(attr->length);
      // attrLen may not be on 4 byte boundary, in which case we need to pad to 4 bytes when advancing to next attribute
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
            if(!mHasMappedAddress)
            {
               mHasMappedAddress = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mMappedAddress )== false )
               {
                  WarningLog(<< "problem parsing MappedAddress");
                  return false;
               }
               StackLog(<< "MappedAddress = " << mMappedAddress);
            }
            else
            {
               WarningLog(<< "Duplicate MappedAddress in message - ignoring.");
            }
            break;  

         case ResponseAddress:  // deprecated
            if(!mHasResponseAddress)
            {
               mHasResponseAddress = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mResponseAddress )== false )
               {
                  WarningLog(<< "problem parsing ResponseAddress");
                  return false;
               }
               StackLog(<< "ResponseAddress = " << mResponseAddress);
            }
            else
            {
               WarningLog(<< "Duplicate ResponseAddress in message - ignoring.");
            }
            break;  
				
         case ChangeRequest:  // deprecated
            if(!mHasChangeRequest)
            {
               mHasChangeRequest = true;
               if (stunParseAtrUInt32( body, attrLen, mChangeRequest) == false)
               {
                  WarningLog(<< "problem parsing ChangeRequest");
                  return false;
               }
               StackLog(<< "ChangeRequest = " << mChangeRequest);
            }
            else
            {
               WarningLog(<< "Duplicate ChangeRequest in message - ignoring.");
            }
            break;
				
         case SourceAddress:  // deprecated
            if(!mHasSourceAddress)
            {
               mHasSourceAddress = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mSourceAddress )== false )
               {
                  WarningLog(<< "problem parsing SourceAddress");
                  return false;
               }
               StackLog(<< "SourceAddress = " << mSourceAddress);
            }
            else
            {
               WarningLog(<< "Duplicate SourceAddress in message - ignoring.");
            }
            break;  
				
         case ChangedAddress:  // deprecated
            if(!mHasChangedAddress)
            {
               mHasChangedAddress = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mChangedAddress )== false )
               {
                  WarningLog(<< "problem parsing ChangedAddress");
                  return false;
               }
               StackLog(<< "ChangedAddress = " << mChangedAddress);
            }
            else
            {
               WarningLog(<< "Duplicate ChangedAddress in message - ignoring.");
            }
            break;  
				
         case Username: 
            if(!mHasUsername)
            {
               if(attrLen > MAX_USERNAME_BYTES)
               {
                  WarningLog(<< "Username length=" << attrLen << " is longer than max allowed=" << MAX_USERNAME_BYTES);
                  return false;
               }
               mHasUsername = true;
               mUsername = new resip::Data(resip::Data::Share, body, attrLen);
               StackLog(<< "Username = " << *mUsername);
            }
            else
            {
               WarningLog(<< "Duplicate Username in message - ignoring.");
            }
            break;
				
         case Password: 
            if(!mHasPassword)
            {
               if(attrLen > MAX_PASSWORD_BYTES)
               {
                  WarningLog(<< "Password length=" << attrLen << " is longer than max allowed=" << MAX_PASSWORD_BYTES);
                  return false;
               }
               mHasPassword = true;
               mPassword = new resip::Data(resip::Data::Share, body, attrLen);
               StackLog(<< "Password = " << *mPassword);
            }
            else
            {
               WarningLog(<< "Duplicate Password in message - ignoring.");
            }
            break;
				
         case MessageIntegrity:
            if(!mHasMessageIntegrity)
            {
               mHasMessageIntegrity = true;
               if (stunParseAtrIntegrity( body, attrLen, mMessageIntegrity) == false)
               {
                  WarningLog(<< "problem parsing MessageIntegrity");
                  return false;
               }
               //StackLog(<< "MessageIntegrity = " << mMessageIntegrity.hash);
               mMessageIntegrityMsgLength = body + attrLen - buf - sizeof(StunMsgHdr);
            }
            else
            {
               WarningLog(<< "Duplicate MessageIntegrity in message - ignoring.");
            }
            break;
				
         case ErrorCode:
            if(!mHasErrorCode)
            {
               if(attrLen-4 > MAX_ERRORCODE_REASON_BYTES)
               {
                  WarningLog(<< "ErrorCode reason length=" << attrLen-4 << " is longer than max allowed=" << MAX_ERRORCODE_REASON_BYTES);
                  return false;
               }
               mHasErrorCode = true;
               if (stunParseAtrError(body, attrLen, mErrorCode) == false)
               {
                  WarningLog(<< "problem parsing ErrorCode");
                  return false;
               }
               StackLog(<< "ErrorCode = " << (int(mErrorCode.errorClass) * 100 + int(mErrorCode.number)) 
                                 << " (" << *mErrorCode.reason << ")");
            }
            else
            {
               WarningLog(<< "Duplicate ErrorCode in message - ignoring.");
            }
            break;
				
         case UnknownAttribute:
            if(!mHasUnknownAttributes)
            {
               mHasUnknownAttributes = true;
               if (stunParseAtrUnknown(body, attrLen, mUnknownAttributes) == false)
               {
                  WarningLog(<< "problem parsing UnknownAttribute");
                  return false;
               }
               // TODO output
            }
            else
            {
               WarningLog(<< "Duplicate UnknownAttribute in message - ignoring.");
            }
            break;
				
         case ReflectedFrom:  // deprecated
            if(!mHasReflectedFrom)
            {
               mHasReflectedFrom = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mReflectedFrom ) == false )
               {
                  WarningLog(<< "problem parsing ReflectedFrom");
                  return false;
               }
               StackLog(<< "ReflectedFrom = " << mReflectedFrom);
            }
            else
            {
               WarningLog(<< "Duplicate ReflectedFrom in message - ignoring.");
            }
            break;  
				
         case Realm: 
            if(!mHasRealm)
            {
               if(attrLen > MAX_REALM_BYTES)
               {
                  WarningLog(<< "Realm length=" << attrLen << " is longer than max allowed=" << MAX_REALM_BYTES);
                  return false;
               }
               mHasRealm = true;
               mRealm = new resip::Data(resip::Data::Share, body, attrLen);
               StackLog(<< "Realm = " << *mRealm);
            }
            else
            {
               WarningLog(<< "Duplicate Realm in message - ignoring.");
            }
            break;

         case Nonce: 
            if(!mHasNonce)
            {
               if(attrLen > MAX_NONCE_BYTES)
               {
                  WarningLog(<< "Nonce length=" << attrLen << " is longer than max allowed=" << MAX_NONCE_BYTES);
                  return false;
               }
               mHasNonce = true;
               mNonce = new resip::Data(resip::Data::Share, body, attrLen);
               StackLog(<< "Nonce = " << *mNonce);
            }
            else
            {
               WarningLog(<< "Duplicate Nonce in message - ignoring.");
            }
            break;

         case XorMappedAddress_old:
         case XorMappedAddress:
            if(!mHasXorMappedAddress)
            {
               mHasXorMappedAddress = true;
               if ( stunParseAtrXorAddress(  body,  attrLen,  mXorMappedAddress ) == false )
               {
                  WarningLog(<< "problem parsing XorMappedAddress");
                  return false;
               }
               StackLog(<< "XorMappedAddress = " << mXorMappedAddress);
            }
            else
            {
               WarningLog(<< "Duplicate XorMappedAddress in message - ignoring.");
            }
            break;  				

         case Fingerprint:
            if(!mHasFingerprint)
            {
               mHasFingerprint = true;
               if (stunParseAtrUInt32( body, attrLen, mFingerprint) == false)
               {
                  WarningLog(<< "problem parsing Fingerprint");
                  return false;
               }
               StackLog(<< "Fingerprint = " << mFingerprint);
            }
            else
            {
               WarningLog(<< "Duplicate Fingerprint in message - ignoring.");
            }
            break;

         case Software: 
            if(!mHasSoftware)
            {
               if(attrLen > MAX_SOFTWARE_BYTES)
               {
                  WarningLog(<< "Software length=" << attrLen << " is longer than max allowed=" << MAX_SOFTWARE_BYTES);
                  return false;
               }
               mHasSoftware = true;
               mSoftware = new resip::Data(resip::Data::Share, body, attrLen);
               StackLog(<< "Software = " << *mSoftware);
            }
            else
            {
               WarningLog(<< "Duplicate Software in message - ignoring.");
            }
            break;

         case AlternateServer:
            if(!mHasAlternateServer)
            {
               mHasAlternateServer = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mAlternateServer ) == false )
               {
                  WarningLog(<< "problem parsing AlternateServer");
                  return false;
               }
               StackLog(<< "AlternateServer = " << mAlternateServer);
            }
            else
            {
               WarningLog(<< "Duplicate AlternateServer in message - ignoring.");
            }
            break;  

         case SecondaryAddress:
            if(!mHasSecondaryAddress)
            {
               mHasSecondaryAddress = true;
               if ( stunParseAtrAddress(  body,  attrLen,  mSecondaryAddress ) == false )
               {
                  WarningLog(<< "problem parsing secondaryAddress");
                  return false;
               }
               StackLog(<< "SecondaryAddress = " << mSecondaryAddress);
            }
            else
            {
               WarningLog(<< "Duplicate SecondaryAddress in message - ignoring.");
            }
            break;  

         // TURN attributes

         case TurnChannelNumber:
            if(!mHasTurnChannelNumber)
            {
               UInt32 channelNumber;
               mHasTurnChannelNumber = true;
               if(stunParseAtrUInt32( body, attrLen, channelNumber) == false)
               {
                  WarningLog(<< "problem parsing channel number");
                  return false;
               }
               mTurnChannelNumber = (channelNumber & 0xFFFF0000) >> 16;
               StackLog(<< "Turn ChannelNumber = " << mTurnChannelNumber);
            }
            else
            {
               WarningLog(<< "Duplicate TurnChannelNumber in message - ignoring.");
            }
            break;

         case TurnLifetime:
            if(!mHasTurnLifetime)
            {
               mHasTurnLifetime = true;
               if (stunParseAtrUInt32( body, attrLen, mTurnLifetime) == false)
               {
                  WarningLog(<< "problem parsing turn lifetime");
                  return false;
               }
               StackLog(<< "Turn Lifetime = " << mTurnLifetime);
            }
            else
            {
               WarningLog(<< "Duplicate TurnLifetime in message - ignoring.");
            }
            break;

         case TurnBandwidth:
            if(!mHasTurnBandwidth)
            {
               mHasTurnBandwidth = true;
               if (stunParseAtrUInt32( body, attrLen, mTurnBandwidth) == false)
               {
                  WarningLog(<< "problem parsing turn bandwidth");
                  return false;
               }
               StackLog(<< "Turn Bandwidth = " << mTurnBandwidth);
            }
            else
            {
               WarningLog(<< "Duplicate TurnBandwidth in message - ignoring.");
            }
            break;

         case TurnXorPeerAddress:
            if(mCntTurnXorPeerAddress < TURN_MAX_XOR_PEER_ADDR)
            {
               if ( stunParseAtrXorAddress(  body,  attrLen,  mTurnXorPeerAddress[mCntTurnXorPeerAddress]) == false )
               {
                  WarningLog(<< "problem parsing turn peer address");
                  return false;
               }
               StackLog(<< "Turn Peer Address = " << mTurnXorPeerAddress[mCntTurnXorPeerAddress]);
               mCntTurnXorPeerAddress++;
            }
            else
            {
               WarningLog(<< "Duplicate TurnXorPeerAddress in message - ignoring.");
            }
            break;

         //overlay on parse, ownership is buffer parsed from
         case TurnData:
            if(!mHasTurnData)
            {
               mHasTurnData = true;
               mTurnData = new resip::Data(resip::Data::Share, body, attrLen);
            }
            else
            {
               WarningLog(<< "Duplicate TurnData in message - ignoring.");
            }
            break;

         case TurnXorRelayedAddress:
            if(!mHasTurnXorRelayedAddress)
            {
               mHasTurnXorRelayedAddress = true;
               if ( stunParseAtrXorAddress(  body,  attrLen,  mTurnXorRelayedAddress ) == false )
               {
                  WarningLog(<< "problem parsing turn relay address");
                  return false;
               }
               StackLog(<< "Turn Relayed Address = " << mTurnXorRelayedAddress);
            }
            else
            {
               WarningLog(<< "Duplicate TurnXorRelayedAddress in message - ignoring.");
            }
            break;

         case TurnEvenPort:
            if(!mHasTurnEvenPort)
            {
               mHasTurnEvenPort = true;
               if (stunParseAtrEvenPort( body, attrLen, mTurnEvenPort) == false)
               {
                  WarningLog(<< "problem parsing turn even port");
                  return false;
               }
               StackLog(<< "Turn Even Port = " << (int)mTurnEvenPort.propType);
            }
            else
            {
               WarningLog(<< "Duplicate TurnEvenPort in message - ignoring.");
            }
            break;

         case TurnRequestedTransport:
            if(!mHasTurnRequestedTransport)
            {
               mHasTurnRequestedTransport = true;
               UInt32 requestedTransport;
               if (stunParseAtrUInt32( body, attrLen, requestedTransport) == false)
               {
                  WarningLog(<< "problem parsing turn requested transport");
                  return false;
               }
               mTurnRequestedTransport = requestedTransport >> 24;
               StackLog(<< "Turn Requested Transport = " << (int)mTurnRequestedTransport);
            }
            else
            {
               WarningLog(<< "Duplicate TurnRequestedTransport in message - ignoring.");
            }
            break;

         case TurnDontFragment:
            if(!mHasTurnDontFragment)
            {
               mHasTurnDontFragment = true;
               if(attrLen != 0)
               {
                  WarningLog(<< "invalid attribute length for DontFragment attribute");
                  return false;
               }
               StackLog(<< "Turn Dont Fragement = <exists>");
            }
            else
            {
               WarningLog(<< "Duplicate TurnDontFragment in message - ignoring.");
            }
            break;

         case TurnReservationToken:
            if(!mHasTurnReservationToken)
            {
               mHasTurnReservationToken = true;
               if ( stunParseAtrUInt64(  body,  attrLen,  mTurnReservationToken ) == false )
               {
                  WarningLog(<< "problem parsing turn reservation token");
                  return false;
               }
               StackLog(<< "Turn Reservation Token = " << mTurnReservationToken);
            }
            else
            {
               WarningLog(<< "Duplicate TurnReservationToken in message - ignoring.");
            }
            break;

         case TurnConnectStat:
            if(!mHasTurnConnectStat)
            {
               mHasTurnConnectStat = true;
               if (stunParseAtrUInt32( body, attrLen, mTurnConnectStat) == false)
               {
                  WarningLog(<< "problem parsing turn connect stat");
                  return false;
               }
               StackLog(<< "Turn Connect Stat = " << mTurnConnectStat);
            }
            else
            {
               WarningLog(<< "Duplicate TurnConnectStat in message - ignoring.");
            }
            break;
					
         // ICE attributes
         case IcePriority:
            if(!mHasIcePriority)
            {
               mHasIcePriority = true;
               if (stunParseAtrUInt32( body, attrLen, mIcePriority) == false)
               {
                  WarningLog(<< "problem parsing ICE priority");
                  return false;
               }
               StackLog(<< "Ice Priority = " << mIcePriority);
            }
            else
            {
               WarningLog(<< "Duplicate IcePriority in message - ignoring.");
            }
            break;

         case IceUseCandidate:
            if(!mHasIceUseCandidate)
            {
               mHasIceUseCandidate = true;
               if(attrLen != 0)
               {
                  WarningLog(<< "invalid attribute length for IceUseCandidate attribute");
                  return false;
               }
               StackLog(<< "Ice UseCandidate = <exists>");
            }
            else
            {
               WarningLog(<< "Duplicate IceUseCandidate in message - ignoring.");
            }
            break;

         case IceControlled:
            if(!mHasIceControlled)
            {
               mHasIceControlled = true;
               if (stunParseAtrUInt64( body, attrLen, mIceControlledTieBreaker) == false)
               {
                  WarningLog(<< "problem parsing ICE controlled");
                  return false;
               }
               StackLog(<< "Ice controlled = " << mIceControlledTieBreaker);
            }
            else
            {
               WarningLog(<< "Duplicate IceControlled in message - ignoring.");
            }
            break;

         case IceControlling:
            if(!mHasIceControlling)
            {
               mHasIceControlling = true;
               if (stunParseAtrUInt64( body, attrLen, mIceControllingTieBreaker) == false)
               {
                  WarningLog(<< "problem parsing ICE controlling");
                  return false;
               }
               StackLog(<< "Ice controlling = " << mIceControllingTieBreaker);
            }
            else
            {
               WarningLog(<< "Duplicate IceControlling in message - ignoring.");
            }
            break;
					
         default:
            if ( atrType <= 0x7FFF ) 
            {
               if(mUnknownRequiredAttributes.numAttributes < STUN_MAX_UNKNOWN_ATTRIBUTES)
               {
                  mUnknownRequiredAttributes.attrType[mUnknownRequiredAttributes.numAttributes++] = atrType;
               }
               WarningLog(<< "Unknown comprehension required attribute: " << atrType);
            }
            else
            {
               InfoLog(<< "Ignoring unknown comprehension optional attribute: " << atrType);
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
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.data(), &addr.addr.ipv6, bytes.size());
      asio::ip::address_v6 addrv6(bytes);

      strm << "[" << addrv6.to_string() << "]:" << addr.port;
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
		case StunMessage::TurnCreatePermissionMethod:
			os << "CreatePermission";
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
StunMessage::encode64(char* buf, const UInt64 data)
{
   UInt64 ndata = hton64(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt64));
   return buf + sizeof(UInt64);
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
   memset(ptr, 0, padsize);  // zero out padded data (note: this is not required by the RFC)
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
   ptr = encode64(ptr, value);
   return ptr;
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
      // Note:  addr.ipv6 is stored in network byte order
      memcpy(ptr, &atr.addr.ipv6, sizeof(atr.addr.ipv6));
      ptr += sizeof(atr.addr.ipv6);
   }
   else
   {
      // Note:  addr.ipv4 is stored in host byte order - encode32 will conver to network byte order
      ptr = encode32(ptr, atr.addr.ipv4);
   }
	
   return ptr;
}

char* 
StunMessage::encodeAtrError(char* ptr, const StunAtrError& atr)
{
   resip_assert(atr.reason);
   UInt16 padsize = (unsigned int)atr.reason->size() % 4 == 0 ? 0 : 4 - ((unsigned int)atr.reason->size() % 4);

   ptr = encode16(ptr, ErrorCode);
   ptr = encode16(ptr, 4 + (UInt16)atr.reason->size()); 
   ptr = encode16(ptr, 0); // pad
   *ptr++ = atr.errorClass & 0x7;  // first 3 bits only
   *ptr++ = atr.number;
   ptr = encode(ptr, atr.reason->data(), (unsigned int)atr.reason->size());
   memset(ptr, 0, padsize);
   return ptr+padsize;
}

char* 
StunMessage::encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr)
{
   UInt16 padsize = (2*atr.numAttributes) % 4 == 0 ? 0 : 4 - ((2*atr.numAttributes) % 4);
   ptr = encode16(ptr, UnknownAttribute);
   ptr = encode16(ptr, 2*atr.numAttributes);
   for (int i=0; i<atr.numAttributes; i++)
   {
      ptr = encode16(ptr, atr.attrType[i]);
   }
   return ptr+padsize;
}

char* 
StunMessage::encodeAtrString(char* ptr, UInt16 type, const Data* atr, UInt16 maxBytes)
{
   resip_assert(atr);
   UInt16 size = atr->size() > maxBytes ? maxBytes : (UInt16)atr->size();
   UInt16 padsize = size % 4 == 0 ? 0 : 4 - (size % 4);
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, size);  
   ptr = encode(ptr, atr->data(), size);
   memset(ptr, 0, padsize);  // zero out padded data (note: this is not required by the RFC)
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
StunMessage::encodeAtrEvenPort(char* ptr, const TurnAtrEvenPort& atr)
{
   ptr = encode16(ptr, TurnEvenPort);
   ptr = encode16(ptr, 1);
   *ptr++ = atr.propType;  
   *ptr++ = 0; // pad
   ptr = encode16(ptr, 0); // pad
   return ptr;
}

bool 
StunMessage::hasMagicCookie()
{
   return mHeader.id.magicCookie == htonl(StunMessage::StunMagicCookie);
}

unsigned int
StunMessage::stunEncodeMessage(char* buf, unsigned int bufLen) 
{
   resip_assert(bufLen >= sizeof(StunMsgHdr));
   char* ptr = buf;

   mHeader.msgType = mClass | mMethod;

   ptr = encode16(ptr, mHeader.msgType);
   char* lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode(ptr, reinterpret_cast<const char*>(&mHeader.id), sizeof(mHeader.id));

   StackLog(<< "Encoding stun message: " << mHeader);

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
      ptr = encodeAtrString(ptr, Username, mUsername, MAX_USERNAME_BYTES);
   }
   if (mHasPassword)
   {
      StackLog(<< "Encoding Password: " << *mPassword);
      ptr = encodeAtrString(ptr, Password, mPassword, MAX_PASSWORD_BYTES);
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
      ptr = encodeAtrString(ptr, Realm, mRealm, MAX_REALM_BYTES);
   }
   if (mHasNonce)
   {
      StackLog(<< "Encoding Nonce: " << *mNonce);
      ptr = encodeAtrString(ptr, Nonce, mNonce, MAX_NONCE_BYTES);
   }
   if (mHasXorMappedAddress)
   {
      StackLog(<< "Encoding XorMappedAddress: " << mXorMappedAddress);
      ptr = encodeAtrXorAddress(ptr, XorMappedAddress, mXorMappedAddress);
   }
   if (mHasSoftware)
   {
      StackLog(<< "Encoding Software: " << *mSoftware);
      ptr = encodeAtrString(ptr, Software, mSoftware, MAX_SOFTWARE_BYTES);
   }
   if (mHasAlternateServer)
   {
      StackLog(<< "Encoding Alternate Server: " << mAlternateServer);
      ptr = encodeAtrAddress(ptr, AlternateServer, mAlternateServer);
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
   if (mHasTurnBandwidth)
   {
      StackLog(<< "Encoding Turn Bandwidth: " << mTurnBandwidth);
      ptr = encodeAtrUInt32(ptr, TurnBandwidth, mTurnBandwidth);
   }   
   if (mCntTurnXorPeerAddress > 0)
   {
      for (int i = 0; i < mCntTurnXorPeerAddress; i++)
      { 
         StackLog(<< "Encoding Turn XorPeerAddress: " << mTurnXorPeerAddress[i]);
         ptr = encodeAtrXorAddress (ptr, TurnXorPeerAddress, mTurnXorPeerAddress[i]);
      }
   }
   if (mHasTurnData)
   {
      StackLog(<< "Encoding TurnData (not shown)");
      ptr = encodeTurnData (ptr, mTurnData);
   }
   if (mHasTurnXorRelayedAddress)
   {
      StackLog(<< "Encoding Turn XorRelayedAddress: " << mTurnXorRelayedAddress);
      ptr = encodeAtrXorAddress (ptr, TurnXorRelayedAddress, mTurnXorRelayedAddress);
   }
   if (mHasTurnEvenPort)
   {
      StackLog(<< "Encoding Turn EvenPort: " << (int)mTurnEvenPort.propType);
      ptr = encodeAtrEvenPort(ptr, mTurnEvenPort);
   }   
   if (mHasTurnRequestedTransport)
   {
      StackLog(<< "Encoding Turn RequestedTransport: " << (int)mTurnRequestedTransport);
      ptr = encodeAtrUInt32(ptr, TurnRequestedTransport, UInt32(mTurnRequestedTransport << 24));
   }   
   if (mHasTurnDontFragment)
   {
      StackLog(<< "Encoding Turn DontFragment: <exists>");
      ptr = encode16(ptr, TurnDontFragment);
      ptr = encode16(ptr, 0);  // 0 attribute length
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
   if (mHasTurnRequestedAddressFamily)
   {
      StackLog(<< "Encoding Turn RequestedAddressFamily: " << mTurnRequestedAddressFamily);
      ptr = encodeAtrUInt32(ptr, TurnRequestedAddressFamily, UInt32(mTurnRequestedAddressFamily << 16));
   }
   if (mHasIcePriority)
   {
      StackLog(<< "Encoding ICE Priority: " << mIcePriority);
      ptr = encodeAtrUInt32(ptr, IcePriority, mIcePriority);
   }
   if (mHasIceUseCandidate)
   {
      StackLog(<< "Encoding ICE UseCandidate: <exists>");
      ptr = encode16(ptr, IceUseCandidate);
      ptr = encode16(ptr, 0); // 0 attribute length
   }
   if (mHasIceControlled)
   {
      StackLog(<< "Encoding ICE Controlled: " << mIceControlledTieBreaker);
      ptr = encodeAtrUInt64(ptr, IceControlled, mIceControlledTieBreaker);
   }
   if (mHasIceControlling)
   {
      StackLog(<< "Encoding ICE Controlling: " << mIceControllingTieBreaker);
      ptr = encodeAtrUInt64(ptr, IceControlling, mIceControllingTieBreaker);
   }

   // Update Length in header now - needed in message integrity calculations
   UInt16 msgSize = ptr - buf - sizeof(StunMsgHdr);
   if(mHasMessageIntegrity) msgSize += 24;  // 4 (attribute header) + 20 (attribute value)
   encode16(lengthp, msgSize);

   if (mHasMessageIntegrity)
   {
      int len = ptr - buf;
      StackLog(<< "Adding message integrity: buffer size=" << len << ", hmacKey=" << mHmacKey.hex());
      StunAtrIntegrity integrity;
      computeHmac(integrity.hash, buf, len, mHmacKey.c_str(), (int)mHmacKey.size());
	   ptr = encodeAtrIntegrity(ptr, integrity);
   }

   // Update Length in header now - may be needed in fingerprint calculations
   if(mHasFingerprint) msgSize += 8;        // 4 (attribute header) + 4 (attribute value)
   encode16(lengthp, msgSize);

   // add finger print if required
   if (mHasFingerprint)
   {
      StackLog(<< "Calculating fingerprint for data of size " << ptr-buf);
      boost::crc_32_type stun_crc;
      stun_crc.process_bytes(buf, ptr-buf); // Calculate CRC across entire message, except the fingerprint attribute
      UInt32 fingerprint = stun_crc.checksum() ^ STUN_CRC_FINAL_XOR;
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
   // !slg! TODO - use newly added rutil/SHA1.hxx class  - will need to add new method to it to support this
   strncpy(hmac,"hmac-not-implemented",20);
}
#else
void
StunMessage::computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   //StackLog(<< "***computeHmac: input='" << Data(input, length).hex() << "', length=" << length << ", key='" << Data(key, sizeKey).hex() << "', keySize=" << sizeKey);

   unsigned int resultSize=20;
   HMAC(EVP_sha1(), 
        key, sizeKey, 
        reinterpret_cast<const unsigned char*>(input), length, 
        reinterpret_cast<unsigned char*>(hmac), &resultSize);
   resip_assert(resultSize == 20);
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
   resip_assert(mUsername);

   if(mRemoteTuple.getAddress().is_v6())
   {
      *mUsername = Data(mRemoteTuple.getAddress().to_v6().to_bytes().data(), mRemoteTuple.getAddress().to_v6().to_bytes().size()).base64encode() + ":";
   }
   else
   {
      *mUsername = Data(mRemoteTuple.getAddress().to_v4().to_bytes().data(), mRemoteTuple.getAddress().to_v4().to_bytes().size()).base64encode() + ":";
   }
   unsigned int port = mRemoteTuple.getPort();
   *mUsername += Data((char*)&port, sizeof(unsigned int)).base64encode() + ":"; 
   *mUsername += resip::Random::getCryptoRandomHex(8) + ":";   // 8 bytes, 64 bits of randomness
   *mUsername += Data((char*)&time, sizeof(time)).hex() + ":";
   char hmac[20];
   computeHmac(hmac, mUsername->data(), (int)mUsername->size(), USERNAME_KEY.data(), (int)USERNAME_KEY.size());
   *mUsername += Data(hmac, sizeof(hmac)).hex();
	
   resip_assert( mUsername->size()%4 == 0 );
   	
   StackLog(<< "computed username=" << *mUsername);

   // Compute Password
   mHasPassword = true;

   if(!mPassword)
   {
      mPassword = new Data;
   }
   resip_assert(mPassword);
   generateShortTermPasswordForUsername(*mPassword);

   StackLog(<< "computed password=" << *mPassword);
}
 
void
StunMessage::generateShortTermPasswordForUsername(Data& password)
{
   char hmac[20];
   resip_assert(mHasUsername && mUsername);
   computeHmac(hmac, mUsername->data(), (int)mUsername->size(), PASSWORD_KEY.data(), (int)PASSWORD_KEY.size());
   password = Data(hmac, sizeof(hmac)).hex();
}

void
StunMessage::getTupleFromUsername(StunTuple& tuple)
{
   resip_assert(mHasUsername);
   resip_assert(mUsername && mUsername->size() >= 92);
   resip_assert(mUsername->size() == 92 || mUsername->size() == 108);

   if(mUsername->size() > 92)  // if over a certain size, then contains IPv6 address
   {
      Data addressPart(Data::Share, mUsername->data(), 24); 
      addressPart = addressPart.base64decode();
      asio::ip::address_v6::bytes_type bytes;
      memcpy(bytes.data(), addressPart.data(), bytes.size());
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
      memcpy(bytes.data(), addressPart.data(), bytes.size());
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
   resip_assert(mHasUsername);

   if(mHasRealm)  // Longterm authenicationmode
   {
      calculateHmacKey(hmacKey, *mUsername, *mRealm, longtermAuthenticationPassword);
   }
   else
   {
      generateShortTermPasswordForUsername(hmacKey);
   }
}

void
StunMessage::calculateHmacKeyForHa1(Data& hmacKey, const Data& ha1)
{
   resip_assert(mHasUsername);

   if(mHasRealm)  // Longterm authenicationmode
   {
      hmacKey = ha1;
   }
   else
   {
      generateShortTermPasswordForUsername(hmacKey);
   }
}

void
StunMessage::calculateHmacKey(Data& hmacKey, const Data& username, const Data& realm, const Data& longtermAuthenticationPassword)
{
   MD5Stream r;
   r << username << ":" << realm << ":" << longtermAuthenticationPassword;
   hmacKey = r.getBin();
  
   StackLog(<< "calculateHmacKey: '" << username << ":" << realm << ":" << longtermAuthenticationPassword << "' = '" << hmacKey.hex() << "'");
}

bool 
StunMessage::checkMessageIntegrity(const Data& hmacKey)
{
   if(mHasMessageIntegrity)
   {
      unsigned char hmac[20];

      // Store original stun message length from mBuffer 
      char *lengthposition = (char*)mBuffer.data() + 2;
      UInt16 originalLength;
      memcpy(&originalLength, lengthposition, 2);

      // Update stun message length in mBuffer for calculation
      UInt16 tempLength = htons(mMessageIntegrityMsgLength);
      memcpy(lengthposition, &tempLength, 2);

      // Calculate HMAC
      int iHMACBufferSize = mMessageIntegrityMsgLength - 24 /* MessageIntegrity size */ + sizeof(StunMsgHdr); // The entire message proceeding the message integrity attribute
      StackLog(<< "Checking message integrity: length=" << mMessageIntegrityMsgLength << ", size=" << iHMACBufferSize << ", hmacKey=" << hmacKey.hex());
      computeHmac((char*)hmac, mBuffer.data(), iHMACBufferSize, hmacKey.c_str(), hmacKey.size());

      // Restore original stun message length in mBuffer
      memcpy(lengthposition, &originalLength, 2);

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
      boost::crc_32_type stun_crc;
      stun_crc.process_bytes(mBuffer.data(), mBuffer.size()-8); // Calculate CRC across entire message, except the fingerprint attribute

      unsigned long crc = stun_crc.checksum() ^ STUN_CRC_FINAL_XOR;
      if(crc == mFingerprint)
      {
         return true;
      }
      else
      {
         WarningLog(<< "Fingerprint=" << mFingerprint << " does not match CRC=" << stun_crc.checksum());
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
