#if !defined(RESIP_OPENSSLDELETER_HXX)
#define RESIP_OPENSSLDELETER_HXX

#include <openssl/ossl_typ.h>

namespace resip
{

struct OpenSSLDeleter {
   void operator()(EVP_CIPHER_CTX* ctx) const noexcept;
   void operator()(EVP_MD_CTX* ctx) const noexcept;
};

}

#endif
