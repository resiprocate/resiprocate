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

      class RegExp
      {
         public:
            // Takes a regexp expression as defined in rfc 2915 (section 3
            // Substitution Expression Grammar) The delimiter is whatever
            // appears in the first character. This can be empty. 
            RegExp(const Data& data);
            RegExp();
            ~RegExp();
            
            bool empty() const;
            
            // Convenience method provided for access to the antecedent of the
            // regexp (the matching regular expression) 
            const Data& regexp() const;

            // Convenience method provided for access to the consequent of the
            // regexp (the replacement)  - this must be a URI
            const Data& replacement() const;
            
            // Applies the regular expression substitution based on the input
            // string. Will return Data::Empty if the input does not match the
            // substitution. 
            Data apply(const Data& input) const;
            
         private:
            Data mRegexp;
            Data mReplacement;
            Data flags;
            //regex_t mRe;
      };
      
         
      DnsNaptrRecord() : mOrder(-1), mPreference(-1) {}
      DnsNaptrRecord(const RROverlay&);
      ~DnsNaptrRecord() {}

      // accessors.
      int order() const { return mOrder; }
      int& order() { return mOrder; }
      int preference() const { return mPreference; }
      const Data& flags() const { return mFlags; }
      const Data& service() const { return mService; }
      const RegExp& regexp() const { return mRegexp; }
      const Data& replacement() const { return mReplacement; }
      const Data& name() const { return mName; }
      bool isSameValue(const Data& value) const;
      
   private:
      int mOrder;
      int mPreference;
      Data mFlags;
      Data mService;
      RegExp mRegexp;
      Data mReplacement;
      Data mName;

};

}


#endif
