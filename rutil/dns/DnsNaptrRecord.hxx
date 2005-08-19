#ifndef RESIP_DNS_NAPTR_RECORD
#define RESIP_DNS_NAPTR_RECORD

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"

namespace resip
{

class Data;
class DnsResourceRecord;
class RROverlay;
class BaseException;

class DnsNaptrRecord : public DnsResourceRecord
{
   public:
      class NaptrException : public BaseException
      {
         public:
            NaptrException(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) 
            {
            }
            
            const char* name() const { return "NaptrException"; }
      };

      DnsNaptrRecord(const RROverlay&);
      ~DnsNaptrRecord() {}

      // accessors.
      int order() const { return mOrder; }
      int& order() { return mOrder; }
      int preference() const { return mPreference; }
      const Data& flags() const { return mFlags; }
      const Data& service() const { return mService; }
      const Data& regexp() const { return mRegexp; }
      const Data& replacement() const { return mReplacement; }
      const Data& name() const { return mName; }
      bool isSameValue(const Data& value) const;
      
   private:
      int mOrder;
      int mPreference;
      Data mFlags;
      Data mService;
      Data mRegexp;
      Data mReplacement;
      Data mName;

};

}


#endif
