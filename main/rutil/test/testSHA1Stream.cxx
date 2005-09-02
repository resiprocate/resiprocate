#include <assert.h>

#include "rutil/SHA1Stream.hxx"

using namespace resip;
using namespace std;


int
main(void)
{
#ifdef USE_SSL

   {
      SHA1Stream str;
      assert(str.getHex() == "da39a3ee5e6b4b0d3255bfef95601890afd80709");
   }

   {
      SHA1Stream str;
      str << "sip:alice@atlanta.example.com"
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
           "a=rtpmap:0 PCMU/8000\r\n";
      assert(str.getHex() == "f2eec616bd43c75fd1cd300691e57301b52b3ad3");
   }

   cerr << "All OK" << endl;
#endif

   return 0;
}
