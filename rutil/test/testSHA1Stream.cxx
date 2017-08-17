#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <assert.h>

#include "rutil/Sha1.hxx"

#ifdef USE_SSL
#include "rutil/ssl/SHA1Stream.hxx"
#endif

using namespace resip;
using namespace std;

int
main(void)
{
    // Test Sha1 class that does not use OpenSSL first
   {
      resip::SHA1 sha1test;
      sha1test.update("");
      assert(sha1test.final() == "da39a3ee" "5e6b4b0d" "3255bfef" "95601890" "afd80709");
   }
   {
      resip::SHA1 sha1test;
      sha1test.update("abc");
      assert(sha1test.final() == "a9993e36" "4706816a" "ba3e2571" "7850c26c" "9cd0d89d");
   }
   {
      resip::SHA1 sha1test;
      sha1test.update("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq");
      assert(sha1test.final() == "84983e44" "1c3bd26e" "baae4aa1" "f95129e5" "e54670f1");
   }
   {
      resip::SHA1 sha1test;
      sha1test.update("");
      Data result = sha1test.finalBin();
      assert(result.hex() == "da39a3ee5e6b4b0d3255bfef95601890afd80709");      
   }
   {
      Data input("sip:alice@atlanta.example.com"
                 ":a84b4c76e66710"
                 ":314159 INVITE"
                 ":Thu, 21 Feb 2002 13:02:03 GMT"
                 ":sip:alice@pc33.atlanta.example.com"
                 ":v=0\r\n"
                 "o=UserA 2890844526 2890844526 IN IP4 pc33.atlanta.example.com\r\n"
                 "s=Session SDP\r\n"
                 "c=IN IP4 pc33.atlanta.example.com\r\n"
                 "t=0 0\r\n"
                 "m=audio 49172 RTP/AVP 0\r\n"
                 "a=rtpmap:0 PCMU/8000\r\n");
      {
         
         resip::SHA1 sha1test;
         sha1test.update(input.c_str());

         assert(sha1test.final() == "f2eec616bd43c75fd1cd300691e57301b52b3ad3");
      }
   }

#ifdef USE_SSL
   // Test Sha1Stream class that uses OpenSSL
   {
      SHA1Stream str;
      assert(str.getHex() == "da39a3ee" "5e6b4b0d" "3255bfef" "95601890" "afd80709");
   }
   {
      SHA1Stream str;
      str << "abc";
      assert(str.getHex() == "a9993e36" "4706816a" "ba3e2571" "7850c26c" "9cd0d89d");
   }
   {
      SHA1Stream str;
      str << "abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq";
      assert(str.getHex() == "84983e44" "1c3bd26e" "baae4aa1" "f95129e5" "e54670f1");
   }

   {
      Data input("sip:alice@atlanta.example.com"
                 ":a84b4c76e66710"
                 ":314159 INVITE"
                 ":Thu, 21 Feb 2002 13:02:03 GMT"
                 ":sip:alice@pc33.atlanta.example.com"
                 ":v=0\r\n"
                 "o=UserA 2890844526 2890844526 IN IP4 pc33.atlanta.example.com\r\n"
                 "s=Session SDP\r\n"
                 "c=IN IP4 pc33.atlanta.example.com\r\n"
                 "t=0 0\r\n"
                 "m=audio 49172 RTP/AVP 0\r\n"
                 "a=rtpmap:0 PCMU/8000\r\n");
      {
         
         SHA1Stream str;
         str << input;

         assert(str.getHex() == "f2eec616bd43c75fd1cd300691e57301b52b3ad3");
      }
      {
         SHA1Stream str;
         str << input;
         assert(str.getBin(32).size() == 4);
      }
      {
         SHA1Stream str;
         str << input;
         assert(str.getBin().size() == 20);
      }
      {
         SHA1Stream str;
         str << input;
         assert(str.getBin(128).size() == 16);
      }

      Data a;
      Data b;
      {
         SHA1Stream str;
         str << input;
         a = str.getBin(32);
         cerr << "A: " << a.hex() << endl;
      }
      {
         SHA1Stream str;
         str << input;
         b = str.getBin(128);
         cerr << "B: " << b.hex() << endl;
      }
      
      assert(memcmp(a.c_str(), b.c_str()+12, 4) == 0);
   }


   cerr << "All OK" << endl;
#endif

   return 0;
}
