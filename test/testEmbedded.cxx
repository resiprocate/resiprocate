#include "sip2/util/DataStream.hxx"

#include "sip2/sipstack/SipMessage.hxx"
#include "sip2/sipstack/Uri.hxx"
#include "sip2/sipstack/Embedded.hxx"
#include "sip2/sipstack/ParserCategories.hxx"

using namespace std;
using namespace Vocal2;

int
main()
{
   {
      Data foo("abcdefghi1232454435");
      assert(Embedded::encode(foo) == foo);
   }

   {
      Data foo("abcdefghi1232454435^ * ");
      cerr << Embedded::encode(foo) << endl;
      assert(Embedded::encode(foo) == "abcdefghi1232454435^%20*%20");
   }

   {
      Data foo("abcdefghi1232454435^&*&");
      unsigned int c;
      char* res = Embedded::decode(foo, c);

      cerr << Data(res, c) << endl;
      assert(foo == Data(res, c));
      delete [] res;
   }

   {
      Data foo("!@#$%^&*() ?:=17qwerty");
      Data bar = Embedded::encode(foo);

      cerr << bar << endl;

      unsigned int c;
      char* res = Embedded::decode(bar, c);
      cerr << Data(res, c) << endl;
      assert(foo == Data(res, c));

      delete [] res;
   }

   {
      Data foo("!@#$%^&*() ?:=17qwerty");
      Data bar(Embedded::encode(Embedded::encode(foo)));

      cerr << bar << endl;

      unsigned int c;
      char* res1 = Embedded::decode(bar, c);
      Data rab(res1, c);
      char* res2 = Embedded::decode(rab, c);

      assert(foo == Data(res2, c));

      delete [] res1;
      delete [] res2;
   }

   {
      Uri foo;
      foo.user() = "speedy";
      foo.host() = "cathaynetworks.com";
      foo.embedded().header(h_CSeq).method() = ACK;
      foo.embedded().header(h_CSeq).sequence() = 4178;

      Data buf;
      {
         DataStream str(buf);
         foo.encode(str);
      }
      cerr << endl << buf << endl;
      assert(buf == "sip:speedy@cathaynetworks.com?CSeq=4178%20ACK");
   }

   {
      cerr << "Parse <Uri> with embedded" << endl;
      
      Data nad("bob<sips:bob@foo.com?CSeq=314159%20ACK>;tag=wd834f");
      NameAddr na(nad); 

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_CSeq));
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 314159);
   }

   {
      cerr << "Parse Uri with embedded" << endl;
      
      Data nad("sips:bob@foo.com;ttl=134?CSeq=314159%20ACK");
      NameAddr na(nad); 

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_CSeq));
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 314159);
      assert(na.uri().param(p_ttl) = 134);
   }

   cerr << endl << "Tests OK" << endl;
}
