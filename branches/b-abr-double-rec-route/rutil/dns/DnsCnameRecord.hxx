#ifndef RESIP_DNS_CNAME_RECORD
#define RESIP_DNS_CNAME_RECORD

#include "rutil/Data.hxx"
#include "rutil/Socket.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/BaseException.hxx"

namespace resip
{

class RROverlay;

class DnsCnameRecord : public DnsResourceRecord
{
   public:
      class CnameException : public BaseException
      {
         public:
            CnameException(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) 
            {
            }
            
            const char* name() const { return "CnameException"; }
      };

      DnsCnameRecord(const RROverlay&);
      ~DnsCnameRecord() {}

      // accessors.
      const Data& cname() const { return mCname; }
      const Data& name() const { return mName; }
      bool isSameValue(const Data&) const;
      
   private:
      Data mCname;
      Data mName;
};

}


#endif
