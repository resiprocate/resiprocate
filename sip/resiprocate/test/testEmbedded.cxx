#include "resiprocate/os/DataStream.hxx"

#include "resiprocate/SipMessage.hxx"
#include "resiprocate/Uri.hxx"
#include "resiprocate/Embedded.hxx"
#include "resiprocate/ParserCategories.hxx"
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

using namespace std;
using namespace resip;

int
main(int argc, char** argv)
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::INFO, argv[0]);

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
      cerr << "Produce embedded, single header" << endl;
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
      cerr << "Produce embedded, multiple headers" << endl;
      Uri foo;
      foo.user() = "speedy";
      foo.host() = "cathaynetworks.com";
      foo.embedded().header(h_CSeq).method() = ACK;
      foo.embedded().header(h_CSeq).sequence() = 4178;

      Via via;
      BranchParameter branch = via.param(p_branch);

      branch.reset("fobbieBletch");
      via.transport() = "TLS";
      via.sentHost() = "cathay.com";
      via.sentPort() = 5066;
      via.param(p_branch) = branch;
      foo.embedded().header(h_Vias).push_back(via);

      branch.reset("bletchieFoo");
      via.transport() = "TCP";
      via.sentHost() = "ixolib.com";
      via.sentPort() = 5067;
      via.param(p_branch) = branch;
      foo.embedded().header(h_Vias).push_back(via);

      Data buf;
      {
         DataStream str(buf);
         foo.encode(str);
      }
      cerr << buf << endl;
      assert(buf == "sip:speedy@cathaynetworks.com?Via=SIP/2.0/TLS%20cathay.com:5066%3Bbranch%3Dz9hG4bK-c87542-fobbieBletch-1-c87542-&Via=SIP/2.0/TCP%20ixolib.com:5067%3Bbranch%3Dz9hG4bK-c87542-bletchieFoo-1-c87542-&CSeq=4178%20ACK");
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
      assert(na.uri().param(p_ttl) == 134);
   }

   {
      cerr << "Parse Uri with embedded followed by NameAddr parameter" << endl;
      
      Data nad("sips:bob@foo.com;ttl=134?CSeq=314159%20ACK;tag=17");
      NameAddr na(nad);

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_CSeq));
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 314159);
      assert(na.uri().param(p_ttl) == 134);
      assert(na.param(p_tag) == "17");
   }

   {
      cerr << "Parse Uri with multiple headers" << endl;
      
      Data nad("sip:speedy@cathaynetworks.com?Via=SIP/2.0/TLS%20cathay.com:5066%3Bbranch%3Dz9hG4bK-c87542-fobbieBletch-c87542-1&Via=SIP/2.0/TCP%20ixolib.com:5067%3Bbranch%3Dz9hG4bK-c87542-bletchieFoo-c87542-1&CSeq=4178%20ACK");
      NameAddr na(nad);

      assert(na.uri().hasEmbedded());
      assert(na.uri().embedded().exists(h_Vias));
      assert(na.uri().embedded().exists(h_CSeq));
      
      assert(na.uri().embedded().header(h_Vias).size() == 2);
      assert(na.uri().embedded().header(h_Vias).front().transport() == "TLS");
      assert((++(na.uri().embedded().header(h_Vias).begin()))->transport() == "TCP");
      assert(na.uri().embedded().header(h_CSeq).method() == ACK);
      assert(na.uri().embedded().header(h_CSeq).sequence() == 4178);
   }

   cerr << endl << "Tests OK" << endl;
}
