#if !defined(RESIP_WEBADMIN_HXX)
#define RESIP_WEBADMIN_HXX 

#include "rutil/Data.hxx"
#include "rutil/Condition.hxx"
#include "rutil/dns/DnsStub.hxx"
#include "rutil/TransportType.hxx"
#include "resip/stack/Tuple.hxx"
#include "repro/HttpBase.hxx"

#include <map>

namespace resip
{
class RegistrationPersistenceManager;
class PublicationPersistenceManager;
class Security;
class DataStream;
}


namespace repro
{
class Store;
class UserStore;
class RouteStore;
typedef std::map<resip::Data, resip::Data> Dictionary;
class Proxy;

class WebAdmin : public HttpBase,
                 public resip::GetDnsCacheDumpHandler
{
   public:
      class ConfigException : public resip::BaseException
      {
         public:
            ConfigException(const resip::Data& msg,
                      const resip::Data& file,
                      const int line)
               : BaseException(msg, file, line) {}
         protected:
            virtual const char* name() const { return "WebAdmin::ConfigException"; }
      };

      WebAdmin(Proxy& proxy,
               resip::RegistrationPersistenceManager& regDb,
               resip::PublicationPersistenceManager& pubDb,
               const resip::Data& realm, // this realm is used for http challenges
               int port=5080,
               resip::IpVersion version=resip::V4,
               const resip::Data& ipAddr = resip::Data::Empty);

      // (Re)load the users.txt file
      void parseUserFile();
      
   protected:
      virtual void buildPage( const resip::Data& uri, 
                              int pageNumber,
                              const resip::Data& user,
                              const resip::Data& password);

      // Handler
      virtual void onDnsCacheDumpRetrieved(std::pair<unsigned long, unsigned long> key, const resip::Data& dnsEntryStrings);

   private: 
      resip::Data buildDefaultPage();
      resip::Data buildUserPage();
      
      void buildDomainsSubPage(resip::DataStream& s);
      void buildAclsSubPage(resip::DataStream& s);

      void buildAddUserSubPage(resip::DataStream& s);
      void buildEditUserSubPage(resip::DataStream& s);
      void buildShowUsersSubPage(resip::DataStream& s);

      void buildAddFilterSubPage(resip::DataStream& s);
      void buildEditFilterSubPage(resip::DataStream& s);
      void buildShowFiltersSubPage(resip::DataStream& s);

      void buildAddRouteSubPage(resip::DataStream& s);
      void buildEditRouteSubPage(resip::DataStream& s);
      void buildShowRoutesSubPage(resip::DataStream& s);

      void buildRegistrationsSubPage(resip::DataStream& s);
      void buildPublicationsSubPage(resip::DataStream& s);
      void buildSettingsSubPage(resip::DataStream& s);
      void buildRestartSubPage(resip::DataStream& s);
      void buildLogLevelSubPage(resip::DataStream& s);

      resip::Data buildCertPage(const resip::Data& domain);

      Proxy& mProxy;
      Store& mStore;
      resip::RegistrationPersistenceManager& mRegDb;
      resip::PublicationPersistenceManager& mPubDb;

      resip::Data mDnsCache;
      resip::Mutex mDnsCacheMutex;
      resip::Condition mDnsCacheCondition;
      
      bool mNoWebChallenges;
      
      Dictionary mHttpParams;
      
      // list of the keys of records that should be deleted
      class RemoveKey
      {
      public:
         RemoveKey(const resip::Data &key1, const resip::Data &key2);
         bool operator<(const RemoveKey& rhs) const;
         resip::Data mKey1;
         resip::Data mKey2;
      };
      std::set<RemoveKey> mRemoveSet;

      resip::Data mPageOutlinePre;
      resip::Data mPageOutlinePost;

      resip::Data mUserFile;
      std::map<resip::Data,resip::Data> mUsers;
};



}

#endif  

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
