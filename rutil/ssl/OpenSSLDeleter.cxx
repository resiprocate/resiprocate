#if defined(HAVE_CONFIG_H)
  #include "config.h"
#endif

#ifdef USE_SSL

#include "rutil/ssl/OpenSSLDeleter.hxx"

#include <openssl/evp.h>

namespace resip
{

void OpenSSLDeleter::operator()(EVP_MD_CTX* ctx) const noexcept
{
   EVP_MD_CTX_free(ctx);
}

} // namespace resip

#endif
