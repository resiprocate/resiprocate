#include "resiprocate/os/Socket.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "RROverlay.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsAAAARecord.hxx"

using namespace resip;

#ifdef USE_IPV6

DnsAAAARecord::DnsAAAARecord(const RROverlay& overlay)
{
   memcpy(&mAddr, overlay.data(), sizeof(in6_addr));
}

#endif
