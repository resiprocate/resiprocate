#include "sip2/sipstack/PlainContents.hxx"
#include "sip2/sipstack/HeaderFieldValue.hxx"
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      const Data txt("some plain text\r\n");

      HeaderFieldValue hfv(txt.data(), txt.size());
      PlainContents pc(&hfv);

      assert(pc.text() == "some plain text");
   }

   cerr << "All OK" << endl;
}

