#include <sipstack/ParserCategories.hxx>
#include <sipstack/HeaderFieldValue.hxx>
#include <sipstack/HeaderTypes.hxx>
#include <assert.h>
#include <string.h>
#include <iostream>

using namespace std;
using namespace Vocal2;

int
main(int arc, char** argv)
{
   {
      // test header hash
      for (int i = Headers::CSeq; i < Headers::UNKNOWN; i++)
      {
         assert(Headers::getType(Headers::HeaderNames[i].c_str(), Headers::HeaderNames[i].size()) == i);
      }
   }

   {
      // test parameter hash
      for (int i = ParameterTypes::transport; i < ParameterTypes::UNKNOWN; i++)
      {
         assert(ParameterTypes::getType(ParameterTypes::ParameterNames[i].c_str(), ParameterTypes::ParameterNames[i].size()) == i);
      }
   }
   
   {
      char *org = "WuggaWuggaFoo";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv);
      assert(tok.value() == org);
   }

   {
      char *org = "WuggaWuggaFoo;ttl=2";
      
      HeaderFieldValue hfv(org, strlen(org));
      Token tok(&hfv);
      cerr << tok.value() << endl;
      assert(tok.value() == "WuggaWuggaFoo");
      cerr << tok.param(p_ttl) << endl;
      assert(tok.param(p_ttl) == 2);
      cerr << tok;
   }

   {
      char *viaString = /* Via: */ " SIP/2.0/UDP a.b.c.com:5000;ttl=3;maddr=1.2.3.4;received = foo.com";
      
      HeaderFieldValue hfv(viaString, strlen(viaString));
      Via via(&hfv);
      cerr << via << endl;
      assert(via.sentPort() == 5000);
      assert(via.sentHost() == "a.b.c.com");
      assert(via.param(p_maddr) == "1.2.3.4");
   }
}


