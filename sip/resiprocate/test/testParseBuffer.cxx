#include <util/ParseBuffer.hxx>
#include <string.h>
#include <assert.h>
int
main(int arc, char** argv)
{
   {
      
      char *buf = "Here is a \t buffer with some stuff.";

      Vocal2::ParseBuffer pb(buf, strlen(buf));
   
      assert(!pb.eof());
      assert(pb.position() == buf);

      pb.skipWhiteSpace();
      assert(pb.position() == buf);

      pb.skipNonWhiteSpace();
      assert(*pb.position() == ' ');
   }
   
   {
      char *buf = "    \t buffer with some stuff.";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   

      pb.skipWhiteSpace();
      assert(*pb.position() == 'b');
      
      pb.skipToChar('s');
      assert(*pb.position() == 's');
      pb.skipNonWhiteSpace();
      pb.skipWhiteSpace();
      pb.skipNonWhiteSpace();
      assert(pb.eof());
   }

   {
      char *buf = "jhsjfhskd;|.";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   

      pb.skipToOneOf(".|;");
      assert(*pb.position() == ';');
   }
}

