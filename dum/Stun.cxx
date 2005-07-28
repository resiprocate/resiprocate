#include <cassert>
#include <iostream>

#ifdef WIN32
#include <time.h>
#endif

#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/Stun.hxx"

#define RESIPROCATE_SUBSYSTEM Subsystem::TRANSPORT

using namespace resip;
using namespace std;

const UInt16 STUN_MAX_MESSAGE_SIZE = 2048;
const UInt16 STUN_PORT = 3478;

const UInt8  IPv4Family = 0x01;
const UInt8  IPv6Family = 0x02;

// define  flags  
const UInt32 ChangeIpFlag   = 0x04;
const UInt32 ChangePortFlag = 0x02;

// define  stun attribute
const UInt16 MappedAddress    = 0x0001;
const UInt16 ResponseAddress  = 0x0002;
const UInt16 ChangeRequest    = 0x0003;
const UInt16 SourceAddress    = 0x0004;
const UInt16 ChangedAddress   = 0x0005;
const UInt16 Username         = 0x0006;
const UInt16 Password         = 0x0007;
const UInt16 MessageIntegrity = 0x0008;
const UInt16 ErrorCode        = 0x0009;
const UInt16 UnknownAttribute = 0x000A;
const UInt16 ReflectedFrom    = 0x000B;
const UInt16 XorMappedAddress = 0x0020;
const UInt16 XorOnly          = 0x0021;
const UInt16 ServerName       = 0x0022;
const UInt16 SecondaryAddress = 0x0050; // Non standard extention

// define types for a stun message 
const UInt16 BindRequestMsg               = 0x0001;
const UInt16 BindResponseMsg              = 0x0101;
const UInt16 BindErrorResponseMsg         = 0x0111;
const UInt16 SharedSecretRequestMsg       = 0x0002;
const UInt16 SharedSecretResponseMsg      = 0x0102;
const UInt16 SharedSecretErrorResponseMsg = 0x0112;

Stun::Stun(Socket fd,
           UInt32 destAddr,
           UInt16 destPort,
           StunResultSink* sink)
   : mFd(fd),
     mSink(sink),
     mNumTimeout(0),
     mTestICompleted(false),
     mTestI2Completed(false),
     mMappedIpSame(false)
{
   assert(mFd!=INVALID_SOCKET);
   assert(mSink!=0);
   mTestIDest.addr = destAddr;
   mTestIDest.port = destPort;
   memset(&mMappedIp, 0, sizeof(StunAddress4));
   memset(&mTestI2Dest, 0, sizeof(StunAddress4));
}

Stun::~Stun()
{
}

void Stun::buildFdSet(FdSet& fdSet)
{
   fdSet.setRead(mFd);
}

void Stun::process(FdSet& fdSet)
{
   assert(mFd != INVALID_SOCKET);
   bool result = false;
   if (!mTestICompleted || !mTestI2Completed)
   {
      if (mNumTimeout > MAX_NUM_TIMEOUT)
      {
         result = true;
      }
      else
      {
         if (!fdSet.readyToRead(mFd))
         {
            ++mNumTimeout;
            StunAtrString username;
            StunAtrString password;
            username.sizeValue = 0;
            password.sizeValue = 0;
            // NOTE: shared secrect request is not currently support. 

            if (!mTestICompleted ) 
            {
               sendTest(mFd, mTestIDest, username, password, TestI);
            }
			
            if ( mTestICompleted && !mTestI2Completed )
            {
               // check the address to send to if valid.
               if ( mTestI2Dest.addr != 0 && mTestI2Dest.port != 0 )
               {
                  sendTest(mFd, mTestI2Dest, username, password, TestIWithChangedIp);
               }
            }
         }
         else
         {
            char msg[STUN_MAX_MESSAGE_SIZE];
            int msgLen = sizeof(msg);     						
            StunAddress4 from;				
            getMessage(mFd, msg, &msgLen, &from.addr, &from.port);
            StunMessage resp;
            memset(&resp, 0, sizeof(StunMessage));
            parseMessage(msg, msgLen, resp);
						
            switch(resp.msgHdr.id.octet[0])
            {
               case TestI:
               {
                  if (!mTestICompleted)
                  {
                     mMappedIp.addr = resp.mappedAddress.ipv4.addr;
                     mMappedIp.port = resp.mappedAddress.ipv4.port;
                     mTestI2Dest.addr = resp.changedAddress.ipv4.addr;
                     mTestI2Dest.port = mTestIDest.port;
                     mNumTimeout = MAX_NUM_TIMEOUT - mNumTimeout;
                  }
                  mTestICompleted = true;
               } break;
               case TestIWithChangedIp:
               {
                  if (!mTestI2Completed)
                  {
                     StunAddress4 testI2mappedAddr;
                     testI2mappedAddr.addr = resp.mappedAddress.ipv4.addr;
                     testI2mappedAddr.port = resp.mappedAddress.ipv4.port;
                     if ( (testI2mappedAddr.addr  == mMappedIp.addr ) &&
                          (testI2mappedAddr.port == mMappedIp.port ))
                     { 
                        mMappedIpSame = true;
                     }
                  }
                  mTestI2Completed = true;
                  result = true;
               } break;
            }
         }
      }
   }

   if (result)
   {
      processResult();
   }
}

void Stun::processResult()
{
   StunResult result;
   if ( mTestICompleted )
   {
      bool nat;
      Socket s = openPort(0, mMappedIp.addr);
      if ( s != INVALID_SOCKET )
      {
         closeSocket(s);
         nat = false;
      }
      else
      {
         nat = true;
      }
      result.mIp = mMappedIp.addr;
      result.mPort = mMappedIp.port;
      if (nat)
      {
         if ( mMappedIpSame )
         {
            result.mMsg = "Non symmetric";
         }
         else
         {
            result.mSymmetric = true;
            result.mMsg = "Symmetric";
         }

      }
      else
      {
         result.mOpen = true;
         result.mMsg = "No NAT detected";
      }
   }
   else
   {
      result.mBlocked =true;
      result.mMsg = "UDP blocked";
   }
   mSink->onStunResult(result);
}

int Stun::stunRand()
{
   // return 32 bits of random stuff
   assert( sizeof(int) == 4 );
   static bool init=false;
   if ( !init )
   { 
      init = true;
		
      UInt64 tick;
      
#if defined(WIN32) 
      volatile unsigned int lowtick=0,hightick=0;
      __asm
      {
         rdtsc 
            mov lowtick, eax
            mov hightick, edx
            }
      tick = hightick;
      tick <<= 32;
      tick |= lowtick;
#elif defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) )
      asm("rdtsc" : "=A" (tick));
#elif defined (__SUNPRO_CC) || defined( __sparc__ )	
      tick = gethrtime();
#elif defined(__MACH__) 
      int fd=open("/dev/random",O_RDONLY);
      read(fd,&tick,sizeof(tick));
      closesocket(fd);
#else
#     error Need some way to seed the random number generator 
#endif 
      int seed = int(tick);
#ifdef WIN32
      srand(seed);
#else
      srandom(seed);
#endif
   }
	
#ifdef WIN32
   assert( RAND_MAX == 0x7fff );
   int r1 = rand();
   int r2 = rand();
	
   int ret = (r1<<16) + r2;
	
   return ret;
#else
   return random(); 
#endif
}

void Stun::sendTest(Socket fd, 
                    StunAddress4& dest,
                    const StunAtrString& username,
                    const StunAtrString& password,
                    int testNum)
{
   assert( dest.addr != 0 );
   assert( dest.port != 0 );
	
   bool changePort=false;
   bool changeIP=false;
   bool discard=false;
	
   switch (testNum)
   {
      case TestI:
      case TestIWithChangedIp:
         break;
      case TestII:
         changePort=true;
         changeIP=true;
         break;
      case TestIII:
         changePort=true;
         break;
      default:
         return;
   }

   StunMessage req;
   memset(&req, 0, sizeof(StunMessage));
	
   buildReqSimple(&req, username, changePort, changeIP, testNum);
	
   char buf[STUN_MAX_MESSAGE_SIZE];
   int len = STUN_MAX_MESSAGE_SIZE;
	
   len = encodeMessage(req, buf, len, password);
	
   sendMessage(fd, buf, len, dest.addr, dest.port);
	
   // add some delay so the packets don't get sent too quickly 
#ifdef WIN32 // !cj! TODO - should fix this up in windows
		 clock_t now = clock();
		 assert( CLOCKS_PER_SEC == 1000 );
		 while ( clock() <= now+10 ) { };
#else
		 usleep(10*1000);
#endif
}

void Stun::buildReqSimple(StunMessage* msg,
                          const StunAtrString& username,
                          bool changePort,
                          bool changeIp,
                          unsigned int id)
{
   assert( msg );
   memset( msg , 0 , sizeof(*msg) );

   msg->msgHdr.msgType = BindRequestMsg;

   for ( int i=0; i<16; i=i+4 )
   {
      assert(i+3<16);
      int r = stunRand();
      msg->msgHdr.id.octet[i+0]= r>>0;
      msg->msgHdr.id.octet[i+1]= r>>8;
      msg->msgHdr.id.octet[i+2]= r>>16;
      msg->msgHdr.id.octet[i+3]= r>>24;
   }
	
   if ( id != 0 )
   {
      msg->msgHdr.id.octet[0] = id;
   }
	
   msg->hasChangeRequest = true;
   msg->changeRequest.value =(changeIp?ChangeIpFlag:0) | 
      (changePort?ChangePortFlag:0);
	
   if ( username.sizeValue > 0 )
   {
      msg->hasUsername = true;
      msg->username = username;
   }
}

unsigned int Stun::encodeMessage(const StunMessage& msg,
                                 char* buf,
                                 unsigned int bufLen,
                                 const StunAtrString& password)
{
   assert(bufLen >= sizeof(StunMsgHdr));
   char* ptr = buf;
	
   ptr = encode16(ptr, msg.msgHdr.msgType);
   char* lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode(ptr, reinterpret_cast<const char*>(msg.msgHdr.id.octet), sizeof(msg.msgHdr.id));
	
   if (msg.hasMappedAddress)
   {
      ptr = encodeAtrAddress4 (ptr, MappedAddress, msg.mappedAddress);
   }
   if (msg.hasResponseAddress)
   {
      ptr = encodeAtrAddress4(ptr, ResponseAddress, msg.responseAddress);
   }
   if (msg.hasChangeRequest)
   {
      ptr = encodeAtrChangeRequest(ptr, msg.changeRequest);
   }
   if (msg.hasSourceAddress)
   {
      ptr = encodeAtrAddress4(ptr, SourceAddress, msg.sourceAddress);
   }
   if (msg.hasChangedAddress)
   {
      ptr = encodeAtrAddress4(ptr, ChangedAddress, msg.changedAddress);
   }
   if (msg.hasUsername)
   {
      ptr = encodeAtrString(ptr, Username, msg.username);
   }
   if (msg.hasPassword)
   {
      ptr = encodeAtrString(ptr, Password, msg.password);
   }
   if (msg.hasErrorCode)
   {
      ptr = encodeAtrError(ptr, msg.errorCode);
   }
   if (msg.hasUnknownAttributes)
   {
      ptr = encodeAtrUnknown(ptr, msg.unknownAttributes);
   }
   if (msg.hasReflectedFrom)
   {
      ptr = encodeAtrAddress4(ptr, ReflectedFrom, msg.reflectedFrom);
   }
   if (msg.hasXorMappedAddress)
   {
      ptr = encodeAtrAddress4 (ptr, XorMappedAddress, msg.xorMappedAddress);
   }
   if (msg.xorOnly)
   {
      ptr = encodeXorOnly( ptr );
   }
   if (msg.hasServerName)
   {
      ptr = encodeAtrString(ptr, ServerName, msg.serverName);
   }
   if (msg.hasSecondaryAddress)
   {
      ptr = encodeAtrAddress4 (ptr, SecondaryAddress, msg.secondaryAddress);
   }

   if (password.sizeValue > 0)
   {
      StunAtrIntegrity integrity;
      computeHmac(integrity.hash, buf, int(ptr-buf) , password.value, password.sizeValue);
      ptr = encodeAtrIntegrity(ptr, integrity);
   }
	
   encode16(lengthp, UInt16(ptr - buf - sizeof(StunMsgHdr)));
   return int(ptr - buf);
}

char* 
Stun::encode16(char* buf, UInt16 data)
{
   UInt16 ndata = htons(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt16));
   return buf + sizeof(UInt16);
}

char* 
Stun::encode32(char* buf, UInt32 data)
{
   UInt32 ndata = htonl(data);
   memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(UInt32));
   return buf + sizeof(UInt32);
}

char* 
Stun::encode(char* buf, const char* data, unsigned int length)
{
   memcpy(buf, data, length);
   return buf + length;
}

//#ifndef USE_SSL
void
Stun::computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   strncpy(hmac,"hmac-not-implemented",20);
}
/*
#else
#include <openssl/hmac.h>
void
Stun::computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
{
   unsigned int resultSize=0;
   HMAC(EVP_sha1(), 
        key, sizeKey,
        reinterpret_cast<const unsigned char*>(input), length, 
        reinterpret_cast<unsigned char*>(hmac), &resultSize);
   assert(resultSize == 20);
}
#endif
*/

char* 
Stun::encodeAtrAddress4(char* ptr, UInt16 type, const StunAtrAddress4& atr)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 8);
   *ptr++ = atr.pad;
   *ptr++ = IPv4Family;
   ptr = encode16(ptr, atr.ipv4.port);
   ptr = encode32(ptr, atr.ipv4.addr);	
   return ptr;
}

char* 
Stun::encodeAtrChangeRequest(char* ptr, const StunAtrChangeRequest& atr)
{
   ptr = encode16(ptr, ChangeRequest);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr.value);
   return ptr;
}

char* 
Stun::encodeAtrError(char* ptr, const StunAtrError& atr)
{
   ptr = encode16(ptr, ErrorCode);
   ptr = encode16(ptr, 6 + atr.sizeReason);
   ptr = encode16(ptr, atr.pad);
   *ptr++ = atr.errorClass;
   *ptr++ = atr.number;
   ptr = encode(ptr, atr.reason, atr.sizeReason);
   return ptr;
}

char* 
Stun::encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr)
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
Stun::encodeXorOnly(char* ptr)
{
   ptr = encode16(ptr, XorOnly );
   return ptr;
}

char* 
Stun::encodeAtrString(char* ptr, UInt16 type, const StunAtrString& atr)
{
   assert(atr.sizeValue % 4 == 0);
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, atr.sizeValue);
   ptr = encode(ptr, atr.value, atr.sizeValue);
   return ptr;
}

char* 
Stun::encodeAtrIntegrity(char* ptr, const StunAtrIntegrity& atr)
{
   ptr = encode16(ptr, MessageIntegrity);
   ptr = encode16(ptr, 20);
   ptr = encode(ptr, atr.hash, sizeof(atr.hash));
   return ptr;
}

Socket
Stun::openPort(unsigned short port, unsigned int interfaceIp)
{
   Socket fd;
   fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
   if ( fd == INVALID_SOCKET )
   {
      return INVALID_SOCKET;
   }
    
   struct sockaddr_in addr;
   memset((char*) &(addr),0, sizeof((addr)));
   addr.sin_family = AF_INET;
   addr.sin_addr.s_addr = htonl(INADDR_ANY);
   addr.sin_port = htons(port);
    
   if ( (interfaceIp != 0) && ( interfaceIp != 0x100007f ) )
   {
      addr.sin_addr.s_addr = htonl(interfaceIp);
   }
	
   if ( bind( fd,(struct sockaddr*)&addr, sizeof(addr)) != 0 )
   {
      int e = getErrno();
      closeSocket(fd);
      switch (e)
      {
         case 0:
         {
            InfoLog(<< "Could not bind socket" << endl);
            return INVALID_SOCKET;
         }
         case EADDRINUSE:
         {
            InfoLog(<< "Port " << port << " for receiving UDP is in use" << endl);
            return INVALID_SOCKET;
         }
         case EADDRNOTAVAIL:
         {
            InfoLog(<< "Cannot assign requested address" << endl);          
            return INVALID_SOCKET;
         }
         default:
         {
            InfoLog(<< "Could not bind UDP receive port"
                 << "Error=" << e << " " << strerror(e) << endl);
            return INVALID_SOCKET;
         }
      }
   }
   
   assert( fd != INVALID_SOCKET  );
   return fd;
}

bool 
Stun::getMessage(Socket fd, char* buf, int* len,
                 unsigned int* srcIp, unsigned short* srcPort)
{
   assert( fd != INVALID_SOCKET );	
   int originalSize = *len;
   assert( originalSize > 0 );   
   struct sockaddr_in from;
   int fromLen = sizeof(from);	
   *len = recvfrom(fd,
                   buf,
                   originalSize,
                   0,
                   (struct sockaddr *)&from,
                   (socklen_t*)&fromLen);
	
   if ( *len == SOCKET_ERROR )
   {
      int err = getErrno();		
      switch (err)
      {
         case ENOTSOCK:
            InfoLog(<< "Error fd not a socket" << endl);
            break;
         case ECONNRESET:
            InfoLog(<< "Error connection reset - host not reachable" << endl);
            break;
         default:
            InfoLog(<< "Socket Error=" << err << endl);
      }
      return false;
   }
	
   if ( *len < 0 )
   {
      InfoLog(<< "socket closed? negative len" << endl);
      return false;
   }
    
   if ( *len == 0 )
   {
      InfoLog(<< "socket closed? zero len" << endl);
      return false;
   }
    
   *srcPort = ntohs(from.sin_port);
   *srcIp = ntohl(from.sin_addr.s_addr);
	
   if ( (*len)+1 >= originalSize )
   {
      InfoLog(<< "Received a message that was too large" << endl);
      return false;
   }
   buf[*len]=0;    
   return true;
}

bool 
Stun::sendMessage(Socket fd, char* buf, int l, 
                  unsigned int dstIp, unsigned short dstPort)
{
   assert( fd != INVALID_SOCKET );
    
   int s;
   if ( dstPort == 0 )
   {
      // sending on a connected port 
      assert( dstIp == 0 );
      s = send(fd,buf,l,0);
   }
   else
   {
      assert( dstIp != 0 );
      assert( dstPort != 0 );
        
      struct sockaddr_in to;
      int toLen = sizeof(to);
      memset(&to,0,toLen);
        
      to.sin_family = AF_INET;
      to.sin_port = htons(dstPort);
      to.sin_addr.s_addr = htonl(dstIp);
        
      s = sendto(fd, buf, l, 0,(sockaddr*)&to, toLen);
   }
    
   if ( s == SOCKET_ERROR )
   {
      int e = getErrno();
      switch (e)
      {
         case ECONNREFUSED:
         case EHOSTDOWN:
         case EHOSTUNREACH:
         {
            // quietly ignore this 
         }
         break;
         case EAFNOSUPPORT:
         {
            InfoLog(<< "err EAFNOSUPPORT in send" << endl);
         }
         break;
         default:
         {
            InfoLog(<< "err " << e << " "  << strerror(e) << " in send" << endl);
         }
      }
      return false;
   }
    
   if ( s == 0 )
   {
      InfoLog(<< "no data sent in send" << endl);
      return false;
   }
    
   if ( s != l )
   {
      InfoLog(<< "only " << s << " out of " << l << " bytes sent" << endl);
      return false;
   }
    
   return true;
}

bool
Stun::parseMessage(char* buf, unsigned int bufLen, StunMessage& msg)
{
   InfoLog(<< "Received stun message: " << bufLen << " bytes" << endl);
   memset(&msg, 0, sizeof(msg));
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
      InfoLog(<< "Bad message" << endl);
      return false;
   }
	
   memcpy(&msg.msgHdr, buf, sizeof(StunMsgHdr));
   msg.msgHdr.msgType = ntohs(msg.msgHdr.msgType);
   msg.msgHdr.msgLength = ntohs(msg.msgHdr.msgLength);
	
   if (msg.msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen)
   {
      InfoLog(<< "Message header length doesn't match message size: " << msg.msgHdr.msgLength << " - " << bufLen << endl);
      return false;
   }
	
   char* body = buf + sizeof(StunMsgHdr);
   unsigned int size = msg.msgHdr.msgLength;
	
   while ( size > 0 )
   {
      // !jf! should check that there are enough bytes left in the buffer
		
      StunAtrHdr* attr = reinterpret_cast<StunAtrHdr*>(body);
		
      unsigned int attrLen = ntohs(attr->length);
      int atrType = ntohs(attr->type);
		
      if ( attrLen+4 > size ) 
      {
         InfoLog(<< "claims attribute is larger than size of message " <<"(attribute type="<<atrType<<")"<< endl);
         return false;
      }
		
      body += 4; // skip the length and type in attribute header
      size -= 4;
		
      switch ( atrType )
      {
         case MappedAddress:
            msg.hasMappedAddress = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.mappedAddress )== false )
            {
               InfoLog(<< "problem parsing MappedAddress" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "MappedAddress = " << msg.mappedAddress.ipv4.addr << ":" << msg.mappedAddress.ipv4.port << endl);
            }				
            break;
         case ResponseAddress:
            msg.hasResponseAddress = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.responseAddress )== false )
            {
               InfoLog(<< "problem parsing ResponseAddress" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "ResponseAddress = " << msg.mappedAddress.ipv4.addr << ":" << msg.mappedAddress.ipv4.port << endl);
            }
            break;
         case ChangeRequest:
            msg.hasChangeRequest = true;
            if (parseAtrChangeRequest( body, attrLen, msg.changeRequest) == false)
            {
               InfoLog(<< "problem parsing ChangeRequest" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "ChangeRequest = " << msg.changeRequest.value << endl);
            }
            break;
         case SourceAddress:
            msg.hasSourceAddress = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.sourceAddress )== false )
            {
               InfoLog(<< "problem parsing SourceAddress" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "SourceAddress = " << msg.mappedAddress.ipv4.addr << ":" << msg.mappedAddress.ipv4.port << endl);
            }
            break;
         case ChangedAddress:
            msg.hasChangedAddress = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.changedAddress )== false )
            {
               InfoLog(<< "problem parsing ChangedAddress" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "ChangedAddress = " << msg.mappedAddress.ipv4.addr << ":" << msg.mappedAddress.ipv4.port << endl);
            }
            break;
         case Username:
            msg.hasUsername = true;
            if (parseAtrString( body, attrLen, msg.username) == false)
            {
               InfoLog(<< "problem parsing Username" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "Username = " << msg.username.value << endl);
            }
            break;
         case Password:
            msg.hasPassword = true;
            if (parseAtrString( body, attrLen, msg.password) == false)
            {
               InfoLog(<< "problem parsing Password" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "Password = " << msg.password.value << endl);
            }
            break;				
         case MessageIntegrity:
            msg.hasMessageIntegrity = true;
            if (parseAtrIntegrity( body, attrLen, msg.messageIntegrity) == false)
            {
               InfoLog(<< "problem parsing MessageIntegrity" << endl);
               return false;
            }
            else
            {
               //if (verbose) clog << "MessageIntegrity = " << msg.messageIntegrity.hash << endl;
            }
					
            // read the current HMAC
            // look up the password given the user of given the transaction id 
            // compute the HMAC on the buffer
            // decide if they match or not
            break;
				
         case ErrorCode:
            msg.hasErrorCode = true;
            if (parseAtrError(body, attrLen, msg.errorCode) == false)
            {
               InfoLog(<< "problem parsing ErrorCode" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "ErrorCode = " << int(msg.errorCode.errorClass) 
                       << " " << int(msg.errorCode.number) 
                       << " " << msg.errorCode.reason << endl);
            }					
            break;
				
         case UnknownAttribute:
            msg.hasUnknownAttributes = true;
            if (parseAtrUnknown(body, attrLen, msg.unknownAttributes) == false)
            {
               InfoLog(<< "problem parsing UnknownAttribute" << endl);
               return false;
            }
            break;
				
         case ReflectedFrom:
            msg.hasReflectedFrom = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.reflectedFrom ) == false )
            {
               InfoLog(<< "problem parsing ReflectedFrom" << endl);
               return false;
            }
            break;  
				
         case XorMappedAddress:
            msg.hasXorMappedAddress = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.xorMappedAddress ) == false )
            {
               InfoLog(<< "problem parsing XorMappedAddress" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "XorMappedAddress = " << msg.mappedAddress.ipv4.addr << ":" << msg.mappedAddress.ipv4.port << endl);
            }
            break;  

         case XorOnly:
            msg.xorOnly = true;
            InfoLog(<< "xorOnly = true" << endl);
            break;  
				
         case ServerName: 
            msg.hasServerName = true;
            if (parseAtrString( body, attrLen, msg.serverName) == false)
            {
               InfoLog(<< "problem parsing ServerName" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "ServerName = " << msg.serverName.value << endl);
            }
            break;
				
         case SecondaryAddress:
            msg.hasSecondaryAddress = true;
            if ( parseAtrAddress(  body,  attrLen,  msg.secondaryAddress ) == false )
            {
               InfoLog(<< "problem parsing secondaryAddress" << endl);
               return false;
            }
            else
            {
               InfoLog(<< "SecondaryAddress = " << msg.mappedAddress.ipv4.addr << ":" << msg.mappedAddress.ipv4.port << endl);
            }
            break;  
					
         default:
            InfoLog(<< "Unknown attribute: " << atrType << endl);
            if ( atrType <= 0x7FFF ) 
            {
               return false;
            }
      }
		
      body += attrLen;
      size -= attrLen;
   }
    
   return true;
}

bool 
Stun::parseAtrAddress( char* body, unsigned int hdrLen,  StunAtrAddress4& result )
{
   if ( hdrLen != 8 )
   {
      InfoLog(<< "hdrLen wrong for Address" <<endl);
      return false;
   }
   result.pad = *body++;
   result.family = *body++;
   if (result.family == IPv4Family)
   {
      UInt16 nport;
      memcpy(&nport, body, 2); body+=2;
      result.ipv4.port = ntohs(nport);
		
      UInt32 naddr;
      memcpy(&naddr, body, 4); body+=4;
      result.ipv4.addr = ntohl(naddr);
      return true;
   }
   else if (result.family == IPv6Family)
   {
      InfoLog(<< "ipv6 not supported" << endl);
   }
   else
   {
      InfoLog(<< "bad address family: " << result.family << endl);
   }
	
   return false;
}

bool 
Stun::parseAtrChangeRequest( char* body, unsigned int hdrLen,  StunAtrChangeRequest& result )
{
   if ( hdrLen != 4 )
   {
      InfoLog(<< "hdr length = " << hdrLen << " expecting " << sizeof(result) << endl);
		InfoLog(<< "Incorrect size for ChangeRequest" << endl);
      return false;
   }
   else
   {
      memcpy(&result.value, body, 4);
      result.value = ntohl(result.value);
      return true;
   }
}

bool 
Stun::parseAtrError( char* body, unsigned int hdrLen,  StunAtrError& result )
{
   if ( hdrLen >= sizeof(result) )
   {
      InfoLog(<< "head on Error too large" << endl);
      return false;
   }
   else
   {
      memcpy(&result.pad, body, 2); body+=2;
      result.pad = ntohs(result.pad);
      result.errorClass = *body++;
      result.number = *body++;
		
      result.sizeReason = hdrLen - 4;
      memcpy(&result.reason, body, result.sizeReason);
      result.reason[result.sizeReason] = 0;
      return true;
   }
}

bool 
Stun::parseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown& result )
{
   if ( hdrLen >= sizeof(result) )
   {
      InfoLog(<< "head on Unknown too large" << endl);
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
Stun::parseAtrString( char* body, unsigned int hdrLen,  StunAtrString& result )
{
   if ( hdrLen >= STUN_MAX_STRING )
   {
      InfoLog(<< "String is too large" << endl);
      return false;
   }
   else
   {
      if (hdrLen % 4 != 0)
      {
         InfoLog(<< "Bad length string " << hdrLen << endl);
         return false;
      }
		
      result.sizeValue = hdrLen;
      memcpy(&result.value, body, hdrLen);
      result.value[hdrLen] = 0;
      return true;
   }
}


bool 
Stun::parseAtrIntegrity( char* body, unsigned int hdrLen,  StunAtrIntegrity& result )
{
   if ( hdrLen != 20)
   {
      InfoLog(<< "MessageIntegrity must be 20 bytes" << endl);
      return false;
   }
   else
   {
      memcpy(&result.hash, body, hdrLen);
      return true;
   }
}
/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
