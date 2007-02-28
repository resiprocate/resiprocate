#ifndef RESIP_RRGREYLIST_HXX
#define RESIP_RRGREYLIST_HXX

#include "rutil/dns/DnsStub.hxx"

namespace resip
{

class RRGreylist : public DnsStub::ResultTransform
{
   public:
      RRGreylist();
      ~RRGreylist();

      void greylist(const Data& target, int rrType, const Data& result);
      void removeGreylist(const Data& target, int rrType);
      void removeFromGreylist(const Data& target, int rrType, const Data& result);
      void transform(const Data& target, int rrType, std::vector<DnsResourceRecord*>&);

      static void setGreylistDuration(time_t ms);
      static time_t getGreylistDuration();
   private:

      RRGreylist(const RRGreylist&);
      RRGreylist& operator=(const RRGreylist&);

      static time_t theGreylistDurationMs;
      typedef std::vector<DnsResourceRecord*> RRVector;

      typedef struct
      {
         resip::Data result;
         time_t expiry;
      } ResultWithExpiry;

      class Transform
      {
         public:
            Transform();
            virtual ~Transform();
            virtual bool transform(RRVector& rrs);
            void add(const Data& result);
            void remove(const Data& result);
            bool isEmpty() const;
            void refresh();

         protected:
            typedef std::list<ResultWithExpiry> Greylist;
            Greylist mGreylist; // ip for a/aaaa, target host for srv, and replacement for naptr.
            bool isGreylisted(DnsResourceRecord* rr);
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

      typedef std::map<MapKey, Transform*> TransformMap;
      TransformMap mTransforms;  
};

}

#endif
