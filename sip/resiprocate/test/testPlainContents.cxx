#include "resiprocate/PlainContents.hxx"
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      const Data txt("some plain text");

      HeaderFieldValue hfv(txt.data(), txt.size());
      Mime type("text", "plain");
      PlainContents pc(&hfv, type);

      cerr << pc.text() << endl;

      assert(pc.text() == "some plain text");
   }

   cerr << "All OK" << endl;
}

