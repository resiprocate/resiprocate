#if !defined(RESIP_APICHECKLIST_HXX)
#define RESIP_APICHECKLIST_HXX


// These types were chosen because they represent the exported stuff.

#include "resiprocate/SipStack.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/TlsConnection.hxx"
#include "resiprocate/Security.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/Logger.hxx"
#include "resiprocate/TransactionController.hxx"
#include "resiprocate/Executive.hxx"
#include "resiprocate/Message.hxx"
#include "resiprocate/Uri.hxx"


// Make an entry in the table
#define TENT(x) { #x, sizeof(resip::x) }

static ::resip::ApiCheck::ApiEntry resipApiSizeList[] =
{
    TENT(SipStack),
    TENT(DnsResult),
    TENT(Security),
    TENT(Connection),
    TENT(TlsConnection),
    TENT(TransportSelector),
    TENT(DnsInterface),
    TENT(DnsResult),
    TENT(Tuple),
    TENT(TransportType),
    TENT(IpVersion),
    TENT(SipMessage),
    TENT(Uri),
    TENT(TransactionController),
    TENT(Executive),
    TENT(Message),
    {0,0}
};

#endif
