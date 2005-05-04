#include <resiprocate/MacSecurity.hxx>

#include <sys/types.h>
#include <openssl/e_os2.h>
#include <openssl/evp.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/pkcs7.h>
#include <openssl/ossl_typ.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#include <iostream>
using namespace std;

namespace resip
{

/*
 * Test class for MacSecurity
 */
class TestMacSecurity : public MacSecurity
{
   public:

      TestMacSecurity() {};

      // returns non zero if tests failed
      static int main()
      {
         TestMacSecurity test;

         // at first # of root certs should be 0
         // assert(# of root certs == 0);
         
         // load the root certs
         test.preload();
         
         // now the # of root certs should be > 0
         // assert(# of root certs > 0);
         
         return 0;
      }
};
} // namespace resip


int main(int argc, char* argv[])
{
   return resip::TestMacSecurity::main();
}