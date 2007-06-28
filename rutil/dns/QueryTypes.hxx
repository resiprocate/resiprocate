#ifndef RESIP_QUERY_TYPES_HXX
#define RESIP_QUERY_TYPES_HXX

#include "rutil/HeapInstanceCounter.hxx"
#include "rutil/compat.hxx"
#include "rutil/Socket.hxx"
#include "rutil/BaseException.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsHostRecord.hxx"
#include "DnsCnameRecord.hxx"
#include "DnsAAAARecord.hxx"
#include "DnsSrvRecord.hxx"
#include "DnsNaptrRecord.hxx"

//'Interface' of the RRType concept
// class QueryType
// {
//   public:
//       static unsigned short getRRType();
//       enum SupportsCName; //does following CNAME apply to this query type
// };


//!dcm! -- add UnusedChecking(_enum) below;

namespace resip
{

#define defineQueryType(_name, _type, _rrType, _supportsCName, _rfc)    \
class RR_##_name                                                        \
{                                                                       \
   public:                                                              \
      RESIP_HeapCount(RR_##_name);                                    \
      enum {SupportsCName = _supportsCName};                            \
      static unsigned short getRRType();                                \
      typedef _type Type;                                               \
      static Data getRRTypeName();                                          \
};                                                                      \
extern RR_##_name q_##_name

defineQueryType(A, DnsHostRecord, 1, true, "RFC 1035");
defineQueryType(CNAME, DnsCnameRecord, 5, false, "RFC 1035");

#ifdef USE_IPV6
defineQueryType(AAAA, DnsAAAARecord, 28, true, "RFC 3596");
#endif

defineQueryType(SRV, DnsSrvRecord, 33, true, "RFC 2782");
defineQueryType(NAPTR, DnsNaptrRecord, 35, true, "RFC 2915");

}

#undef defineQueryType

#endif
