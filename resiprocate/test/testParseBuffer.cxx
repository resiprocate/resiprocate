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

      pb.skipWhitespace();
      assert(pb.position() == buf);

      pb.skipNonWhitespace();
      assert(*pb.position() == ' ');
   }
   
   {
      char *buf = "    \t buffer with some stuff.";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   

      pb.skipWhitespace();
      assert(*pb.position() == 'b');
      
      pb.skipToChar('s');
      assert(*pb.position() == 's');
      pb.skipNonWhitespace();
      pb.skipWhitespace();
      pb.skipNonWhitespace();
      assert(pb.eof());
   }

   {
      char *buf = "jhsjfhskd;|.";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   

      pb.skipToOneOf(".|;");
      assert(*pb.position() == ';');
   }
   {
      char *buf = "\"  \\\"Q \t buffer with some stuff.\"Z";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   
      pb.skipWhitespace();
      pb.skipToChar('"');
      pb.skipChar();
      pb.skipToEndQuote();
      assert(*pb.position() == '"');
      pb.skipChar();
      assert(*pb.position() == 'Z');
   }
   
   {
      char *buf = "17 ";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   
      assert(pb.integer() == 17);
   }
   
   {
      char *buf = "17";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   
      assert(pb.integer() == 17);
   }

   {
      char *buf = "17.71";
      Vocal2::ParseBuffer pb(buf, strlen(buf));   
      float val = pb.floatVal();
      assert(val > 17.70 && val < 17.72);
   }
}

