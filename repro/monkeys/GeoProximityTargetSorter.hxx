#ifndef GEO_PROXIMITY_TARGET_SORTER_HXX
#define GEO_PROXIMITY_TARGET_SORTER_HXX 1

#ifndef RESIP_FIXED_POINT

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#include "repro/Processor.hxx"
#include "repro/ResponseContext.hxx"
#include "repro/ProxyConfig.hxx"

#include "rutil/Data.hxx"

namespace resip
{
   class SipMessage;
}

namespace repro
{

class RequestContext;

/*
  If enabled, then this baboon can post-process the target list.  
  This includes targets from the StaticRoute monkey and/or targets
  from the LocationServer monkey.  Requests that meet the filter 
  criteria will have their Target list, flatened (serialized) and
  ordered based on the proximity of the target to the client sending
  the request.  Proximity is determined by looking for a 
  x-repro-geolocation="<latitude>,<longitude>" parameter on the Contact
  header of a received request, or the Contact headers of Registration
  requests.  If this parameter is not found, then this processor will
  attempt to determine the public IP address closest to the client or
  target and use the MaxMind Geo IP library to lookup the geo location.

  There are several requirements for using this Processor/Baboon:
  1.  RESIP_FIXED_POINT preprocessor define is required to allow floating
      point operations required for distance calculations.
  2.  USE_MAXMIND_GEOIP preprocessor define is required to allow linking
      with the MaxMind Geo IP library for looking up lat/long information
      for an IP address.
  3.  A copy of the GeoIP City database is required for looking up lat/long
      information for an IP address.  A free version of the databases can 
      be downloaded from here:
      http://geolite.maxmind.com/download/geoip/database/GeoLiteCity.dat.gz
      and here (v6):
      http://geolite.maxmind.com/download/geoip/database/GeoLiteCityv6-beta/
      For a more accurate database, please see the details here:
      http://www.maxmind.com/app/city
      The location of these files are specifed in the following setting2:
      GeoProximityIPv4CityDatabaseFile and GeoProximityIPv6CityDatabaseFile
  4.  A request URI filter must be defined in the repro configuration.  This
      filter uses a PCRE compliant regular expression to attempt
      to match against the request URI of inbound requests.  Any requests
      matching this expression, will have its Targets sorted as described
      above.
      ie: GeoProximityRequestUriFilter = ^sip:mediaserver.*@mydomain.com$

   Some additional settings are:
   GeoProximityDefaultDistance - The distance (in Kilometers) to use for 
      proximity sorting, when the Geo Location of a target cannot be 
      determined. 

   LoadBalanceEqualDistantTargets - If enabled, then targets that are 
      determined to be of equal distance from the client, will be placed in 
      a random order.
*/


class GeoProximityTargetSorter : public Processor
{
   public:

      static resip::KeyValueStore::Key mGeoTargetSortingDoneKey;

      GeoProximityTargetSorter(ProxyConfig& config);
      virtual ~GeoProximityTargetSorter();
      
      virtual processor_action_t process(RequestContext &);
      
      // static fn available for other parts of repro to access the geoip library
      // fills in any non-null field
      static bool geoIPLookup(const resip::Tuple& address, 
                              double* latitude, 
                              double* longitude, 
                              resip::Data* country=0, 
                              resip::Data* region=0, 
                              resip::Data* city=0);

   protected:
      void getClientGeoLocation(const resip::SipMessage& request, double& latitude, double& longitude);
      void getTargetGeoLocation(const Target& target, double& latitude, double& longitude);
      double getTargetDistance(const Target& target, double clientLatitude, double clientLongitude);
      void parseGeoLocationParameter(const resip::Data& parameter, double& latitude, double& longitude);
      double calculateDistance(double latitude1, double longitude1, double latitude2, double longitude2);

      resip::Data mRUriRegularExpressionData;
      regex_t* mRUriRegularExpression;

      unsigned long mDefaultDistance;
      bool mLoadBalanceEqualDistantTargets;

      // Use static instance of the GeoIP library
      // - allows static geoIPLookup method
      // - reduces memory in cases where multiple instances of GeoProximityTargetSorter are needed
      //   (take care when creating multipleinstances since static initialization is not mutexed)
      static void* mGeoIPv4;
      static void* mGeoIPv6;
};

}

#endif // ndef RESIP_FIXED_POINT

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
 */
