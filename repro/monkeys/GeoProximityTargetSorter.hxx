#ifndef GEO_PROXIMITY_TARGET_SORTER_HXX
#define GEO_PROXIMITY_TARGET_SORTER_HXX 1

#ifndef RESIP_FIXED_POINT

#ifdef WIN32
#include <pcreposix.h>
#else
#include <regex.h>
#endif

#ifdef USE_MAXMIND_GEOIP
// MaxMind GeoIP header
#include <GeoIP.h>
#include <GeoIPCity.h>
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

class GeoProximityTargetSorter : public Processor
{
   public:

      static resip::KeyValueStore::Key mGeoTargetSortingDoneKey;

      GeoProximityTargetSorter(ProxyConfig& config);
      virtual ~GeoProximityTargetSorter();
      
      virtual processor_action_t process(RequestContext &);
      virtual void dump(EncodeStream &os) const;
         
   protected:
      void getClientGeoLocation(const resip::SipMessage& request, double& latitude, double& longitude);
      void getTargetGeoLocation(const Target& target, double& latitude, double& longitude);
      double getTargetDistance(const Target& target, double clientLatitude, double clientLongitude);
      void parseGeoLocationParameter(const resip::Data& parameter, double& latitude, double& longitude);
      double calculateDistance(double latitude1, double longitude1, double latitude2, double longitude2);
      bool geoIPLookup(const resip::Tuple& address, double& latitude, double& longitude);

      resip::Data mRUriRegularExpressionData;
      regex_t *mRUriRegularExpression;

      unsigned long mDefaultDistance;
      bool mLoadBalanceEqualDistantTargets;
#ifdef USE_MAXMIND_GEOIP
      GeoIP *mGeoIP;
#endif
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
