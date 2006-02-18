#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

#include "QueryTypes.hxx"

using namespace resip;

#define defineQueryType(_name, _type, _rrType)                    \
unsigned short RR_##_name::getRRType()                          \
{                                                                    \
   return _rrType;                                                   \
}                                                                    \
RR_##_name resip::q_##_name

defineQueryType(A, DnsHostRecord, 1);
defineQueryType(CNAME, DnsCnameRecord, 5);

#ifdef USE_IPV6
defineQueryType(AAAA, DnsAAAARecord, 28);
#endif

defineQueryType(SRV, DnsSrvRecord, 33);
defineQueryType(NAPTR, DnsNaptrRecord, 35);

