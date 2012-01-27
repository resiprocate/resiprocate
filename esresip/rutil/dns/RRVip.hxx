/* ***********************************************************************
   Copyright 2006-2007 Estacado Systems, LLC. All rights reserved.
 *********************************************************************** */

#ifndef RESIP_RRVIP_HXX
#define RESIP_RRVIP_HXX

#include "rutil/dns/DnsStub.hxx"

namespace resip
{

/**
   @ingroup resip_dns
*/
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

      /**
         @internal
         @ingroup resip_dns
      */
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

      /**
         @internal
         @ingroup resip_dns
      */
      class NaptrTransform : public Transform
      {
         public:
            NaptrTransform(const Data& vip);
            void transform(RRVector& rrs, bool&);
      };

      /**
         @internal
         @ingroup resip_dns
      */
      class SrvTransform : public Transform
      {
         public:
            SrvTransform(const Data& vip);
            void transform(RRVector& rrs, bool&);
      };

      /**
         @internal
         @ingroup resip_dns
      */
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

      /**
         @internal
         @ingroup resip_dns
      */
      class TransformFactory
      {
         public:
            virtual ~TransformFactory() {}
            virtual Transform* createTransform(const Data& vip) = 0;
      };

      /**
         @internal
         @ingroup resip_dns
      */
      class HostTransformFactory : public TransformFactory
      {
         public:
            Transform* createTransform(const Data& vip) { return new Transform(vip); }
      };

      /**
         @internal
         @ingroup resip_dns
      */
      class NaptrTransformFactroy : public TransformFactory
      {
         public:
            Transform* createTransform(const Data& vip) { return new NaptrTransform(vip); }
      };

      /**
         @internal
         @ingroup resip_dns
      */
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
