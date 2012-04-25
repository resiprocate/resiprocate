#ifndef RESIP_RRVIP_HXX
#define RESIP_RRVIP_HXX

#include "rutil/dns/DnsStub.hxx"

namespace resip
{

class RRVip : public DnsStub::ResultTransform
{
   public:
      RRVip();
      ~RRVip();

      void vip(const Data& target, int rrType, const Data& vip);
      void removeVip(const Data& target, int rrType);
      void transform(const Data& target, int rrType, std::vector<DnsResourceRecord*>&);

   private:

      RRVip(const RRVip&);
      RRVip& operator=(const RRVip&);

      typedef std::vector<DnsResourceRecord*> RRVector;
      class Transform
      {
         public:
            Transform(const Data& vip);
            virtual ~Transform();
            virtual void transform(RRVector& rrs, bool& invalidVip);
            void updateVip(const Data& vip);
            const Data& vip() { return mVip; };

         protected:
            Data mVip; // ip for a/aaaa, target host for srv, and replacement for naptr.
      };

      class NaptrTransform : public Transform
      {
         public:
            NaptrTransform(const Data& vip);
            void transform(RRVector& rrs, bool&);
      };

      class SrvTransform : public Transform
      {
         public:
            SrvTransform(const Data& vip);
            void transform(RRVector& rrs, bool&);
      };

      class MapKey
      {
         public:
            MapKey();
            MapKey(const Data& target, int rrType);
            bool operator<(const MapKey&) const;
         private:
            Data mTarget;
            int mRRType;
      };

      class TransformFactory
      {
         public:
            virtual ~TransformFactory() {}
            virtual Transform* createTransform(const Data& vip) = 0;
      };

      class HostTransformFactory : public TransformFactory
      {
         public:
            Transform* createTransform(const Data& vip) { return new Transform(vip); }
      };

      class NaptrTransformFactroy : public TransformFactory
      {
         public:
            Transform* createTransform(const Data& vip) { return new NaptrTransform(vip); }
      };

      class SrvTransformFactory : public TransformFactory
      {
         public:
            Transform* createTransform(const Data& vip) { return new SrvTransform(vip); }
      };

      typedef std::map<int, TransformFactory*> TransformFactoryMap;
      TransformFactoryMap  mFactories;

      typedef std::map<MapKey, Transform*> TransformMap;
      TransformMap mTransforms;  
};

}

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000-2005 Vovida Networks, Inc.  All rights reserved.
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
