
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/BaseException.hxx"
#include "RROverlay.hxx"
#include "DnsResourceRecord.hxx"
#include "DnsHostRecord.hxx"

using namespace resip;

DnsHostRecord::DnsHostRecord(const RROverlay& overlay)
{
   memcpy(&mAddr, overlay.data(), sizeof(in_addr));
}

Data DnsHostRecord::host() const
{
   return Data(inet_ntoa(mAddr));
}

