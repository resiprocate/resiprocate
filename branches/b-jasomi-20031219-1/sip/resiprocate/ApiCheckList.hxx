#if !defined(RESIP_APICHECKLIST_HXX)
#define RESIP_APICHECKLIST_HXX


#include "resiprocate/ApiCheck.hxx"

// These types were chosen because they represent the exported stuff.


#include "resiprocate/Connection.hxx"
#include "resiprocate/DnsResult.hxx"
#include "resiprocate/Headers.hxx"
#include "resiprocate/MsgHeaderScanner.hxx"
#include "resiprocate/TlsConnection.hxx"
#include "resiprocate/TransportSelector.hxx"
#include "resiprocate/UdpTransport.hxx"
#include "resiprocate/os/Tuple.hxx"

// Make an entry in the table
#define TENT(x,y) { #x, sizeof(resip::x), y }

static ::resip::ApiCheck::ApiEntry anonymous_resipApiSizeList[] =
{
    // KEEP SORTED ALPHABETICALLY
    TENT(Connection,"NEW_MSG_HEADER_SCANNER"),
    TENT(DnsResult,"USE_IPV6"),
    TENT(Headers,"PARTIAL_TEMPLATE_SPECIALIZATION"),
#if defined(NEW_MSG_HEADER_SCANNER)
    TENT(MsgHeaderScanner,"NEW_MSG_HEADER_SCANNER"),
#endif
    TENT(SipMessage, "PARTIAL_TEMPLATE_SPECIALIZATION"),
    TENT(TlsConnection,"USE_SSL"),
    TENT(TransportSelector,"USE_IPV6"),
    TENT(Tuple,"USE_IPV6"),
    TENT(UdpTransport,"NEW_MSG_HEADER_SCANNER")
};

static resip::ApiCheck
 anonymous_resipApiCheckObj(anonymous_resipApiSizeList,
                            sizeof(anonymous_resipApiSizeList)/sizeof(*anonymous_resipApiSizeList)
     );

#endif
