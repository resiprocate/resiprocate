#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifndef RESIP_FIXED_POINT
#include <algorithm>

#ifdef USE_MAXMIND_GEOIP
// MaxMind GeoIP header
#include <GeoIP.h>
#include <GeoIPCity.h>
#endif

#include "repro/monkeys/GeoProximityTargetSorter.hxx"

#include "repro/RequestContext.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/Proxy.hxx"
#include "repro/Target.hxx"

#include "resip/stack/ExtensionParameter.hxx"
#include "resip/stack/Helper.hxx"
#include "rutil/Data.hxx"
#include "rutil/Random.hxx"
#include "rutil/Logger.hxx"
#include "repro/ProxyConfig.hxx"
#include "rutil/WinLeakCheck.hxx"


#define RESIPROCATE_SUBSYSTEM resip::Subsystem::REPRO

using namespace resip;
using namespace repro;
using namespace std;

KeyValueStore::Key GeoProximityTargetSorter::mGeoTargetSortingDoneKey = Proxy::allocateRequestKeyValueStoreKey();

// Custom NameAddr parameter that Regitering party or request sender can use on their Contact 
// header to pass their geo location information to repro, so that a lookup in the MaxMind 
// Geo IP tables is not required
static ExtensionParameter p_geolocation("x-repro-geolocation");

void* GeoProximityTargetSorter::mGeoIPv4 = 0;
void* GeoProximityTargetSorter::mGeoIPv6 = 0;

class GeoProximityTargetContainer
{
public:
   GeoProximityTargetContainer(double distance, Target* target) : mDistance(distance), mTarget(target) {}
   ~GeoProximityTargetContainer() {} 

    // Sort targets by distance from client
    static bool instanceCompare(const GeoProximityTargetContainer& lhs, 
                                const GeoProximityTargetContainer& rhs)
    {
       return lhs.mDistance < rhs.mDistance;
    }

    Target* getTarget() { return mTarget; }
    double getDistance() { return mDistance; }

private:
   double mDistance;
   Target* mTarget;
};

GeoProximityTargetSorter::GeoProximityTargetSorter(ProxyConfig& config) :
   Processor("GeoProximityTargetHandler"),
   mRUriRegularExpressionData(config.getConfigData("GeoProximityRequestUriFilter", "")),
   mRUriRegularExpression(0),
   mDefaultDistance(config.getConfigUnsignedLong("GeoProximityDefaultDistance", 0)),
   mLoadBalanceEqualDistantTargets(config.getConfigBool("LoadBalanceEqualDistantTargets", true))
{
   int flags = REG_EXTENDED | REG_NOSUB;

   if(!mRUriRegularExpressionData.empty())
   {
      mRUriRegularExpression = new regex_t;
      int ret = regcomp(mRUriRegularExpression, mRUriRegularExpressionData.c_str(), flags);
      if( ret != 0 )
      {
         delete mRUriRegularExpression;
         ErrLog( << "GeoProximityRequestUriFilter rule has invalid match expression: "
                 << mRUriRegularExpressionData);
         mRUriRegularExpression = 0;
      }
   }
   else
   {
      mRUriRegularExpression = 0;
   }

#ifdef USE_MAXMIND_GEOIP
   // Initialize GeoIP library - load data
   Data geoIPv4Database = config.getConfigData("GeoProximityIPv4CityDatabaseFile", "GeoLiteCity.dat", false);
   if(!geoIPv4Database.empty())
   {
      mGeoIPv4 = (GeoIP*)GeoIP_open(geoIPv4Database.c_str(), GEOIP_MEMORY_CACHE);  // Cache entire DB in memory - could make this configurable
      if(mGeoIPv4 != 0)
      {
         InfoLog(<< "GeoProximityTargetSorter: IPv4 db info: " << GeoIP_database_info((GeoIP*)mGeoIPv4));

         int i = GeoIP_database_edition((GeoIP*)mGeoIPv4);
   
         if(i != GEOIP_CITY_EDITION_REV0 /* 6 */ &&
            i != GEOIP_CITY_EDITION_REV1  /* 2 */ &&
            i != GEOIP_CITYCONFIDENCE_EDITION /* 15 */ &&
            i != GEOIP_CITYCONFIDENCEDIST_EDITION /* 16 */)
         {
            // Wrong DB Type
            ErrLog(<< "GeoProximityTargetSorter: IPv4 GeoIP database is not of the correct type, IPv4 geo lookups will not take place.");
            GeoIP_delete((GeoIP*)mGeoIPv4);
            mGeoIPv4 = 0;
         }
      }
      else
      {
         ErrLog(<< "GeoProximityTargetSorter: Failed to open IPv4 GeoIP database, IPv4 geo lookups will not take place: " << geoIPv4Database);
      }
   }
   else
   {
      InfoLog(<< "GeoProximityTargetSorter: No IPv4 GeoIP database specified, IPv4 geo lookups will not take place.");
   }

#ifdef USE_IPV6
   Data geoIPv6Database = config.getConfigData("GeoProximityIPv6CityDatabaseFile", "GeoLiteCityv6.dat", false);
   if(!geoIPv6Database.empty())
   {
      mGeoIPv6 = (GeoIP*)GeoIP_open(geoIPv6Database.c_str(), GEOIP_MEMORY_CACHE);  // Cache entire DB in memory - could make this configurable
      if(mGeoIPv6 != 0)
      {
         InfoLog(<< "GeoProximityTargetSorter: IPv6 db info: " << GeoIP_database_info((GeoIP*)mGeoIPv6));

         int i = GeoIP_database_edition((GeoIP*)mGeoIPv6);

         if(i != GEOIP_CITY_EDITION_REV1_V6 /* 30 */ &&
            i != GEOIP_CITY_EDITION_REV0_V6  /* 31 */)
         {
            // Wrong DB Type
            ErrLog(<< "GeoProximityTargetSorter: IPv6 GeoIP database is not of the correct type, IPv6 geo lookups will not take place.");
            GeoIP_delete((GeoIP*)mGeoIPv6);
            mGeoIPv6 = 0;
         }
      }  
      else
      {
         ErrLog(<< "GeoProximityTargetSorter: Failed to open IPv6 GeoIP database, IPv6 geo lookups will not take place: " << geoIPv6Database);
      }
   }
   else
   {
      InfoLog(<< "GeoProximityTargetSorter: No IPv6 GeoIP database specified, IPv6 geo lookups will not take place.");
   }
#else
   mGeoIPv6 = 0;
#endif

#endif
}

GeoProximityTargetSorter::~GeoProximityTargetSorter()
{
   if(mRUriRegularExpression)
   {
      regfree(mRUriRegularExpression);
      delete mRUriRegularExpression;
      mRUriRegularExpression = 0;
   }
#ifdef USE_MAXMIND_GEOIP
   if(mGeoIPv4)
   {
      GeoIP_delete((GeoIP*)mGeoIPv4);
      mGeoIPv4 = 0;
   }
   if(mGeoIPv6)
   {
      GeoIP_delete((GeoIP*)mGeoIPv6);
      mGeoIPv6 = 0;
   }
#endif
}

Processor::processor_action_t
GeoProximityTargetSorter::process(RequestContext &rc)
{
   // Check if we've already run this Baboon or not - if not, then run it.  Baboons
   // are dispatched multiple times, once when the Monkeys are done, and once for each
   // response received (after the Lemurs have run).
   //
   // Note:  This check causes us to only sort the Targets once (ie. the first time 
   //        the Monkeys are complete).  In the future we may want to allow GeoProximity 
   //        sorting of multiple contacts returned in a 3xx response (when 
   //        RecursiveRedirect is enabled).
   if(!rc.getKeyValueStore().getBoolValue(mGeoTargetSortingDoneKey))
   {
      rc.getKeyValueStore().setBoolValue(mGeoTargetSortingDoneKey, true);
   
      ResponseContext& rsp=rc.getResponseContext();

      // If there is not 2 or more candidate targets then there is nothing to sort
      if(rsp.getCandidateTransactionMap().size() >= 2)
      {
         // This is the first run, no Targets should be active yet
         resip_assert(rsp.hasCandidateTransactions() && 
               !rsp.hasActiveTransactions() &&
               !rsp.hasTerminatedTransactions());

         // Check if request meets filter criteria
         if(!mRUriRegularExpressionData.empty())  // If empty we consider all URI's matched
         {
            if(mRUriRegularExpression)
            {
               if(regexec(mRUriRegularExpression, Data::from(rc.getOriginalRequest().header(h_RequestLine).uri()).c_str(), 0 /*ignored*/, 0 /*ignored*/, 0/*eflags*/) != 0)
               {
                  // did not match 
                  DebugLog( << "GeoProximityTargetSorter: Skipped - request URI "<< rc.getOriginalRequest().header(h_RequestLine).uri() << " did not match.");
                  return Processor::Continue;
               }
            }
            else
            {
               // Regular expression provided is bad - error was logged at startup time
               return Processor::Continue;
            }
         }

         SipMessage& request = rc.getOriginalRequest();
         double clientLatitude;
         double clientLongitude;
         getClientGeoLocation(request, clientLatitude, clientLongitude);

         DebugLog(<< "GeoProximityTargetSorter: numCandidates=" << rsp.getCandidateTransactionMap().size() 
                 << ", clientLatitude=" << clientLatitude << ", clientLongitude=" << clientLongitude);

         std::vector<GeoProximityTargetContainer> flatTargetList;

         std::list<std::list<resip::Data> >& targetCollection = rsp.mTransactionQueueCollection;
         std::list<std::list<resip::Data> >::iterator outer = targetCollection.begin();
         std::list<resip::Data>::iterator inner;
         int outerCounter=1;
         for(; outer!=targetCollection.end(); outer++, outerCounter++)
         {
            inner=outer->begin();
            for(; inner != outer->end(); inner++)
            {
               resip_assert(rsp.isCandidate(*inner));
               Target* target = rsp.getTarget(*inner);
               if(target)
               {
                  resip::ContactInstanceRecord& rec = target->rec();
                  double distance = getTargetDistance(*target, clientLatitude, clientLongitude);
         
                  if(rec.mPublicAddress.getType() != UNKNOWN_TRANSPORT)
                  {
                     DebugLog(<< "GeoProximityTargetSorter: TransactionQueueCollection[" << outerCounter << "]: Target=" << rec.mContact 
                              << ", PublicAddress=" << rec.mPublicAddress
                              << ", DistanceFromClient=" << distance);
                  }
                  else
                  {
                     DebugLog(<< "GeoProximityTargetSorter: TransactionQueueCollection[" << outerCounter << "]: Target=" << rec.mContact 
                              << ", DistanceFromClient=" << distance);
                  }
   
                  // Flatten batches - since we will be serial routing
                  flatTargetList.push_back(GeoProximityTargetContainer(distance, target));
               }
               else
               {
                  WarningLog(<< "GeoProximityTargetSorter: TransactionQueueCollection[" << outerCounter << "]: TID=" << *inner << " - Couldn't find target");
               }
            }
         }

         if(mLoadBalanceEqualDistantTargets)
         {
            // Shuffle Targets - so that targets of equal distance will be in random order
            random_shuffle(flatTargetList.begin(), flatTargetList.end());
         }

         // Sort List by distance - note:  if we don't know the client location, there is no point in sorting
         // just use the newly flattened / shuffled target list in the existing order
         if(clientLatitude != 0 && clientLongitude != 0)
         {
            sort(flatTargetList.begin(), flatTargetList.end(), GeoProximityTargetContainer::instanceCompare);
         }

         // Rebuild targetCollection - Note:  All batches are split up to individual / serial targets
         targetCollection.clear();
         std::vector<GeoProximityTargetContainer>::iterator it = flatTargetList.begin();
         outerCounter=1;
         for(; it != flatTargetList.end(); it++, outerCounter++)
         {
            std::list<resip::Data> queue;
            queue.push_back(it->getTarget()->tid());
            targetCollection.push_back(queue);

            DebugLog(<< "GeoProximityTargetSorter: Sorted TransactionQueueCollection[" << outerCounter << "]: Target=" << it->getTarget()->rec().mContact 
                    << ", DistanceFromClient=" << it->getDistance());
         }
      }
      else
      {
         DebugLog( << "GeoProximityTargetSorter: Skipped since only one target for request URI "<< rc.getOriginalRequest().header(h_RequestLine).uri());
      }
   }

   return Processor::Continue;
}

void 
GeoProximityTargetSorter::getClientGeoLocation(const SipMessage& request, double& latitude, double& longitude)
{
   resip_assert(request.isRequest());

   // First check to see if x-repro-geolocation parameter is on Contact header
   if(request.exists(h_Contacts) && request.header(h_Contacts).size() >= 1)
   {
      // Only need to look at first contact - this won't be called for REGISTER messages or 3xx responses
      if(request.header(h_Contacts).front().exists(p_geolocation))
      {
         parseGeoLocationParameter(request.header(h_Contacts).front().param(p_geolocation), latitude, longitude);
         return;
      }
   }

   // If we cannot determine geo location of target - then return 0,0
   latitude = 0;
   longitude = 0;

   // Next - try and find the public IP of the client that sent the request
   Tuple publicAddress = Helper::getClientPublicAddress(request);
   if(publicAddress.getType() != UNKNOWN_TRANSPORT)
   {
      // Do a MaxMind GeoIP lookup to determine latitude and longitude
      geoIPLookup(publicAddress, &latitude, &longitude);
   }
}

void 
GeoProximityTargetSorter::getTargetGeoLocation(const Target& target, double& latitude, double& longitude)
{
   // First check to see if x-repro-geolocation parameter is on Contact header
   if(target.rec().mContact.exists(p_geolocation))
   {
      parseGeoLocationParameter(target.rec().mContact.param(p_geolocation), latitude, longitude);
      return;
   }

   // If we cannot determine geo location of client - then return 0,0
   latitude = 0;
   longitude = 0;

   // Next - see if we stored a public IP of the client at registration time
   if(target.rec().mPublicAddress.getType() != UNKNOWN_TRANSPORT)
   {
      // Do a MaxMind GeoIP lookup to determine latitude and longitude
      geoIPLookup(target.rec().mPublicAddress, &latitude, &longitude);
   }
   else
   {
      // Next - see if contact address is public or not
      Tuple contactAddress(target.rec().mContact.uri().host(), 0, UNKNOWN_TRANSPORT);
      if(!contactAddress.isPrivateAddress())
      {
         // Do a MaxMind GeoIP lookup to determine latitude and longitude
         geoIPLookup(contactAddress, &latitude, &longitude);
      }
   }
}

double 
GeoProximityTargetSorter::getTargetDistance(const Target& target, double clientLatitude, double clientLongitude)
{
   if(clientLatitude == 0 && clientLongitude == 0)
   {
      return (double)mDefaultDistance;
   }
   double targetLatitude;
   double targetLongitude;
   getTargetGeoLocation(target, targetLatitude, targetLongitude);
   if(targetLatitude == 0 && targetLongitude == 0)
   {
      return (double)mDefaultDistance;
   }

   return calculateDistance(clientLatitude, clientLongitude, targetLatitude, targetLongitude);
}

void 
GeoProximityTargetSorter::parseGeoLocationParameter(const Data& parameter, double& latitude, double& longitude)
{
   ParseBuffer pb(parameter);

   latitude = 0.0;
   longitude = 0.0;
   Data token;
   const char* anchor = pb.position();
   pb.skipToChar(Symbols::COMMA[0]);
   pb.data(token, anchor);
   latitude = token.convertDouble();
   if(!pb.eof())
   {
      pb.skipChar();
      if(!pb.eof())
      {
         anchor = pb.position();
         pb.skipToOneOf(",\"");  // Skip to comma or end double quote - be tolerant of altitude being specified
         pb.data(token, anchor);
         longitude = token.convertDouble();
         return;
      }
   }
   DebugLog(<< "GeoProximityTargetSorter: parseGeoLocationParameter - invalid parameter format: " << parameter);
}

static const double DEG_TO_RAD = 0.017453292519943295769236907684886;
static const double EARTH_RADIUS = 6372.797560856; // km
//static const double EARTH_RADIUS = 3963.0; // miles
double
GeoProximityTargetSorter::calculateDistance(double lat1, double long1, double lat2, double long2)
{
   double dLat  = (lat1 - lat2) * DEG_TO_RAD; 
   double dLong = (long1 - long2) * DEG_TO_RAD; 
   double hLat = sin(dLat * 0.5); 
   double hLong = sin(dLong * 0.5); 
   double distance;

   hLat *= hLat; 
   hLong *= hLong; 

   distance = 2.0 * asin(sqrt(hLat + (cos(lat1*DEG_TO_RAD) * cos(lat2*DEG_TO_RAD)) *hLong)) * EARTH_RADIUS; 
   return distance;
}

bool 
GeoProximityTargetSorter::geoIPLookup(const Tuple& address, double* latitude, double* longitude, Data* country, Data* region, Data* city)
{
#ifdef USE_MAXMIND_GEOIP
   GeoIPRecord *gir = 0;
   // ?slg? - should we introduce a local cache?  or is GeoIP library performance good enough?
   if(address.ipVersion() == V6)
   {
      if(mGeoIPv6)
      {
         gir = GeoIP_record_by_ipnum_v6((GeoIP*)mGeoIPv6, reinterpret_cast<const sockaddr_in6&>(address.getSockaddr()).sin6_addr);
      }
   }
   else
   {
      if(mGeoIPv4)
      {
         gir = GeoIP_record_by_ipnum((GeoIP*)mGeoIPv4, ntohl(reinterpret_cast<const sockaddr_in&>(address.getSockaddr()).sin_addr.s_addr));
      }
   }
   
   if(gir != 0)
   {
      Data countryData(Data::Share, gir->country_code ? gir->country_code : "");
      Data regionData(Data::Share, gir->region ? gir->region : "");
      Data cityData(Data::Share, gir->city ? gir->city : "");
      if(latitude) *latitude = gir->latitude;
      if(longitude) *longitude = gir->longitude;
      if(country) *country = countryData;
      if(region) *region = regionData;
      if(city) *city = cityData;

      DebugLog(<< "GeoProximityTargetSorter::geoIPLookup: Tuple=" << address 
               << ", Country=" << countryData
               << ", Region=" << regionData
               << ", City=" << cityData
               << ", Lat/Long=" << gir->latitude << "/" << gir->longitude);

      GeoIPRecord_delete(gir);
      return true;
   }
   else
   {
      DebugLog(<< "GeoProximityTargetSorter::geoIPLookup: no geo location information found for Tuple=" << address);
   }
#endif

   return false;
}


#endif  // ndef RESIP_FIXED_POINT

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
