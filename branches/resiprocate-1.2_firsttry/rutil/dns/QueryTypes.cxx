#include "QueryTypes.hxx"

using namespace resip;

#define defineQueryType(_name, _type, _rrType)  \
   Data RR_##_name::getRRTypeName()             \
{                                               \
   return #_name;                               \
}                                               \
unsigned short RR_##_name::getRRType()          \
{                                               \
   return _rrType;                              \
}                                               \
RR_##_name resip::q_##_name

defineQueryType(A, DnsHostRecord, 1);
defineQueryType(CNAME, DnsCnameRecord, 5);

#ifdef USE_IPV6
defineQueryType(AAAA, DnsAAAARecord, 28);
#endif

defineQueryType(SRV, DnsSrvRecord, 33);
defineQueryType(NAPTR, DnsNaptrRecord, 35);

