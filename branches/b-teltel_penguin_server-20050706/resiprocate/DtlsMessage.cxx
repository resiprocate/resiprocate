#include "resiprocate/DtlsMessage.hxx"

#ifdef USE_DTLS

using namespace resip ;

DtlsMessage::DtlsMessage( SSL *ssl )
    : mSSL( ssl )
{
}

#endif /* USE_DTLS */
