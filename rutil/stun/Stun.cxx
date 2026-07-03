#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "rutil/ResipAssert.h"
#include <cstring>
#include <iostream>
#include <cstdlib>
#include <errno.h>
#include <ctype.h>

#ifdef WIN32
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#else

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>

#ifndef __CYGWIN__
#include <arpa/nameser.h>
#include <resolv.h>
#endif

#include <net/if.h>

#if defined (__SUNPRO_CC) || defined (__sun__)
#include <sys/sockio.h> // solaris only?
#endif

#endif

#include "Udp.hxx"
#include "Stun.hxx"
#include "rutil/Socket.hxx"
#include "rutil/Random.hxx"
#ifndef USE_SSL
#include "rutil/Sha1.hxx"
#endif
#include "rutil/WinLeakCheck.hxx"

using namespace std;

namespace resip
{

   static bool
      stunParseAtrAddress(char* body, unsigned int hdrLen, StunAtrAddress4& result)
   {
      if (hdrLen != 8)
      {
         // clog << "hdrLen wrong for Address" <<endl;
         return false;
      }
      result.pad = *body++;
      result.family = *body++;
      if (result.family == IPv4Family)
      {
         uint16_t nport;
         memcpy(&nport, body, 2); body += 2;
         result.ipv4.port = ntohs(nport);

         uint32_t naddr;
         memcpy(&naddr, body, 4); body += 4;
         result.ipv4.addr = ntohl(naddr);
         return true;
      }
      else if (result.family == IPv6Family)
      {
         //clog << "ipv6 not supported" << endl;
      }
      else
      {
         //clog << "bad address family: " << result.family << endl;
      }

      return false;
   }

   static bool
      stunParseUInt32(char* body, unsigned int hdrLen, uint32_t& result)
   {
      if (hdrLen != 4)
      {
         return false;
      }
      else
      {
         uint32_t tmp;
         memcpy(&tmp, body, 4);
         result = ntohl(tmp);
         return true;
      }
   }

   static bool
      stunParseAtrChangeRequest(char* body, unsigned int hdrLen, StunAtrChangeRequest& result)
   {
      if (hdrLen != 4)
      {
         //clog << "hdr length = " << hdrLen << " expecting " << sizeof(result) << endl;

         //clog << "Incorrect size for ChangeRequest" << endl;
         return false;
      }
      else
      {
         memcpy(&result.value, body, 4);
         result.value = ntohl(result.value);
         return true;
      }
   }

   static bool
      stunParseAtrError(char* body, unsigned int hdrLen, StunAtrError& result)
   {
      if (hdrLen >= (sizeof(result) - sizeof(result.sizeReason))) // Note: result.sizeReason is extra info in StunAtrError that is not on the wire
      {
         //clog << "head on Error too large" << endl;
         return false;
      }
      else if (hdrLen < 4)  // check for min len
      {
         return false;
      }
      else
      {
         memcpy(&result.pad, body, 2); body += 2;
         result.pad = ntohs(result.pad);
         result.errorClass = *body++;
         result.number = *body++;

         result.sizeReason = hdrLen - 4;
         memcpy(&result.reason, body, result.sizeReason);
         result.reason[result.sizeReason] = 0;
         return true;
      }
   }

   static bool
      stunParseAtrUnknown(char* body, unsigned int hdrLen, StunAtrUnknown& result)
   {
      if (hdrLen >= sizeof(result))
      {
         return false;
      }
      else
      {
         if (hdrLen % 4 != 0) return false;
         result.numAttributes = hdrLen / 4;
         for (int i = 0; i < result.numAttributes; i++)
         {
            memcpy(&result.attrType[i], body, 2); body += 2;
            result.attrType[i] = ntohs(result.attrType[i]);
         }
         return true;
      }
   }


   static bool
      stunParseAtrString(char* body, unsigned int hdrLen, StunAtrString& result)
   {
      if (hdrLen >= STUN_MAX_STRING)
      {
         //clog << "String is too large" << endl;
         return false;
      }
      else
      {
         // !jr! Removed - newer RFCs don't require individual attributes to have this
         // 4 byte boundary restriction.  (Note the overall structure of the 
         // stun message attributes must still be padded to 4 byte boundaries)
         //if (hdrLen % 4 != 0)
         //{
         //   //clog << "Bad length string " << hdrLen << endl;
         //   return false;
         //}

         result.sizeValue = hdrLen;
         memcpy(&result.value, body, hdrLen);
         result.value[hdrLen] = 0;
         return true;
      }
   }

   static bool
      stunParseAtrUInt32(char* body, unsigned int hdrLen, uint32_t& result)
   {
      if (hdrLen != 4)
      {
         // clog << "hdrLen wrong for UInt32 attribute";
         return false;
      }
      else
      {
         memcpy(&result, body, 4);
         result = ntohl(result);
         return true;
      }
   }


   static bool
      stunParseAtrIntegrity(char* body, unsigned int hdrLen, StunAtrIntegrity& result)
   {
      if (hdrLen != 20)
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
      stunParseMessage(char* buf, unsigned int bufLen, StunMessage& msg, bool verbose)
   {
      if (verbose) clog << "Received stun message: " << bufLen << " bytes" << endl;

      if (sizeof(StunMsgHdr) > bufLen)
      {
         clog << "Bad message" << endl;
         return false;
      }

      memcpy(&msg.msgHdr, buf, sizeof(StunMsgHdr));
      msg.msgHdr.msgType = ntohs(msg.msgHdr.msgType);
      msg.msgHdr.msgLength = ntohs(msg.msgHdr.msgLength);

      if (msg.msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen)
      {
         clog << "Message header length doesn't match message size: " << msg.msgHdr.msgLength << " - " << bufLen << endl;
         return false;
      }

      char* body = buf + sizeof(StunMsgHdr);
      unsigned int size = msg.msgHdr.msgLength;

      if (verbose) clog << "bytes after header = " << size << endl;

      while (size > 0)
      {
         if (size < sizeof(StunAtrHdr))
         {
            return false;
         }

         StunAtrHdr* attr = reinterpret_cast<StunAtrHdr*>(body);

         unsigned int attrLen = ntohs(attr->length);

         // attrLen may not be on 4 byte boundary, in which case we need to pad to 4 bytes when advancing to next attribute
         unsigned int attrLenPad = attrLen % 4 == 0 ? 0 : 4 - (attrLen % 4);

         int atrType = ntohs(attr->type);

         if (verbose) clog << "Found attribute type=" << atrType << " length=" << attrLen << endl;
         if (attrLen + attrLenPad + 4 > size)
         {
            clog << "claims attribute is larger than size of message " << "(attribute type=" << atrType << ")" << endl;
            return false;
         }

         body += 4; // skip the length and type in attribute header
         size -= 4;

         switch (atrType)
         {
            case MappedAddress:
               msg.hasMappedAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.mappedAddress) == false)
               {
                  clog << "problem parsing MappedAddress" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "MappedAddress = " << msg.mappedAddress.ipv4 << endl;
               }

               break;

            case ResponseAddress:
               msg.hasResponseAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.responseAddress) == false)
               {
                  if (verbose) clog << "problem parsing ResponseAddress" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "ResponseAddress = " << msg.responseAddress.ipv4 << endl;
               }
               break;

            case ChangeRequest:
               msg.hasChangeRequest = true;
               if (stunParseAtrChangeRequest(body, attrLen, msg.changeRequest) == false)
               {
                  if (verbose) clog << "problem parsing ChangeRequest" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "ChangeRequest = " << msg.changeRequest.value << endl;
               }
               break;

            case SourceAddress:
               msg.hasSourceAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.sourceAddress) == false)
               {
                  if (verbose) clog << "problem parsing SourceAddress" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "SourceAddress = " << msg.sourceAddress.ipv4 << endl;
               }
               break;

            case ChangedAddress:
               msg.hasChangedAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.changedAddress) == false)
               {
                  if (verbose) clog << "problem parsing ChangedAddress" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "ChangedAddress = " << msg.changedAddress.ipv4 << endl;
               }
               break;

            case Username:
               msg.hasUsername = true;
               if (stunParseAtrString(body, attrLen, msg.username) == false)
               {
                  if (verbose) clog << "problem parsing Username" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "Username = " << msg.username.value << endl;
               }

               break;

            case Password:
               msg.hasPassword = true;
               if (stunParseAtrString(body, attrLen, msg.password) == false)
               {
                  if (verbose) clog << "problem parsing Password" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "Password = " << msg.password.value << endl;
               }
               break;

            case MessageIntegrity:
               msg.hasMessageIntegrity = true;
               // Record where the MESSAGE-INTEGRITY attribute header begins so the
               // HMAC can be verified later (body currently points just past the
               // 4-byte attribute header). See stunVerifyMessageIntegrity().
               msg.messageIntegrityOffset = static_cast<unsigned int>((body - 4) - buf);
               if (stunParseAtrIntegrity(body, attrLen, msg.messageIntegrity) == false)
               {
                  if (verbose) clog << "problem parsing MessageIntegrity" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "MessageIntegrity = " << msg.messageIntegrity.hash << endl;
               }
               break;

            case ErrorCode:
               msg.hasErrorCode = true;
               if (stunParseAtrError(body, attrLen, msg.errorCode) == false)
               {
                  if (verbose) clog << "problem parsing ErrorCode" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "ErrorCode = " << int(msg.errorCode.errorClass)
                     << " " << int(msg.errorCode.number)
                     << " " << msg.errorCode.reason << endl;
               }

               break;

            case UnknownAttribute:
               msg.hasUnknownAttributes = true;
               if (stunParseAtrUnknown(body, attrLen, msg.unknownAttributes) == false)
               {
                  if (verbose) clog << "problem parsing UnknownAttribute" << endl;
                  return false;
               }
               break;

            case ReflectedFrom:
               msg.hasReflectedFrom = true;
               if (stunParseAtrAddress(body, attrLen, msg.reflectedFrom) == false)
               {
                  if (verbose) clog << "problem parsing ReflectedFrom" << endl;
                  return false;
               }
               break;

            case Realm:
               if (!msg.hasRealm)
               {
                  if (attrLen > MAX_REALM_BYTES)
                  {
                     if (verbose) clog << "Realm length=" << attrLen << " is longer than max allowed=" << MAX_REALM_BYTES << endl;
                     return false;
                  }
                  msg.hasRealm = true;
                  msg.realm = new resip::Data(resip::Data::Share, body, attrLen);
                  if (verbose) clog << "Realm = " << *msg.realm << endl;
               }
               else
               {
                  if (verbose) clog << "Duplicate Realm in message - ignoring." << endl;
               }
               break;

            case Nonce:
               if (!msg.hasNonce)
               {
                  if (attrLen > MAX_NONCE_BYTES)
                  {
                     if (verbose) clog << "Nonce length=" << attrLen << " is longer than max allowed=" << MAX_NONCE_BYTES << endl;
                     return false;
                  }
                  msg.hasNonce = true;
                  msg.nonce = new resip::Data(resip::Data::Share, body, attrLen);
                  if (verbose) clog << "Nonce = " << *msg.nonce << endl;
               }
               else
               {
                  if (verbose) clog << "Duplicate Nonce in message - ignoring." << endl;
               }
               break;

            case XorMappedAddress:
            case XorMappedAddress_old:
               msg.hasXorMappedAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.xorMappedAddress) == false)
               {
                  if (verbose) clog << "problem parsing XorMappedAddress" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "XorMappedAddress = " << msg.xorMappedAddress.ipv4 << endl;
               }
               break;

            case XorOnly:
               msg.xorOnly = true;
               if (verbose)
               {
                  if (verbose) clog << "xorOnly = true" << endl;
               }
               break;

            case Software:
               // case ServerName: // Was non-standard "ServerName" attribute(0x8022) now standardardized as SOFTWARE(0x8022)
               msg.hasSoftware = true;
               if (stunParseAtrString(body, attrLen, msg.software) == false)
               {
                  if (verbose) clog << "problem parsing ServerName" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "ServerName = " << msg.software.value << endl;
               }
               break;

            case AlternateServer:
               if (!msg.hasAlternateServer)
               {
                  msg.hasAlternateServer = true;
                  if (stunParseAtrAddress(body, attrLen, msg.alternateServer) == false)
                  {
                     if (verbose) clog << "problem parsing AlternateServer" << endl;
                     return false;
                  }
                  if (verbose) clog << "AlternateServer = " << msg.alternateServer.ipv4;
               }
               else
               {
                  if (verbose) clog << "Duplicate AlternateServer in message - ignoring." << endl;
               }
               break;

            case Fingerprint:
               if (!msg.hasFingerprint)
               {
                  msg.hasFingerprint = true;
                  if (stunParseAtrUInt32(body, attrLen, msg.fingerprint) == false)
                  {
                     if (verbose) clog << "problem parsing Fingerprint" << endl;
                     return false;
                  }
                  if (verbose) clog << "Fingerprint = " << msg.fingerprint;
               }
               else
               {
                  if (verbose) clog << "Duplicate Fingerprint in message - ignoring." << endl;
               }
               break;


            case SecondaryAddress:
               msg.hasSecondaryAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.secondaryAddress) == false)
               {
                  if (verbose) clog << "problem parsing secondaryAddress" << endl;
                  return false;
               }
               else
               {
                  if (verbose) clog << "SecondaryAddress = " << msg.secondaryAddress.ipv4 << endl;
               }
               break;

               // End STUN attributes

                // TURN attributes            
            case TurnLifetime:
               msg.hasTurnLifetime = true;
               if (stunParseUInt32(body, attrLen, msg.turnLifetime) == false)
               {
                  return false;
               }
               break;

            case TurnAlternateServer:
               msg.hasTurnAlternateServer = true;
               if (stunParseAtrAddress(body, attrLen, msg.turnAlternateServer) == false)
               {
                  return false;
               }
               break;

            case TurnMagicCookie:
               msg.hasTurnMagicCookie = true;
               if (stunParseUInt32(body, attrLen, msg.turnMagicCookie) == false)
               {
                  return false;
               }
               break;

            case TurnBandwidth:
               msg.hasTurnBandwidth = true;
               if (stunParseUInt32(body, attrLen, msg.turnBandwidth) == false)
               {
                  return false;
               }
               break;

            case TurnDestinationAddress:
               msg.hasTurnDestinationAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.turnDestinationAddress) == false)
               {
                  return false;
               }
               break;

            case TurnRemoteAddress:
               msg.hasTurnRemoteAddress = true;
               if (stunParseAtrAddress(body, attrLen, msg.turnRemoteAddress) == false)
               {
                  return false;
               }
               break;

               //overlay on parse, ownership is buffer parsed from
            case TurnData:
               msg.hasTurnData = true;
               msg.turnData = new resip::Data(resip::Data::Share, body, attrLen);
               break;

               //case TurnNonce:
               //break;
               //case TurnRealm:
               //break;

            default:
               if (verbose) clog << "Unknown attribute: " << atrType << endl;
               if (atrType <= 0x7FFF)
               {
                  return false;
               }
         }

         body += attrLen + attrLenPad;
         size -= attrLen + attrLenPad;
      }

      return true;
   }

   static char*
      encode16(char* buf, uint16_t data)
   {
      uint16_t ndata = htons(data);
      memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(uint16_t));
      return buf + sizeof(uint16_t);
   }

   static char*
      encode32(char* buf, uint32_t data)
   {
      uint32_t ndata = htonl(data);
      memcpy(buf, reinterpret_cast<void*>(&ndata), sizeof(uint32_t));
      return buf + sizeof(uint32_t);
   }


   static char*
      encode(char* buf, const char* data, unsigned int length)
   {
      memcpy(buf, data, length);
      return buf + length;
   }

   static char*
      encodeTurnData(char* ptr, const resip::Data* td)
   {
      ptr = encode16(ptr, TurnData);
      ptr = encode16(ptr, (uint16_t)td->size());
      memcpy(ptr, td->data(), td->size());
      ptr += td->size();

      return ptr;
   }

   static char*
      encodeAtrUInt32(char* ptr, uint16_t type, uint32_t value)
   {
      ptr = encode16(ptr, type);
      ptr = encode16(ptr, 4);
      ptr = encode32(ptr, value);
      return ptr;
   }

   static char*
      encodeAtrAddress4(char* ptr, uint16_t type, const StunAtrAddress4& atr)
   {
      ptr = encode16(ptr, type);
      ptr = encode16(ptr, 8);
      *ptr++ = atr.pad;
      *ptr++ = IPv4Family;
      ptr = encode16(ptr, atr.ipv4.port);
      ptr = encode32(ptr, atr.ipv4.addr);

      return ptr;
   }

   static char*
      encodeAtrChangeRequest(char* ptr, const StunAtrChangeRequest& atr)
   {
      ptr = encode16(ptr, ChangeRequest);
      ptr = encode16(ptr, 4);
      ptr = encode32(ptr, atr.value);
      return ptr;
   }

   static char*
      encodeMagicCookie(char* ptr, const uint32_t& cookie)
   {
      ptr = encode16(ptr, TurnMagicCookie);
      ptr = encode16(ptr, 4);
      ptr = encode32(ptr, cookie);
      return ptr;
   }


   static char*
      encodeAtrError(char* ptr, const StunAtrError& atr)
   {
      ptr = encode16(ptr, ErrorCode);
      ptr = encode16(ptr, 4 + atr.sizeReason);
      ptr = encode16(ptr, atr.pad);
      *ptr++ = atr.errorClass;
      *ptr++ = atr.number;
      ptr = encode(ptr, atr.reason, atr.sizeReason);
      return ptr;
   }


   static char*
      encodeAtrUnknown(char* ptr, const StunAtrUnknown& atr)
   {
      ptr = encode16(ptr, UnknownAttribute);
      ptr = encode16(ptr, 2 + 2 * atr.numAttributes);
      for (int i = 0; i < atr.numAttributes; i++)
      {
         ptr = encode16(ptr, atr.attrType[i]);
      }
      return ptr;
   }


   static char*
      encodeXorOnly(char* ptr)
   {
      ptr = encode16(ptr, XorOnly);
      return ptr;
   }


   static char*
      encodeAtrString(char* ptr, uint16_t type, const StunAtrString& atr)
   {
      resip_assert(atr.sizeValue % 4 == 0);

      ptr = encode16(ptr, type);
      ptr = encode16(ptr, atr.sizeValue);
      ptr = encode(ptr, atr.value, atr.sizeValue);
      return ptr;
   }


   static char*
      encodeAtrIntegrity(char* ptr, const StunAtrIntegrity& atr)
   {
      ptr = encode16(ptr, MessageIntegrity);
      ptr = encode16(ptr, 20);
      ptr = encode(ptr, atr.hash, sizeof(atr.hash));
      return ptr;
   }


   unsigned int
      stunEncodeMessage(const StunMessage& msg,
         char* buf,
         unsigned int bufLen,
         const StunAtrString& password,
         bool verbose)
   {
      resip_assert(bufLen >= sizeof(StunMsgHdr));
      char* ptr = buf;

      if (verbose) clog << "Encoding stun message: " << endl;

      ptr = encode16(ptr, msg.msgHdr.msgType);
      char* lengthp = ptr;
      ptr = encode16(ptr, 0);
      ptr = encode(ptr, reinterpret_cast<const char*>(msg.msgHdr.id.octet), sizeof(msg.msgHdr.id));

      if (msg.hasTurnMagicCookie)
      {
         if (verbose) clog << "Encoding TurnMagicCookie: " << msg.turnMagicCookie << endl;
         ptr = encodeMagicCookie(ptr, msg.turnMagicCookie);
      }
      if (msg.hasTurnDestinationAddress)
      {
         if (verbose) clog << "Encoding TurnDestinationAddress: " << msg.turnDestinationAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, TurnDestinationAddress, msg.turnDestinationAddress);
      }
      if (msg.hasMappedAddress)
      {
         if (verbose) clog << "Encoding MappedAddress: " << msg.mappedAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, MappedAddress, msg.mappedAddress);
      }
      if (msg.hasResponseAddress)
      {
         if (verbose) clog << "Encoding ResponseAddress: " << msg.responseAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, ResponseAddress, msg.responseAddress);
      }
      if (msg.hasTurnAlternateServer)
      {
         if (verbose) clog << "Encoding AlternateServer: " << msg.turnAlternateServer.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, TurnAlternateServer, msg.turnAlternateServer);
      }
      if (msg.hasChangeRequest)
      {
         if (verbose) clog << "Encoding ChangeRequest: " << msg.changeRequest.value << endl;
         ptr = encodeAtrChangeRequest(ptr, msg.changeRequest);
      }
      if (msg.hasSourceAddress)
      {
         if (verbose) clog << "Encoding SourceAddress: " << msg.sourceAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, SourceAddress, msg.sourceAddress);
      }
      if (msg.hasChangedAddress)
      {
         if (verbose) clog << "Encoding ChangedAddress: " << msg.changedAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, ChangedAddress, msg.changedAddress);
      }
      if (msg.hasUsername)
      {
         if (verbose) clog << "Encoding Username: " << msg.username.value << endl;
         ptr = encodeAtrString(ptr, Username, msg.username);
      }
      if (msg.hasPassword)
      {
         if (verbose) clog << "Encoding Password: " << msg.password.value << endl;
         ptr = encodeAtrString(ptr, Password, msg.password);
      }
      if (msg.hasErrorCode)
      {
         if (verbose) clog << "Encoding ErrorCode: class="
            << int(msg.errorCode.errorClass)
            << " number=" << int(msg.errorCode.number)
            << " reason="
            << msg.errorCode.reason
            << endl;

         ptr = encodeAtrError(ptr, msg.errorCode);
      }
      if (msg.hasUnknownAttributes)
      {
         if (verbose) clog << "Encoding UnknownAttribute: ???" << endl;
         ptr = encodeAtrUnknown(ptr, msg.unknownAttributes);
      }
      if (msg.hasReflectedFrom)
      {
         if (verbose) clog << "Encoding ReflectedFrom: " << msg.reflectedFrom.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, ReflectedFrom, msg.reflectedFrom);
      }
      if (msg.hasXorMappedAddress)
      {
         if (verbose) clog << "Encoding XorMappedAddress: " << msg.xorMappedAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, XorMappedAddress, msg.xorMappedAddress);
      }
      if (msg.xorOnly)
      {
         if (verbose) clog << "Encoding xorOnly: " << endl;
         ptr = encodeXorOnly(ptr);
      }
      if (msg.hasSoftware)
      {
         if (verbose) clog << "Encoding Software: " << msg.software.value << endl;
         ptr = encodeAtrString(ptr, Software, msg.software);
      }
      if (msg.hasSecondaryAddress)
      {
         if (verbose) clog << "Encoding SecondaryAddress: " << msg.secondaryAddress.ipv4 << endl;
         ptr = encodeAtrAddress4(ptr, SecondaryAddress, msg.secondaryAddress);
      }
      if (msg.hasTurnLifetime)
      {
         if (verbose) clog << "Encoding Turn Lifetime: " << msg.turnLifetime << endl;
         ptr = encodeAtrUInt32(ptr, TurnLifetime, msg.turnLifetime);
      }
      if (msg.hasTurnBandwidth)
      {
         if (verbose) clog << "Encoding Turn Bandwidth: " << msg.turnBandwidth << endl;
         ptr = encodeAtrUInt32(ptr, TurnBandwidth, msg.turnBandwidth);
      }
      if (msg.hasTurnData)
      {
         if (verbose) clog << "Encoding TurnData (not shown)" << endl;
         ptr = encodeTurnData(ptr, msg.turnData);
      }
      if (password.sizeValue > 0)
      {
         if (verbose) clog << "HMAC with password: " << password.value << endl;

         // allocate space for message integrity attribute (hash + attribute type + size)
         char* ptrMessageIntegrity = ptr;
         ptr += 20 + sizeof(MessageIntegrity) + sizeof(uint16_t);
         encode16(lengthp, uint16_t(ptr - buf - sizeof(StunMsgHdr)));

         StunAtrIntegrity integrity;
         // RFC 5389 section 15.4: the HMAC-SHA1 is computed over the STUN message
         // up to (but not including) the MESSAGE-INTEGRITY attribute, with the
         // message-length field in the header already set to include the
         // MESSAGE-INTEGRITY attribute (done above). No additional block padding
         // is applied to the input.
         int len = int(ptrMessageIntegrity - buf);
         computeHmac(integrity.hash, buf, len, password.value, password.sizeValue);
         encodeAtrIntegrity(ptrMessageIntegrity, integrity);
      }

      if (verbose) clog << endl;
      encode16(lengthp, uint16_t(ptr - buf - sizeof(StunMsgHdr)));
      return int(ptr - buf);
   }

   int
      stunRand()
   {
      // return 32 bits of random stuff
      resip_assert(sizeof(int) == 4);
      static bool init = false;
      if (!init)
      {
         init = true;

         uint64_t tick;

#if defined(WIN32) 
#if !defined(UNDER_CE) && !defined(__GNUC__) && !defined(_WIN64) && !defined(_M_ARM)
         volatile unsigned int lowtick = 0, hightick = 0;
         __asm
         {
            rdtsc
            mov lowtick, eax
            mov hightick, edx
         }
         tick = hightick;
         tick <<= 32;
         tick |= lowtick;
#else
         tick = GetTickCount();
#endif
#elif defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) || defined(__x86_64__) )
         asm("rdtsc" : "=A" (tick));
#elif defined (__SUNPRO_CC) || (defined(__sun) && defined(__SVR4))
         tick = gethrtime();
#elif defined(__APPLE__) || defined(__MACH__)
         int fd = open("/dev/random", O_RDONLY);
         read(fd, &tick, sizeof(tick));
         closeSocket(fd);
#elif defined(__linux__)
         int fd = open("/dev/urandom", O_RDONLY);
         read(fd, &tick, sizeof(tick));
         closeSocket(fd);
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
      resip_assert(RAND_MAX == 0x7fff);
      int r1 = rand();
      int r2 = rand();

      int ret = (r1 << 16) + r2;

      return ret;
#else
      return random();
#endif
   }


   /// return a random number to use as a port 
   int
      stunRandomPort()
   {
      int min = 0x4000;
      int max = 0x7FFF;

      int ret = stunRand();
      ret = ret | min;
      ret = ret & max;

      return ret;
   }


#ifdef USE_SSL
#ifdef WIN32
   //hack for name collision of OCSP_RESPONSE and wincrypt.h in latest openssl release 0.9.8h
   //http://www.google.com/search?q=OCSP%5fRESPONSE+wincrypt%2eh
   //continue to watch this issue for a real fix.
#undef OCSP_RESPONSE
#endif
#include <openssl/hmac.h>

   void
      computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
   {
      unsigned int resultSize = 0;
      HMAC(EVP_sha1(),
         key, sizeKey,
         reinterpret_cast<const unsigned char*>(input), length,
         reinterpret_cast<unsigned char*>(hmac), &resultSize);
      resip_assert(resultSize == 20);
   }
#else
   void
      computeHmac(char* hmac, const char* input, int length, const char* key, int sizeKey)
   {
      // HMAC-SHA1 (RFC 2104) using the bundled Sha1 implementation, for builds
      // without OpenSSL.
      const int blockSize = 64;  // SHA1 block size in bytes

      unsigned char paddedKey[blockSize];
      memset(paddedKey, 0, sizeof(paddedKey));
      if (sizeKey > blockSize)
      {
         // Keys longer than the block size are first hashed down.
         Sha1 keyHash;
         keyHash.update(std::string(key, sizeKey));
         const Data hashedKey = keyHash.finalBin();
         memcpy(paddedKey, hashedKey.data(),
                hashedKey.size() < static_cast<size_t>(blockSize) ? hashedKey.size() : blockSize);
      }
      else if (sizeKey > 0)
      {
         memcpy(paddedKey, key, sizeKey);
      }

      unsigned char ipad[blockSize];
      unsigned char opad[blockSize];
      for (int i = 0; i < blockSize; ++i)
      {
         ipad[i] = paddedKey[i] ^ 0x36;
         opad[i] = paddedKey[i] ^ 0x5c;
      }

      // inner = SHA1( (key XOR ipad) || message )
      Sha1 inner;
      inner.update(std::string(reinterpret_cast<const char*>(ipad), blockSize));
      inner.update(std::string(input, length));
      const Data innerDigest = inner.finalBin();

      // hmac = SHA1( (key XOR opad) || inner )
      Sha1 outer;
      outer.update(std::string(reinterpret_cast<const char*>(opad), blockSize));
      outer.update(std::string(innerDigest.data(), innerDigest.size()));
      const Data outerDigest = outer.finalBin();

      resip_assert(outerDigest.size() == 20);
      memcpy(hmac, outerDigest.data(), 20);
   }
#endif


   bool
      stunVerifyMessageIntegrity(const char* buf,
         unsigned int bufLen,
         const StunMessage& message,
         const StunAtrString& password)
   {
      if (!message.hasMessageIntegrity || password.sizeValue == 0)
      {
         return false;
      }

      const unsigned int miOffset = message.messageIntegrityOffset;
      // The MESSAGE-INTEGRITY attribute is a 4-byte header followed by a 20-byte
      // HMAC-SHA1 value. Reject anything that does not fit inside the buffer.
      if (miOffset < sizeof(StunMsgHdr) ||
          miOffset + 4 + 20 > bufLen ||
          miOffset > STUN_MAX_MESSAGE_SIZE)
      {
         return false;
      }

      // RFC 5389 section 15.4: the HMAC is computed over the STUN message up to
      // (but not including) the MESSAGE-INTEGRITY attribute, with the message
      // length field in the header set to point at the end of the
      // MESSAGE-INTEGRITY attribute. We copy the prefix so we can adjust the
      // length field without mutating the caller's buffer (and so any trailing
      // attributes, e.g. FINGERPRINT, are excluded).
      char hashInput[STUN_MAX_MESSAGE_SIZE];
      memcpy(hashInput, buf, miOffset);
      const uint16_t adjustedLength = htons(static_cast<uint16_t>(miOffset + 4));
      memcpy(hashInput + 2, &adjustedLength, sizeof(adjustedLength));

      char computed[20] = { 0 };
      computeHmac(computed, hashInput, static_cast<int>(miOffset),
                  password.value, password.sizeValue);

      // Constant-time comparison to avoid leaking HMAC bytes via timing.
      unsigned char diff = 0;
      for (int i = 0; i < 20; ++i)
      {
         diff |= static_cast<unsigned char>(computed[i]) ^
                 static_cast<unsigned char>(message.messageIntegrity.hash[i]);
      }
      return diff == 0;
   }


   static void
      toHex(const char* buffer, int bufferSize, char* output)
   {
      static char hexmap[] = "0123456789abcdef";

      const char* p = buffer;
      char* r = output;
      for (int i = 0; i < bufferSize; i++)
      {
         unsigned char temp = *p++;

         int hi = (temp & 0xf0) >> 4;
         int low = (temp & 0xf);

         *r++ = hexmap[hi];
         *r++ = hexmap[low];
      }
      *r = 0;
   }

   void
      stunCreateUserName(const StunAddress4& source, StunAtrString* username)
   {
      uint64_t time = stunGetSystemTimeSecs();
      time -= (time % 20 * 60);
      //UInt64 hitime = time >> 32;
      uint64_t lotime = time & 0xFFFFFFFF;

      constexpr size_t bufferSize = 256;
      char buffer[bufferSize];
      int expectedSize = std::snprintf(buffer,
         bufferSize,
         "%08x:%08x:%08x:",
         uint32_t(source.addr),
         uint32_t(Random::getCryptoRandom()),  // CSPRNG nonce (CWE-338)
         uint32_t(lotime));
      resip_assert(expectedSize > 0 && static_cast<size_t>(expectedSize) < bufferSize);

      resip_assert(strlen(buffer) + 41 < STUN_MAX_STRING);

      char hmac[20] = { 0 };
      char key[] = "Jason";
      computeHmac(hmac, buffer, (int)strlen(buffer), key, (int)strlen(key));
      char hmacHex[41] = { 0 };
      toHex(hmac, 20, hmacHex);
      hmacHex[40] = 0;

      snprintf(buffer + expectedSize, bufferSize - expectedSize, "%s", hmacHex);

      int l = (int)strlen(buffer);
      resip_assert(l + 1 < STUN_MAX_STRING);
      resip_assert(l % 4 == 0);

      username->sizeValue = l;
      memcpy(username->value, buffer, l);
      username->value[l] = 0;

      //if (verbose) clog << "computed username=" << username.value << endl;
   }

   void
      stunCreatePassword(const StunAtrString& username, StunAtrString* password)
   {
      char hmac[20] = { 0 };
      char key[] = "Fluffy";
      //char buffer[STUN_MAX_STRING];
      computeHmac(hmac, username.value, (int)strlen(username.value), key, (int)strlen(key));
      toHex(hmac, 20, password->value);
      password->sizeValue = 40;
      password->value[40] = 0;

      //clog << "password=" << password->value << endl;
   }


   uint64_t
      stunGetSystemTimeSecs()
   {
      uint64_t time = 0;
#if defined(WIN32)  
      SYSTEMTIME t;
      // CJ TODO - this probably has bug on wrap around every 24 hours
      GetSystemTime(&t);
      time = (t.wHour * 60 + t.wMinute) * 60 + t.wSecond;
#else
      struct timeval now;
      gettimeofday(&now, NULL);
      //assert( now );
      time = now.tv_sec;
#endif
      return time;
   }


   ostream& operator<< (ostream& strm, const UInt128& r)
   {
      strm << int(r.octet[0]);
      for (int i = 1; i < 16; i++)
      {
         strm << ':' << int(r.octet[i]);
      }

      return strm;
   }

   ostream&
      operator<<(ostream& strm, const StunAddress4& addr)
   {
      uint32_t ip = addr.addr;
      strm << ((int)(ip >> 24) & 0xFF) << ".";
      strm << ((int)(ip >> 16) & 0xFF) << ".";
      strm << ((int)(ip >> 8) & 0xFF) << ".";
      strm << ((int)(ip >> 0) & 0xFF);

      strm << ":" << addr.port;

      return strm;
   }

   ostream&
      operator<<(ostream& os, const StunMsgHdr& h)
   {
      os << "STUN: ";
      switch (h.msgType) {
         case BindRequestMsg:
            os << "BindingRequest";
            break;
         case BindResponseMsg:
            os << "BindingResponse";
            break;
         case BindErrorResponseMsg:
            os << "BindingErrorResponse";
            break;
         case TurnAllocateRequest:
            os << "TurnAllocateRequest";
            break;
         case TurnAllocateResponse:
            os << "TurnAllocateResponse";
            break;
         case TurnAllocateErrorResponse:
            os << "TurnAllocateErrorResponse";
            break;
         case TurnSendRequest:
            os << "TurnSendRequest";
            break;
         case TurnSendResponse:
            os << "TurnSendResponse";
            break;
         case TurnSendErrorResponse:
            os << "TurnSendErrorResponse";
            break;
         case TurnDataIndication:
            os << "TurnDataIndication";
            break;
         case TurnSetActiveDestinationRequest:
            os << "TurnSetActiveDestinationRequest";
            break;
         case TurnSetActiveDestinationResponse:
            os << "TurnSetActiveDestinationResponse";
            break;
         case TurnSetActiveDestinationErrorResponse:
            os << "TurnSetActiveDestinationErrorResponse";
            break;
      }

      os << ", id ";

      os << std::hex;
      for (unsigned int i = 0; i < sizeof(UInt128); i++) {
         os << static_cast<int>(h.id.octet[i]);
      }
      os << std::dec;

      return os;
   }


   // returns true if it scucceeded
   bool
      stunParseHostName(char* peerName,
         uint32_t& ip,
         uint16_t& portVal,
         uint16_t defaultPort)
   {
      in_addr sin_addr;

      char host[512];
      snprintf(host, sizeof(host), "%s", peerName);
      host[512 - 1] = '\0';
      char* port = NULL;

      int portNum = defaultPort;

      // pull out the port part if present.
      char* sep = strchr(host, ':');

      if (sep == NULL)
      {
         portNum = defaultPort;
      }
      else
      {
         *sep = '\0';
         port = sep + 1;
         // set port part

         char* endPtr = NULL;

         portNum = strtol(port, &endPtr, 10);

         if (endPtr != NULL)
         {
            if (*endPtr != '\0')
            {
               portNum = defaultPort;
            }
         }
      }

      if (portNum < 1024) return false;
      if (portNum >= 0xFFFF) return false;

      // figure out the host part
#ifdef WIN32
      resip_assert(strlen(host) >= 1);
      if (isdigit(static_cast<unsigned char>(host[0])))
      {
         // assume it is an ip address
         unsigned long a = 0;
         inet_pton(AF_INET, host, &a);
         ip = ntohl(a);
      }
      else
#endif
      {
         // assume it is a host name - getaddrinfo replaces deprecated gethostbyname
         // it is thread-safe and supports both IPv4 and IPv6
         struct addrinfo hints = {};
         struct addrinfo* res = NULL;
         hints.ai_family = AF_INET;        // IPv4 only for now, matching original behaviour
         hints.ai_socktype = SOCK_STREAM;

         int err = getaddrinfo(host, NULL, &hints, &res);
         if (err != 0 || res == NULL)
         {
            std::cerr << "getaddrinfo error: " << gai_strerror(err) << std::endl;
#ifdef WIN32
            resip_assert(getErrno() != WSANOTINITIALISED);
#endif
            ip = ntohl(0x7F000001L);
            return false;
         }

         sin_addr = ((struct sockaddr_in*)res->ai_addr)->sin_addr;
         ip = ntohl(sin_addr.s_addr);
         freeaddrinfo(res);
      }

      portVal = portNum;
      return true;
   }


   bool
      stunParseServerName(char* name, StunAddress4& addr)
   {
      resip_assert(name);

      // TODO - put in DNS SRV stuff.

      bool ret = stunParseHostName(name, addr.addr, addr.port, 3478);
      if (ret != true)
      {
         addr.port = 0xFFFF;
      }
      return ret;
   }

   static void
      stunCreateErrorResponse(StunMessage& response, int cl, int number, const char* msg)
   {
      response.msgHdr.msgType = BindErrorResponseMsg;
      response.hasErrorCode = true;
      response.errorCode.errorClass = cl;
      response.errorCode.number = number;
      snprintf(response.errorCode.reason, sizeof(response.errorCode.reason), "%s", msg);
      response.errorCode.sizeReason = (uint16_t)strlen(msg);
   }

#if 0
   static void
      stunCreateSharedSecretErrorResponse(StunMessage& response, int cl, int number, const char* msg)
   {
      response.msgHdr.msgType = SharedSecretErrorResponseMsg;
      response.hasErrorCode = true;
      response.errorCode.errorClass = cl;
      response.errorCode.number = number;
      strcpy(response.errorCode.reason, msg);
   }
#endif

   static void
      stunCreateSharedSecretResponse(const StunMessage& request, const StunAddress4& source, StunMessage& response)
   {
      response.msgHdr.msgType = SharedSecretResponseMsg;
      response.msgHdr.id = request.msgHdr.id;

      response.hasUsername = true;
      stunCreateUserName(source, &response.username);

      response.hasPassword = true;
      stunCreatePassword(response.username, &response.password);
   }


   // This funtion takes a single message sent to a stun server, parses
   // and constructs an apropriate repsonse - returns true if message is
   // valid
   bool
      stunServerProcessMsg(char* buf,
         unsigned int bufLen,
         StunAddress4& from,
         StunAddress4& secondary,
         StunAddress4& myAddr,
         StunAddress4& altAddr,
         StunMessage* resp,
         StunAddress4* destination,
         StunAtrString* hmacPassword,
         bool* changePort,
         bool* changeIp,
         bool verbose)
   {
      *changeIp = false;
      *changePort = false;

      StunMessage req;
      bool ok = stunParseMessage(buf, bufLen, req, verbose);

      if (!ok)      // Complete garbage, drop it on the floor
      {
         if (verbose) clog << "Request did not parse" << endl;
         return false;
      }
      if (verbose) clog << "Request parsed ok" << endl;

      StunAddress4 mapped = req.mappedAddress.ipv4;
      StunAddress4 respondTo = req.responseAddress.ipv4;
      uint32_t flags = req.changeRequest.value;

      switch (req.msgHdr.msgType)
      {
         case SharedSecretRequestMsg:
            if (verbose) clog << "Received SharedSecretRequestMsg on udp. send error 433." << endl;
            // !cj! - should fix so you know if this came over TLS or UDP
            stunCreateSharedSecretResponse(req, from, *resp);
            //stunCreateSharedSecretErrorResponse(*resp, 4, 33, "this request must be over TLS");
            return true;

         case BindRequestMsg:
            if (!req.hasMessageIntegrity)
            {
               if (verbose) clog << "BindRequest does not contain MessageIntegrity" << endl;

               if (0) // !jf! mustAuthenticate
               {
                  if (verbose) clog << "Received BindRequest with no MessageIntegrity. Sending 401." << endl;
                  stunCreateErrorResponse(*resp, 4, 1, "Missing MessageIntegrity");
                  return true;
               }
            }
            else
            {
               if (!req.hasUsername)
               {
                  if (verbose) clog << "No UserName. Send 432." << endl;
                  stunCreateErrorResponse(*resp, 4, 32, "No UserName and contains MessageIntegrity");
                  return true;
               }
               else
               {
                  if (verbose) clog << "Validating username: " << req.username.value << endl;

                  // Derive the password for this username with the same
                  // shared-secret scheme stunCreateSharedSecretResponse() used
                  // to hand the credential out (stunCreatePassword), then verify
                  // the request's MESSAGE-INTEGRITY against it.
                  //
                  // This replaces a check that (a) compared the wrong bytes
                  // (the message header instead of the received HMAC), (b) read
                  // an uninitialized buffer when built without OpenSSL, and
                  // (c) only ever recognized a single hard-coded "test"/"1234"
                  // credential while silently accepting any other username
                  // without verification.
                  StunAtrString password;
                  stunCreatePassword(req.username, &password);

                  if (!stunVerifyMessageIntegrity(buf, bufLen, req, password))
                  {
                     if (verbose) clog << "MessageIntegrity is bad. Sending 431." << endl;
                     stunCreateErrorResponse(*resp, 4, 31, "Integrity Check Failure");
                     return true;
                  }

                  if (verbose) clog << "MessageIntegrity validated" << endl;

                  // Authenticated: echo USERNAME and request MESSAGE-INTEGRITY on
                  // the response. The response HMAC is computed by
                  // stunEncodeMessage() using hmacPassword, which is set below.
                  resip_assert(req.hasUsername);
                  resp->hasMessageIntegrity = true;
                  resp->hasUsername = true;
                  resp->username = req.username; // copy username in
               }
            }

            // TODO !jf! should check for unknown attributes here and send 420 listing the
            // unknown attributes. 

            if (respondTo.port == 0) respondTo = from;
            if (mapped.port == 0) mapped = from;

            *changeIp = (flags & ChangeIpFlag) ? true : false;
            *changePort = (flags & ChangePortFlag) ? true : false;

            if (verbose)
            {
               clog << "Request is valid:" << endl;
               clog << "\t flags=" << flags << endl;
               clog << "\t changeIp=" << *changeIp << endl;
               clog << "\t changePort=" << *changePort << endl;
               clog << "\t from = " << from << endl;
               clog << "\t respond to = " << respondTo << endl;
               clog << "\t mapped = " << mapped << endl;
            }

            // form the outgoing message
            resp->msgHdr.msgType = BindResponseMsg;
            for (int i = 0; i < 16; i++)
            {
               resp->msgHdr.id.octet[i] = req.msgHdr.id.octet[i];
            }

            if (req.xorOnly == false)
            {
               resp->hasMappedAddress = true;
               resp->mappedAddress.ipv4.port = mapped.port;
               resp->mappedAddress.ipv4.addr = mapped.addr;
            }

            if (1) // do xorMapped address or not 
            {
               resp->hasXorMappedAddress = true;
               uint16_t id16 = req.msgHdr.id.octet[0] << 8
                  | req.msgHdr.id.octet[1];
               uint32_t id32 = req.msgHdr.id.octet[0] << 24
                  | req.msgHdr.id.octet[1] << 16
                  | req.msgHdr.id.octet[2] << 8
                  | req.msgHdr.id.octet[3];
               resp->xorMappedAddress.ipv4.port = mapped.port ^ id16;
               resp->xorMappedAddress.ipv4.addr = mapped.addr ^ id32;
            }

            resp->hasSourceAddress = true;
            resp->sourceAddress.ipv4.port = (*changePort) ? altAddr.port : myAddr.port;
            resp->sourceAddress.ipv4.addr = (*changeIp) ? altAddr.addr : myAddr.addr;

            resp->hasChangedAddress = true;
            resp->changedAddress.ipv4.port = altAddr.port;
            resp->changedAddress.ipv4.addr = altAddr.addr;

            if (secondary.port != 0)
            {
               resp->hasSecondaryAddress = true;
               resp->secondaryAddress.ipv4.port = secondary.port;
               resp->secondaryAddress.ipv4.addr = secondary.addr;
            }

            if (req.hasUsername && req.username.sizeValue > 0)
            {
               // copy username in
               resp->hasUsername = true;
               resip_assert(req.username.sizeValue % 4 == 0);
               resip_assert(req.username.sizeValue < STUN_MAX_STRING);
               memcpy(resp->username.value, req.username.value, req.username.sizeValue);
               resp->username.sizeValue = req.username.sizeValue;
            }

            if (1) // add Software 
            {
               resp->hasSoftware = true;
               const char software[] = "Vovida.org " STUN_VERSION; // must pad to mult of 4

               resip_assert(sizeof(software) < STUN_MAX_STRING);
               //cerr << "sizeof serverName is "  << sizeof(serverName) << endl;
               resip_assert(sizeof(software) % 4 == 0);
               memcpy(resp->software.value, software, sizeof(software));
               resp->software.sizeValue = sizeof(software);
            }

            if (req.hasMessageIntegrity & req.hasUsername)
            {
               // this creates the password that will be used in the HMAC when then
               // messages is sent
               stunCreatePassword(req.username, hmacPassword);
            }

            if (req.hasUsername && (req.username.sizeValue > 64))
            {
               uint32_t source;
               resip_assert(sizeof(int) == sizeof(uint32_t));

               source = strtoul(req.username.value, nullptr, 16);
               resp->hasReflectedFrom = true;
               resp->reflectedFrom.ipv4.port = 0;
               resp->reflectedFrom.ipv4.addr = source;
            }

            destination->port = respondTo.port;
            destination->addr = respondTo.addr;

            return true;

         default:
            if (verbose) clog << "Unknown or unsupported request " << endl;
            return false;
      }

      resip_assert(0);
      return false;
   }

   bool
      stunInitServer(StunServerInfo& info, const StunAddress4& myAddr,
         const StunAddress4& altAddr, int startMediaPort, bool verbose)
   {
      resip_assert(myAddr.port != 0);
      resip_assert(altAddr.port != 0);
      resip_assert(myAddr.addr != 0);
      //assert( altAddr.addr != 0 );

      info.myAddr = myAddr;
      info.altAddr = altAddr;

      info.myFd = INVALID_SOCKET;
      info.altPortFd = INVALID_SOCKET;
      info.altIpFd = INVALID_SOCKET;
      info.altIpPortFd = INVALID_SOCKET;

      memset(info.relays, 0, sizeof(info.relays));
      if (startMediaPort > 0)
      {
         info.relay = true;

         for (int i = 0; i < MAX_MEDIA_RELAYS; ++i)
         {
            StunMediaRelay* relay = &info.relays[i];
            relay->relayPort = startMediaPort + i;
            relay->fd = 0;
            relay->expireTime = 0;
         }
      }
      else
      {
         info.relay = false;
      }

      if ((info.myFd = openPort(myAddr.port, myAddr.addr, verbose)) == INVALID_SOCKET)
      {
         if (verbose) clog << "Can't open " << myAddr << endl;
         stunStopServer(info);

         return false;
      }
      //if (verbose) clog << "Opened " << myAddr.addr << ":" << myAddr.port << " --> " << info.myFd << endl;

      if ((info.altPortFd = openPort(altAddr.port, myAddr.addr, verbose)) == INVALID_SOCKET)
      {
         if (verbose) clog << "Can't open " << myAddr << endl;
         stunStopServer(info);
         return false;
      }
      //if (verbose) clog << "Opened " << myAddr.addr << ":" << altAddr.port << " --> " << info.altPortFd << endl;


      info.altIpFd = INVALID_SOCKET;
      if (altAddr.addr != 0)
      {
         if ((info.altIpFd = openPort(myAddr.port, altAddr.addr, verbose)) == INVALID_SOCKET)
         {
            if (verbose) clog << "Can't open " << altAddr << endl;
            stunStopServer(info);
            return false;
         }
         //if (verbose) clog << "Opened " << altAddr.addr << ":" << myAddr.port << " --> " << info.altIpFd << endl;;
      }

      info.altIpPortFd = INVALID_SOCKET;
      if (altAddr.addr != 0)
      {
         if ((info.altIpPortFd = openPort(altAddr.port, altAddr.addr, verbose)) == INVALID_SOCKET)
         {
            if (verbose) clog << "Can't open " << altAddr << endl;
            stunStopServer(info);
            return false;
         }
         //if (verbose) clog << "Opened " << altAddr.addr << ":" << altAddr.port << " --> " << info.altIpPortFd << endl;;
      }

      return true;
   }

   void
      stunStopServer(StunServerInfo& info)
   {
      if (info.myFd > 0) closeSocket(info.myFd);
      if (info.altPortFd > 0) closeSocket(info.altPortFd);
      if (info.altIpFd > 0) closeSocket(info.altIpFd);
      if (info.altIpPortFd > 0) closeSocket(info.altIpPortFd);

      if (info.relay)
      {
         for (int i = 0; i < MAX_MEDIA_RELAYS; ++i)
         {
            StunMediaRelay* relay = &info.relays[i];
            if (relay->fd)
            {
               closeSocket(relay->fd);
               relay->fd = 0;
            }
         }
      }
   }

   bool
      stunServerProcess(StunServerInfo& info, bool verbose)
   {
      char msg[STUN_MAX_MESSAGE_SIZE];
      int msgLen = sizeof(msg);

      bool ok = false;
      bool recvAltIp = false;
      bool recvAltPort = false;

      fd_set fdSet;
      resip::Socket maxFd = 0;

      FD_ZERO(&fdSet);
      FD_SET(info.myFd, &fdSet);
      if (info.myFd >= maxFd) maxFd = info.myFd + 1;
      FD_SET(info.altPortFd, &fdSet);
      if (info.altPortFd >= maxFd) maxFd = info.altPortFd + 1;

      if (info.altIpFd != INVALID_SOCKET)
      {
         FD_SET(info.altIpFd, &fdSet);
         if (info.altIpFd >= maxFd) maxFd = info.altIpFd + 1;
      }
      if (info.altIpPortFd != INVALID_SOCKET)
      {
         FD_SET(info.altIpPortFd, &fdSet);
         if (info.altIpPortFd >= maxFd) maxFd = info.altIpPortFd + 1;
      }

      if (info.relay)
      {
         for (int i = 0; i < MAX_MEDIA_RELAYS; ++i)
         {
            StunMediaRelay* relay = &info.relays[i];
            if (relay->fd)
            {
               FD_SET(relay->fd, &fdSet);
               if ((resip::Socket)relay->fd >= maxFd)
               {
                  maxFd = relay->fd + 1;
               }
            }
         }
      }

      if (info.altIpFd != INVALID_SOCKET)
      {
         FD_SET(info.altIpFd, &fdSet);
         if (info.altIpFd >= maxFd) maxFd = info.altIpFd + 1;
      }
      if (info.altIpPortFd != INVALID_SOCKET)
      {
         FD_SET(info.altIpPortFd, &fdSet);
         if (info.altIpPortFd >= maxFd) maxFd = info.altIpPortFd + 1;
      }

      struct timeval tv;
      tv.tv_sec = 0;
      tv.tv_usec = 1000;

      int e = (int)select((int)maxFd, &fdSet, NULL, NULL, &tv);
      if (e < 0)
      {
         int err = getErrno();
         if (verbose) clog << "Error on select: " << strError(err) << endl;
      }
      else if (e >= 0)
      {
         StunAddress4 from;

         // do the media relaying
         if (info.relay)
         {
            time_t now = time(0);
            for (int i = 0; i < MAX_MEDIA_RELAYS; ++i)
            {
               StunMediaRelay* relay = &info.relays[i];
               if (relay->fd)
               {
                  if (FD_ISSET(relay->fd, &fdSet))
                  {
                     char msg[MAX_RTP_MSG_SIZE];
                     int msgLen = sizeof(msg);

                     StunAddress4 rtpFrom;
                     ok = getMessage(relay->fd, msg, &msgLen, &rtpFrom.addr, &rtpFrom.port, verbose);
                     if (ok)
                     {
                        sendMessage(info.myFd, msg, msgLen, relay->destination.addr, relay->destination.port, verbose);
                        relay->expireTime = now + MEDIA_RELAY_TIMEOUT;
                        if (verbose) clog << "Relay packet on "
                           << relay->fd
                           << " from " << rtpFrom
                           << " -> " << relay->destination
                           << endl;
                     }
                  }
                  else if (now > relay->expireTime)
                  {
                     closeSocket(relay->fd);
                     relay->fd = 0;
                  }
               }
            }
         }


         if (FD_ISSET(info.myFd, &fdSet))
         {
            if (verbose) clog << "received on A1:P1" << endl;
            recvAltIp = false;
            recvAltPort = false;
            ok = getMessage(info.myFd, msg, &msgLen, &from.addr, &from.port, verbose);
         }
         else if (FD_ISSET(info.altPortFd, &fdSet))
         {
            if (verbose) clog << "received on A1:P2" << endl;
            recvAltIp = false;
            recvAltPort = true;
            ok = getMessage(info.altPortFd, msg, &msgLen, &from.addr, &from.port, verbose);
         }
         else if ((info.altIpFd != INVALID_SOCKET) && FD_ISSET(info.altIpFd, &fdSet))
         {
            if (verbose) clog << "received on A2:P1" << endl;
            recvAltIp = true;
            recvAltPort = false;
            ok = getMessage(info.altIpFd, msg, &msgLen, &from.addr, &from.port, verbose);
         }
         else if ((info.altIpPortFd != INVALID_SOCKET) && FD_ISSET(info.altIpPortFd, &fdSet))
         {
            if (verbose) clog << "received on A2:P2" << endl;
            recvAltIp = true;
            recvAltPort = true;
            ok = getMessage(info.altIpPortFd, msg, &msgLen, &from.addr, &from.port, verbose);
         }
         else
         {
            return true;
         }

         int relayPort = 0;
         if (info.relay)
         {
            for (int i = 0; i < MAX_MEDIA_RELAYS; ++i)
            {
               StunMediaRelay* relay = &info.relays[i];
               if (relay->destination.addr == from.addr &&
                  relay->destination.port == from.port)
               {
                  relayPort = relay->relayPort;
                  relay->expireTime = time(0) + MEDIA_RELAY_TIMEOUT;
                  break;
               }
            }

            if (relayPort == 0)
            {
               for (int i = 0; i < MAX_MEDIA_RELAYS; ++i)
               {
                  StunMediaRelay* relay = &info.relays[i];
                  if (relay->fd == 0)
                  {
                     if (verbose) clog << "Open relay port " << relay->relayPort << endl;

                     relay->fd = (int)openPort(relay->relayPort, info.myAddr.addr, verbose);
                     relay->destination.addr = from.addr;
                     relay->destination.port = from.port;
                     relay->expireTime = time(0) + MEDIA_RELAY_TIMEOUT;
                     relayPort = relay->relayPort;
                     break;
                  }
               }
            }
         }

         if (!ok)
         {
            if (verbose) clog << "Get message did not return a valid message" << endl;
            return true;
         }

         if (verbose) clog << "Got a request (len=" << msgLen << ") from " << from << endl;

         if (msgLen <= 0)
         {
            return true;
         }

         bool changePort = false;
         bool changeIp = false;

         StunMessage resp;
         StunAddress4 dest;
         StunAtrString hmacPassword;
         hmacPassword.sizeValue = 0;

         StunAddress4 secondary;
         secondary.port = 0;
         secondary.addr = 0;

         if (info.relay && relayPort)
         {
            secondary = from;

            from.addr = info.myAddr.addr;
            from.port = relayPort;
         }

         ok = stunServerProcessMsg(msg, msgLen, from, secondary,
            recvAltIp ? info.altAddr : info.myAddr,
            recvAltIp ? info.myAddr : info.altAddr,
            &resp,
            &dest,
            &hmacPassword,
            &changePort,
            &changeIp,
            verbose);

         if (!ok)
         {
            if (verbose) clog << "Failed to parse message" << endl;
            return true;
         }

         char buf[STUN_MAX_MESSAGE_SIZE];
         int len = sizeof(buf);

         len = stunEncodeMessage(resp, buf, len, hmacPassword, verbose);

         if (dest.addr == 0)  ok = false;
         if (dest.port == 0) ok = false;

         if (ok)
         {
            resip_assert(dest.addr != 0);
            resip_assert(dest.port != 0);

            resip::Socket sendFd;

            bool sendAltIp = recvAltIp;   // send on the received IP address 
            bool sendAltPort = recvAltPort; // send on the received port

            if (changeIp)   sendAltIp = !sendAltIp;   // if need to change IP, then flip logic 
            if (changePort) sendAltPort = !sendAltPort; // if need to change port, then flip logic 

            if (!sendAltPort)
            {
               if (!sendAltIp)
               {
                  sendFd = info.myFd;
               }
               else
               {
                  sendFd = info.altIpFd;
               }
            }
            else
            {
               if (!sendAltIp)
               {
                  sendFd = info.altPortFd;
               }
               else
               {
                  sendFd = info.altIpPortFd;
               }
            }

            if (sendFd != INVALID_SOCKET)
            {
               sendMessage(sendFd, buf, len, dest.addr, dest.port, verbose);
            }
         }
      }

      return true;
   }

   int
      stunFindLocalInterfaces(uint32_t* addresses, int maxRet)
   {
#if defined(WIN32) || defined(__sparc__)
      return 0;
#else
      struct ifconf ifc;

      int s = socket(AF_INET, SOCK_DGRAM, 0);
      const int len = 100 * sizeof(struct ifreq);

      char buf[len];

      ifc.ifc_len = len;
      ifc.ifc_buf = buf;

      int e = ioctl(s, SIOCGIFCONF, &ifc);
      char* ptr = buf;
      int tl = ifc.ifc_len;
      int count = 0;

      while ((tl > 0) && (count < maxRet))
      {
         struct ifreq* ifr = (struct ifreq*)ptr;

         int si = sizeof(ifr->ifr_name) + sizeof(struct sockaddr);
         tl -= si;
         ptr += si;
         //char* name = ifr->ifr_ifrn.ifrn_name;
         //cerr << "name = " << name << endl;

         struct ifreq ifr2;
         ifr2 = *ifr;

         e = ioctl(s, SIOCGIFADDR, &ifr2);
         if (e == -1)
         {
            break;
         }

         //cerr << "ioctl addr e = " << e << endl;

         struct sockaddr a = ifr2.ifr_addr;
         struct sockaddr_in* addr = (struct sockaddr_in*)&a;

         uint32_t ai = ntohl(addr->sin_addr.s_addr);
         if (int((ai >> 24) & 0xFF) != 127)
         {
            addresses[count++] = ai;
         }

#if 0
         cerr << "Detected interface "
            << int((ai >> 24) & 0xFF) << "."
            << int((ai >> 16) & 0xFF) << "."
            << int((ai >> 8) & 0xFF) << "."
            << int((ai) & 0xFF) << endl;
#endif
      }

      closeSocket(s);

      return count;
#endif
   }


   void
      stunBuildReqSimple(StunMessage* msg,
         const StunAtrString& username,
         bool changePort, bool changeIp, unsigned int id)
   {
      resip_assert(msg);

      msg->msgHdr.msgType = BindRequestMsg;

      for (int i = 0; i < 16; i = i + 4)
      {
         resip_assert(i + 3 < 16);
         // STUN transaction IDs must be unpredictable (CWE-338/CWE-345): a
         // predictable ID lets an attacker forge STUN responses. Use the CSPRNG
         // rather than the weak stunRand() generator.
         int r = Random::getCryptoRandom();
         msg->msgHdr.id.octet[i + 0] = r >> 0;
         msg->msgHdr.id.octet[i + 1] = r >> 8;
         msg->msgHdr.id.octet[i + 2] = r >> 16;
         msg->msgHdr.id.octet[i + 3] = r >> 24;
      }

      if (id != 0)
      {
         msg->msgHdr.id.octet[0] = id;
      }

      msg->hasChangeRequest = true;
      msg->changeRequest.value = (changeIp ? ChangeIpFlag : 0) |
         (changePort ? ChangePortFlag : 0);

      if (username.sizeValue > 0)
      {
         msg->hasUsername = true;
         msg->username = username;
      }
   }


   static void
      stunSendTest(resip::Socket myFd, StunAddress4& dest,
         const StunAtrString& username, const StunAtrString& password,
         int testNum, bool verbose)
   {
      resip_assert(dest.addr != 0);
      resip_assert(dest.port != 0);

      bool changePort = false;
      bool changeIP = false;

      switch (testNum)
      {
         case 1:
         case 5:
         case 10:
         case 11:
            break;
         case 2:
            //changePort=true;
            changeIP = true;
            break;
         case 3:
            changePort = true;
            break;
         case 4:
            changeIP = true;
            break;
         default:
            cerr << "Test " << testNum << " is unknown\n";
            resip_assert(0);
      }

      StunMessage req;
      stunBuildReqSimple(&req, username,
         changePort, changeIP,
         testNum);

      char buf[STUN_MAX_MESSAGE_SIZE];
      int len = STUN_MAX_MESSAGE_SIZE;

      len = stunEncodeMessage(req, buf, len, password, verbose);

      if (verbose)
      {
         clog << "About to send msg of len " << len << " to " << dest << endl;
      }

      sendMessage(myFd, buf, len, dest.addr, dest.port, verbose);

   }


   void
      stunGetUserNameAndPassword(const StunAddress4& dest,
         StunAtrString* username,
         StunAtrString* password)
   {
      // !cj! This is totally bogus - need to make TLS connection to dest and get a
      // username and password to use 
      stunCreateUserName(dest, username);
      stunCreatePassword(*username, password);
   }


   bool
      stunTest(StunAddress4& dest, int testNum, bool verbose, StunAddress4* sAddr, unsigned long timeoutMs)
   {
      resip_assert(dest.addr != 0);
      resip_assert(dest.port != 0);

      int port = stunRandomPort();
      uint32_t interfaceIp = 0;
      if (sAddr)
      {
         interfaceIp = sAddr->addr;
         if (sAddr->port != 0)
         {
            port = sAddr->port;
         }
      }
      resip::Socket myFd = openPort(port, interfaceIp, verbose);

      if (myFd == INVALID_SOCKET)
      {
         return false;
      }

      // make socket non-blocking
      if (!makeSocketNonBlocking(myFd))
      {
         closeSocket(myFd);
         return false;
      }

      StunAtrString username;
      StunAtrString password;

      username.sizeValue = 0;
      password.sizeValue = 0;

#ifdef USE_TLS
      stunGetUserNameAndPassword(dest, username, password);
#endif

      stunSendTest(myFd, dest, username, password, testNum, verbose);

      char msg[STUN_MAX_MESSAGE_SIZE];
      int msgLen = STUN_MAX_MESSAGE_SIZE;

      // Wait to receive a packet
      resip::FdSet myFdSet;
      myFdSet.setRead(myFd);
      if (myFdSet.selectMilliSeconds(timeoutMs) < 1)
      {
         // no packet received or an error occured
         closeSocket(myFd);
         return false;
      }

      StunAddress4 from;
      if (!getMessage(myFd, msg, &msgLen, &from.addr, &from.port, verbose))
      {
         closeSocket(myFd);
         return false;
      }

      if (verbose) clog << "Got a response" << endl;

      StunMessage resp;
      bool ok = stunParseMessage(msg, msgLen, resp, verbose);

      if (verbose)
      {
         clog << "\t ok=" << ok << endl;
         clog << "\t id=" << resp.msgHdr.id << endl;
         clog << "\t mappedAddr=" << resp.mappedAddress.ipv4 << endl;
         clog << "\t changedAddr=" << resp.changedAddress.ipv4 << endl;
         clog << endl;
      }

      if (sAddr)
      {
         sAddr->port = resp.mappedAddress.ipv4.port;
         sAddr->addr = resp.mappedAddress.ipv4.addr;
      }

      closeSocket(myFd);
      return ok;
   }


   NatType
      stunNatType(StunAddress4& dest,
         bool verbose,
         bool* preservePort, // if set, is return for if NAT preservers ports or not
         bool* hairpin,  // if set, is the return for if NAT will hairpin packets
         int port, // port to use for the test, 0 to choose random port
         StunAddress4* sAddr // NIC to use 
      )
   {
      resip_assert(dest.addr != 0);
      resip_assert(dest.port != 0);

      if (hairpin)
      {
         *hairpin = false;
      }

      if (port == 0)
      {
         port = stunRandomPort();
      }
      uint32_t interfaceIp = 0;
      if (sAddr)
      {
         interfaceIp = sAddr->addr;
      }
      resip::Socket myFd1 = openPort(port, interfaceIp, verbose);
      resip::Socket myFd2 = openPort(port + 1, interfaceIp, verbose);

      if ((myFd1 == INVALID_SOCKET) || (myFd2 == INVALID_SOCKET))
      {
         if (myFd1 != INVALID_SOCKET) closeSocket(myFd1);
         if (myFd2 != INVALID_SOCKET) closeSocket(myFd2);
         cerr << "Some problem opening port/interface to send on" << endl;
         return StunTypeFailure;
      }

      resip_assert(myFd1 != INVALID_SOCKET);
      resip_assert(myFd2 != INVALID_SOCKET);

      bool respTestI = false;
      bool isNat = true;
      StunAddress4 testImappedAddr;
      bool respTestI2 = false;
      bool mappedIpSame = true;
      StunAddress4 testI2mappedAddr;
      StunAddress4 testI2dest = dest;
      bool respTestII = false;
      bool respTestIII = false;

      bool respTestHairpin = false;
      bool respTestPreservePort = false;

      memset(&testImappedAddr, 0, sizeof(testImappedAddr));

      StunAtrString username;
      StunAtrString password;

      username.sizeValue = 0;
      password.sizeValue = 0;

#ifdef USE_TLS 
      stunGetUserNameAndPassword(dest, username, password);
#endif

      int count = 0;
      while (count < 7)
      {
         struct timeval tv;
         fd_set fdSet;
#ifdef WIN32
         typedef unsigned int FdSetSize;
#else
         typedef int FdSetSize;
#endif
         FdSetSize fdSetSize;
         FD_ZERO(&fdSet); fdSetSize = 0;
         FD_SET(myFd1, &fdSet); fdSetSize = ((FdSetSize)myFd1 + 1 > fdSetSize) ? (FdSetSize)myFd1 + 1 : fdSetSize;
         FD_SET(myFd2, &fdSet); fdSetSize = ((FdSetSize)myFd2 + 1 > fdSetSize) ? (FdSetSize)myFd2 + 1 : fdSetSize;
         tv.tv_sec = 0;
         tv.tv_usec = 150 * 1000; // 150 ms 
         if (count == 0) tv.tv_usec = 0;

         int  err = select(fdSetSize, &fdSet, NULL, NULL, &tv);
         int e = getErrno();
         if (err == SOCKET_ERROR)
         {
            // error occured
            closeSocket(myFd1);
            closeSocket(myFd2);
            cerr << "Error " << e << " " << strError(e) << " in select" << endl;
            return StunTypeFailure;
         }
         else if (err == 0)
         {
            // timeout occured 
            count++;

            if (!respTestI)
            {
               stunSendTest(myFd1, dest, username, password, 1, verbose);
            }

            if ((!respTestI2) && respTestI)
            {
               // check the address to send to if valid 
               if ((testI2dest.addr != 0) &&
                  (testI2dest.port != 0))
               {
                  stunSendTest(myFd1, testI2dest, username, password, 10, verbose);
               }
            }

            if (!respTestII)
            {
               stunSendTest(myFd2, dest, username, password, 2, verbose);
            }

            if (!respTestIII)
            {
               stunSendTest(myFd2, dest, username, password, 3, verbose);
            }

            if (respTestI && (!respTestHairpin))
            {
               if ((testImappedAddr.addr != 0) &&
                  (testImappedAddr.port != 0))
               {
                  stunSendTest(myFd1, testImappedAddr, username, password, 11, verbose);
               }
            }
         }
         else
         {
            //if (verbose) clog << "-----------------------------------------" << endl;
            resip_assert(err > 0);
            // data is avialbe on some fd 

            for (int i = 0; i < 2; i++)
            {
               resip::Socket myFd;
               if (i == 0)
               {
                  myFd = myFd1;
               }
               else
               {
                  myFd = myFd2;
               }

               if (myFd != INVALID_SOCKET)
               {
                  if (FD_ISSET(myFd, &fdSet))
                  {
                     char msg[STUN_MAX_MESSAGE_SIZE];
                     int msgLen = sizeof(msg);

                     StunAddress4 from;

                     getMessage(myFd,
                        msg,
                        &msgLen,
                        &from.addr,
                        &from.port,
                        verbose);

                     StunMessage resp;
                     stunParseMessage(msg, msgLen, resp, verbose);

                     if (verbose)
                     {
                        clog << "Received message of type " << resp.msgHdr.msgType
                           << "  id=" << (int)(resp.msgHdr.id.octet[0]) << endl;
                     }

                     switch (resp.msgHdr.id.octet[0])
                     {
                        case 1:
                        {
                           if (!respTestI)
                           {

                              testImappedAddr.addr = resp.mappedAddress.ipv4.addr;
                              testImappedAddr.port = resp.mappedAddress.ipv4.port;

                              respTestPreservePort = (testImappedAddr.port == port);
                              if (preservePort)
                              {
                                 *preservePort = respTestPreservePort;
                              }

                              testI2dest.addr = resp.changedAddress.ipv4.addr;

                              if (sAddr)
                              {
                                 sAddr->port = testImappedAddr.port;
                                 sAddr->addr = testImappedAddr.addr;
                              }

                              count = 0;
                           }
                           respTestI = true;
                        }
                        break;
                        case 2:
                        {
                           respTestII = true;
                        }
                        break;
                        case 3:
                        {
                           respTestIII = true;
                        }
                        break;
                        case 10:
                        {
                           if (!respTestI2)
                           {
                              testI2mappedAddr.addr = resp.mappedAddress.ipv4.addr;
                              testI2mappedAddr.port = resp.mappedAddress.ipv4.port;

                              mappedIpSame = false;
                              if ((testI2mappedAddr.addr == testImappedAddr.addr) &&
                                 (testI2mappedAddr.port == testImappedAddr.port))
                              {
                                 mappedIpSame = true;
                              }


                           }
                           respTestI2 = true;
                        }
                        break;
                        case 11:
                        {

                           if (hairpin)
                           {
                              *hairpin = true;
                           }
                           respTestHairpin = true;
                        }
                        break;
                     }
                  }
               }
            }
         }
      }

      closeSocket(myFd1);
      closeSocket(myFd2);

      // see if we can bind to this address 
      //cerr << "try binding to " << testImappedAddr << endl;
      resip::Socket s = openPort(0/*use ephemeral*/, testImappedAddr.addr, false);
      if (s != INVALID_SOCKET)
      {
         closeSocket(s);
         isNat = false;
         //cerr << "binding worked" << endl;
      }
      else
      {
         isNat = true;
         //cerr << "binding failed" << endl;
      }

      if (verbose)
      {
         clog << "test I = " << respTestI << endl;
         clog << "test II = " << respTestII << endl;
         clog << "test III = " << respTestIII << endl;
         clog << "test I(2) = " << respTestI2 << endl;
         clog << "is nat  = " << isNat << endl;
         clog << "mapped IP same = " << mappedIpSame << endl;
         clog << "hairpin = " << respTestHairpin << endl;
         clog << "preserver port = " << respTestPreservePort << endl;
      }

#if 0
      // implement logic flow chart from draft RFC
      if (respTestI)
      {
         if (isNat)
         {
            if (respTestII)
            {
               return StunTypeConeNat;
            }
            else
            {
               if (mappedIpSame)
               {
                  if (respTestIII)
                  {
                     return StunTypeRestrictedNat;
                  }
                  else
                  {
                     return StunTypePortRestrictedNat;
                  }
               }
               else
               {
                  return StunTypeSymNat;
               }
            }
         }
         else
         {
            if (respTestII)
            {
               return StunTypeOpen;
            }
            else
            {
               return StunTypeSymFirewall;
            }
         }
      }
      else
      {
         return StunTypeBlocked;
      }
#else
      if (respTestI) // not blocked 
      {
         if (isNat)
         {
            if (mappedIpSame)
            {
               if (respTestII)
               {
                  return StunTypeIndependentFilter;
               }
               else
               {
                  if (respTestIII)
                  {
                     return StunTypeDependentFilter;
                  }
                  else
                  {
                     return StunTypePortDependedFilter;
                  }
               }
            }
            else // mappedIp is not same 
            {
               return StunTypeDependentMapping;
            }
         }
         else  // isNat is false
         {
            if (respTestII)
            {
               return StunTypeOpen;
            }
            else
            {
               return StunTypeFirewall;
            }
         }
      }
      else
      {
         return StunTypeBlocked;
      }
#endif

      return StunTypeUnknown;
   }


   int
      stunOpenSocket(StunAddress4& dest, StunAddress4* mapAddr,
         int port, StunAddress4* srcAddr,
         bool verbose)
   {
      resip_assert(dest.addr != 0);
      resip_assert(dest.port != 0);
      resip_assert(mapAddr);

      if (port == 0)
      {
         port = stunRandomPort();
      }
      unsigned int interfaceIp = 0;
      if (srcAddr)
      {
         interfaceIp = srcAddr->addr;
      }

      resip::Socket myFd = openPort(port, interfaceIp, verbose);
      if (myFd == INVALID_SOCKET)
      {
         return (int)myFd;
      }

      char msg[STUN_MAX_MESSAGE_SIZE];
      int msgLen = sizeof(msg);

      StunAtrString username;
      StunAtrString password;

      username.sizeValue = 0;
      password.sizeValue = 0;

#ifdef USE_TLS
      stunGetUserNameAndPassword(dest, username, password);
#endif

      stunSendTest(myFd, dest, username, password, 1, 0/*false*/);

      StunAddress4 from;

      getMessage(myFd, msg, &msgLen, &from.addr, &from.port, verbose);

      StunMessage resp;
      bool ok = stunParseMessage(msg, msgLen, resp, verbose);
      if (!ok)
      {
         closeSocket(myFd);
         return -1;
      }

      StunAddress4 mappedAddr = resp.mappedAddress.ipv4;

      //clog << "--- stunOpenSocket --- " << endl;
      //clog << "\treq  id=" << req.id << endl;
      //clog << "\tresp id=" << id << endl;
      //clog << "\tmappedAddr=" << mappedAddr << endl;

      *mapAddr = mappedAddr;

      return (int)myFd;
   }


   bool
      stunOpenSocketPair(StunAddress4& dest, StunAddress4* mapAddr,
         int* fd1, int* fd2,
         int port, StunAddress4* srcAddr,
         bool verbose)
   {
      resip_assert(dest.addr != 0);
      resip_assert(dest.port != 0);
      resip_assert(mapAddr);

      const int NUM = 3;

      if (port == 0)
      {
         port = stunRandomPort();
      }

      *fd1 = -1;
      *fd2 = -1;

      char msg[STUN_MAX_MESSAGE_SIZE];
      int msgLen = sizeof(msg);

      StunAddress4 from;
      int fd[NUM];
      int i;

      unsigned int interfaceIp = 0;
      if (srcAddr)
      {
         interfaceIp = srcAddr->addr;
      }

      for (i = 0; i < NUM; i++)
      {
         fd[i] = (int)openPort((port == 0) ? 0 : (port + i),
            interfaceIp, verbose);
         if (fd[i] < 0)
         {
            while (i > 0)
            {
               closeSocket(fd[--i]);
            }
            return false;
         }
      }

      StunAtrString username;
      StunAtrString password;

      username.sizeValue = 0;
      password.sizeValue = 0;

#ifdef USE_TLS
      stunGetUserNameAndPassword(dest, username, password);
#endif

      for (i = 0; i < NUM; i++)
      {
         stunSendTest(fd[i], dest, username, password, 1/*testNum*/, verbose);
      }

      StunAddress4 mappedAddr[NUM];
      for (i = 0; i < NUM; i++)
      {
         msgLen = sizeof(msg) / sizeof(*msg);
         getMessage(fd[i],
            msg,
            &msgLen,
            &from.addr,
            &from.port, verbose);

         StunMessage resp;
         bool ok = stunParseMessage(msg, msgLen, resp, verbose);
         if (!ok)
         {
            return false;
         }

         mappedAddr[i] = resp.mappedAddress.ipv4;
      }

      if (verbose)
      {
         clog << "--- stunOpenSocketPair --- " << endl;
         for (i = 0; i < NUM; i++)
         {
            clog << "\t mappedAddr=" << mappedAddr[i] << endl;
         }
      }

      if (mappedAddr[0].port % 2 == 0)
      {
         if (mappedAddr[0].port + 1 == mappedAddr[1].port)
         {
            *mapAddr = mappedAddr[0];
            *fd1 = fd[0];
            *fd2 = fd[1];
            closeSocket(fd[2]);
            return true;
         }
      }
      else
      {
         if ((mappedAddr[1].port % 2 == 0)
            && (mappedAddr[1].port + 1 == mappedAddr[2].port))
         {
            *mapAddr = mappedAddr[1];
            *fd1 = fd[1];
            *fd2 = fd[2];
            closeSocket(fd[0]);
            return true;
         }
      }

      // something failed, close all and return error
      for (i = 0; i < NUM; i++)
      {
         closeSocket(fd[i]);
      }

      return false;
   }

   bool
      operator<(const UInt128& lhs, const UInt128& rhs)
   {
      return memcmp(&lhs, &rhs, sizeof(lhs)) < 0;
   }

   bool operator==(const UInt128& lhs, const UInt128& rhs)
   {
      return memcmp(&lhs, &rhs, sizeof(lhs)) == 0;
   }

   bool
      operator<(const StunMsgHdr& lhs, const StunMsgHdr& rhs)
   {
      return lhs.id < rhs.id;
   }

}

/* ====================================================================
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 * The Vovida Software License, Version 1.0
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
 */

 // Local Variables:
 // mode:c++
 // c-file-style:"ellemtel"
 // c-file-offsets:((case-label . +))
 // indent-tabs-mode:nil
 // End:


