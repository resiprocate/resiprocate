#include "sip2/sipstack/PlainContents.hxx"
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      const Data txt("Content-Type: text/plain\r\n"
                     "\r\n"
                     "some plain text");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("text", "plain");
      PlainContents pc(&hfv, type);

      cerr << pc.text() << endl;

      assert(pc.text() == "some plain text");
   }

   cerr << "All OK" << endl;
}

