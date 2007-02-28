#ifndef DTLS_CreateCert_hxx
#define DTLS_CreateCert_hxx

#include <openssl/crypto.h>
#include <openssl/ssl.h>

namespace resip
{
class Data;
}

namespace dtls
{

int createCert (const resip::Data& pAor, int expireDays, int keyLen, X509&* outCert, EVP_PKEY&* outKey );

}

#endif
