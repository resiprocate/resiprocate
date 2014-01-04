
#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#include "rutil/Socket.hxx"
#include "rutil/Logger.hxx"
#include "rutil/ParseBuffer.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Lock.hxx"
#include "rutil/TransportType.hxx"
#include "resip/stack/Uri.hxx"
#include "resip/stack/ConnectionManager.hxx"
#include "resip/stack/SipMessage.hxx"

#include "repro/AclStore.hxx"

#ifdef USE_SSL
#include "resip/stack/ssl/TlsConnection.hxx"
#include "resip/stack/ssl/TlsTransport.hxx"
#endif

using namespace resip;
using namespace repro;
using namespace std;


#define RESIPROCATE_SUBSYSTEM Subsystem::REPRO

AclStore::AclStore(AbstractDb& db):
   mDb(db)
{  
   AbstractDb::Key key = mDb.firstAclKey();
   while ( !key.empty() )
   {
      AbstractDb::AclRecord rec = mDb.getAcl(key);
      if(rec.mTlsPeerName.empty())  // If there is no TlsPeerName then record is an Address ACL
      {
         AddressRecord addressRecord(rec.mAddress, rec.mPort, (resip::TransportType)rec.mTransport);
         addressRecord.mMask = rec.mMask;
         addressRecord.key = buildKey(Data::Empty, rec.mAddress, rec.mMask, rec.mPort, rec.mFamily, rec.mTransport);
         mAddressList.push_back(addressRecord);
      }
      else
      {
         TlsPeerNameRecord tlsPeerNameRecord;
         tlsPeerNameRecord.mTlsPeerName = rec.mTlsPeerName;
         tlsPeerNameRecord.key = buildKey(rec.mTlsPeerName, Data::Empty, 0, 0, 0, 0);
         mTlsPeerNameList.push_back(tlsPeerNameRecord); 
      }
      key = mDb.nextAclKey();
   } 
   mTlsPeerNameCursor = mTlsPeerNameList.begin();
   mAddressCursor = mAddressList.begin();
}

AclStore::~AclStore()
{
}

bool
AclStore::addAcl(const resip::Data& tlsPeerName,
                  const resip::Data& address,
                  const short& mask,
                  const short& port,
                  const short& family,
                  const short& transport)
{ 
   Data key = buildKey(tlsPeerName, address, mask, port, family, transport);
   InfoLog( << "Add ACL: key=" << key);
   
   // Check if key exists already
   if(key.prefix(":"))  // a key that starts with a : has no peer name - thus a Address key
   {
      ReadLock lock(mMutex);
      if(findAddressKey(key))
      {
         // Key already exists - don't add again
         return false;
      }
   }
   else
   {
      ReadLock lock(mMutex);
      if(findTlsPeerNameKey(key))
      {
         // Key already exists - don't add again
         return false;
      }
   }

   // Key doesn't exist - add it
   AbstractDb::AclRecord rec;
   rec.mTlsPeerName = tlsPeerName;
   rec.mAddress = address;
   rec.mMask = mask;
   rec.mPort = port;
   rec.mFamily = family;
   rec.mTransport = transport;

   // Add DB record
   if(!mDb.addAcl(key, rec))
   {
      return false;
   }

   // Add local storage
   if(rec.mTlsPeerName.empty())  // If there is no TlsPeerName then record is an Address ACL
   {
      AddressRecord addressRecord(rec.mAddress, rec.mPort, (resip::TransportType)rec.mTransport);
      addressRecord.mMask = rec.mMask;
      addressRecord.key = key;
      {
         WriteLock lock(mMutex);
         mAddressList.push_back(addressRecord);
         mAddressCursor = mAddressList.begin();  // Put cursor back at start
      }
   }
   else
   {
      TlsPeerNameRecord tlsPeerNameRecord;
      tlsPeerNameRecord.mTlsPeerName = rec.mTlsPeerName;
      tlsPeerNameRecord.key = key;
      {
         WriteLock lock(mMutex);
         mTlsPeerNameList.push_back(tlsPeerNameRecord); 
         mTlsPeerNameCursor = mTlsPeerNameList.begin(); // Put cursor back at start
      }
   }
   return true;
}


bool 
AclStore::addAcl(const resip::Data& tlsPeerNameOrAddress,
                  const short& port,
                  const short& transport)
{
   // Input can be in any of these formats
   // localhost         localhost  (becomes 127.0.0.1/8, ::1/128 and fe80::1/64)
   // bare hostname     server1
   // FQDN              server1.example.com
   // IPv4 address      192.168.1.100
   // IPv4 + mask       192.168.1.0/24
   // IPv6 address      :341:0:23:4bb:0011:2435:abcd
   // IPv6 + mask       :341:0:23:4bb:0011:2435:abcd/80
   // IPv6 reference    [:341:0:23:4bb:0011:2435:abcd]
   // IPv6 ref + mask   [:341:0:23:4bb:0011:2435:abcd]/64

   try
   {
      ParseBuffer pb(tlsPeerNameOrAddress);
      const char* anchor = pb.start();

      bool ipv4 = false;
      bool ipv6 = false;
      Data hostOrIp;
      //u_char in[28];
      struct in_addr in4;
#ifdef USE_IPV6
      struct in6_addr in6;
#endif
      int mask;

      if (*pb.position() == '[')   // encountered beginning of IPv6 reference
      {
         anchor = pb.skipChar();
         pb.skipToEndQuote(']');

         pb.data(hostOrIp, anchor);  // copy the presentation form of the IPv6 address
         anchor = pb.skipChar();

         // try to convert into IPv6 network form
#ifdef USE_IPV6
         if (!DnsUtil::inet_pton( hostOrIp.c_str(), in6)) 
#endif
         {
            return false;
         }
         ipv6 = true;
      }
      else
      {
         pb.skipToOneOf(".:");
         if (pb.position() == pb.end())   // We probably have a bare hostname
         {
            pb.data(hostOrIp, anchor);
            if (hostOrIp.lowercase() == "localhost")
            {
               // add special localhost addresses for v4 and v6 to list and return
               addAcl(Data::Empty, "127.0.0.1", 8, port, resip::V4, transport);
               addAcl(Data::Empty, "::1", 128, port, resip::V6, transport);
               return addAcl(Data::Empty, "fe80::1", 64, port, resip::V6, transport);
            }
            else
            {
               // hostOrIp += default domain name (future)
               return addAcl(hostOrIp, Data::Empty, 0, 0, 0, 0);
            }
         }
         else if (*pb.position() == ':')     // Must be an IPv6 address
         {
            pb.skipToChar('/');
            pb.data(hostOrIp, anchor);  // copy the presentation form of the IPv6 address

            // try to convert into IPv6 network form
#ifdef USE_IPV6
            if (!DnsUtil::inet_pton( hostOrIp.c_str(), in6)) 
#endif
            {
               return false;
            }
            ipv6 = true;
         }
         else // *pb.position() == '.'
         {
            // Could be either an IPv4 address or an FQDN
            pb.skipToChar('/');
            pb.data(hostOrIp, anchor);  // copy the presentation form of the address

            // try to interpret as an IPv4 address, if that fails look it up in DNS
            if (DnsUtil::inet_pton( hostOrIp.c_str(), in4)) 
            {
               // it was an IPv4 address
               ipv4 = true;
            }
            else
            {
               // hopefully it is a legal FQDN, try it.
               return addAcl(hostOrIp, Data::Empty, 0, 0, 0, 0);
            }
         }   
      }

      if (!pb.eof() && *pb.position() == '/')    // grab the mask as well
      {
         anchor = pb.skipChar();
         mask = pb.integer();

         if (ipv4)
         {
            if (mask < 8 || mask > 32)
            {
               return false;
            }
         }
         else if (ipv6)
         {
            if (mask < 64 || mask > 128)
            {
               return false;
            }
         }
      }
      else
      {
         if (ipv4)
         {
            mask = 32;
         }
         else // ipv6
         {
            mask = 128;
         }
      }

      if(pb.eof())
      {
         bool ret;
         if (ipv6)
         {
            ret = addAcl(Data::Empty, hostOrIp, mask, port, resip::V6, transport);
         }

         if (ipv4)
         {
            ret = addAcl(Data::Empty, hostOrIp, mask, port, resip::V4, transport);
         }
         return ret;
      }      
   }
   catch(ParseException& e)
   {
      ErrLog(<< "Exception caught:" << e);
   }
   return false;
}

void AclStore::eraseAcl(const resip::Data& tlsPeerName,
                        const resip::Data& address,
                        const short& mask,
                        const short& port,
                        const short& family,
                        const short& transport)
{
   Data key = buildKey(tlsPeerName, address, mask, port, family, transport);
   InfoLog( << "Erase ACL: key=" << key);
   eraseAcl(key);
}

void 
AclStore::eraseAcl(const resip::Data& key)
{  
   // Erase DB record
   mDb.eraseAcl( key );

   // Erase local storage
   if(key.prefix(":"))  // a key that starts with a : has no peer name - thus a Address key
   {
      WriteLock lock(mMutex);
      if(findAddressKey(key))
      {
         mAddressCursor = mAddressList.erase(mAddressCursor);
      }
   }
   else
   {
      WriteLock lock(mMutex);
      if(findTlsPeerNameKey(key))
      {
         mTlsPeerNameCursor = mTlsPeerNameList.erase(mTlsPeerNameCursor);
      }
   }
}


AbstractDb::Key 
AclStore::buildKey(const resip::Data& tlsPeerName,
                     const resip::Data& address,
                     const short& mask,
                     const short& port,
                     const short& family,
                     const short& transport) const
{  
   Data pKey = tlsPeerName+":"+address+"/"+Data(mask)+":"+Data(port)+":"+Data(family)+":"+Data(transport); 
   return pKey;
}


AclStore::Key 
AclStore::getFirstTlsPeerNameKey()
{
   ReadLock lock(mMutex);
   mTlsPeerNameCursor = mTlsPeerNameList.begin();
   if ( mTlsPeerNameCursor == mTlsPeerNameList.end() )
   {
      return Key( Data::Empty );
   }
   
   return mTlsPeerNameCursor->key;
}


bool 
AclStore::findTlsPeerNameKey(const Key& key)
{ 
   // check if cursor happens to be at the key
   if ( mTlsPeerNameCursor != mTlsPeerNameList.end() )
   {
      if ( mTlsPeerNameCursor->key == key )
      {
         return true;
      }
   }
   
   // search for the key 
   mTlsPeerNameCursor = mTlsPeerNameList.begin();
   while (  mTlsPeerNameCursor != mTlsPeerNameList.end() )
   {
      if ( mTlsPeerNameCursor->key == key )
      {
         return true; // found the key 
      }
      mTlsPeerNameCursor++;
   }
   return false; // key was not found 
}


AclStore::Key 
AclStore::getNextTlsPeerNameKey(Key& key)
{  
   ReadLock lock(mMutex);
   if ( !findTlsPeerNameKey(key) )
   {
      return Key(Data::Empty);
   }
      
   mTlsPeerNameCursor++;
   
   if ( mTlsPeerNameCursor == mTlsPeerNameList.end() )
   {
      return Key( Data::Empty );
   }
   
   return mTlsPeerNameCursor->key;
}


AclStore::Key 
AclStore::getFirstAddressKey()
{
   ReadLock lock(mMutex);
   mAddressCursor = mAddressList.begin();
   if ( mAddressCursor == mAddressList.end() )
   {
      return Key( Data::Empty );
   }
   
   return mAddressCursor->key;
}


bool 
AclStore::findAddressKey(const Key& key)
{ 
   // check if cursor happens to be at the key
   if ( mAddressCursor != mAddressList.end() )
   {
      if ( mAddressCursor->key == key )
      {
         return true;
      }
   }
   
   // search for the key 
   mAddressCursor = mAddressList.begin();
   while (  mAddressCursor != mAddressList.end() )
   {
      if ( mAddressCursor->key == key )
      {
         return true; // found the key 
      }
      mAddressCursor++;
   }
   return false; // key was not found 
}


AclStore::Key 
AclStore::getNextAddressKey(Key& key)
{  
   ReadLock lock(mMutex);
   if ( !findAddressKey(key) )
   {
      return Key(Data::Empty);
   }
      
   mAddressCursor++;
   
   if ( mAddressCursor == mAddressList.end() )
   {
      return Key( Data::Empty );
   }
   
   return mAddressCursor->key;
}


resip::Data 
AclStore::getTlsPeerName( const resip::Data& key )
{
   ReadLock lock(mMutex);
   if ( !findTlsPeerNameKey(key) )
   {
      return Data::Empty;
   }
   return mTlsPeerNameCursor->mTlsPeerName;
}


resip::Tuple 
AclStore::getAddressTuple( const resip::Data& key )
{
   ReadLock lock(mMutex);
   if ( !findAddressKey(key) )
   {
      return Tuple();
   }
   return mAddressCursor->mAddressTuple;
}
 

short 
AclStore::getAddressMask( const resip::Data& key )
{
   ReadLock lock(mMutex);
   if ( !findAddressKey(key) )
   {
      return 0;
   }
   return mAddressCursor->mMask;
}

      
bool 
AclStore::isTlsPeerNameTrusted(const std::list<Data>& tlsPeerNames)
{
   ReadLock lock(mMutex);
   for(std::list<Data>::const_iterator it = tlsPeerNames.begin(); it != tlsPeerNames.end(); it++)
   {
      for(TlsPeerNameList::iterator i = mTlsPeerNameList.begin(); i != mTlsPeerNameList.end(); i++)
      {
         if(isEqualNoCase(i->mTlsPeerName, *it))
         {
            InfoLog (<< "AclStore - Tls peer name IS trusted: " << *it);
            return true;
         }
      }
   }
   return false;
}
 

bool 
AclStore::isAddressTrusted(const Tuple& address)
{
   ReadLock lock(mMutex);
   for(AddressList::iterator i = mAddressList.begin(); i != mAddressList.end(); i++)
   {
      if(i->mAddressTuple.isEqualWithMask(address, i->mMask, i->mAddressTuple.getPort() == 0))
      {
         return true;
      }
   }
   return false;
}


// check the sender of the message via source IP address or identity from TLS 
bool
AclStore::isRequestTrusted(const SipMessage& request)
{
   bool trusted = false;
   Tuple source = request.getSource();
   
   // check if the request came over a secure channel and sucessfully authenticated 
   // (ex: TLS or DTLS)
   TransportType receivedTransport = toTransportType(
      request.header(h_Vias).front().transport());
#ifdef USE_SSL
   if(isSecure(receivedTransport))
   {
      StackLog(<<"Not checking the TLS peer certificate names, that is now done by CertificateAuthenticator if enabled");
   }
#endif

   // check the source address against the TrustedNode list
   if(!trusted)
   {
      if(isAddressTrusted(source))
      {
         InfoLog (<< "AclStore - source address IS trusted: " << source.presentationFormat() << ":" << source.getPort() << " " << Tuple::toData(source.getType()));
         trusted = true;
      }
      else
      {
         InfoLog (<< "AclStore - source address NOT trusted: " << source.presentationFormat() << ":" << source.getPort() << " " << Tuple::toData(source.getType()));
      }
   }      
      
   return trusted;
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
 */
