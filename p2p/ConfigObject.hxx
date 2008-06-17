#ifndef __P2P_CONFIG_OBJECT_HXX
#define __P2P_CONFIG_OBJECT_HXX 1

#include <openssl/ssl.h>

namespace p2p
{


class ConfigObject
{
   public:
      const X509        *getCertificate();
      const EVP_PKEY    *getPrivateKey();



};
}


#endif

