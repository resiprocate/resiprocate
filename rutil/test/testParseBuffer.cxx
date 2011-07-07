#include "rutil/ParseBuffer.hxx"
#include <string.h>
#include <assert.h>
#include "rutil/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char** argv)
{
   Log::initialize(Log::Cout, argc > 1 ? Log::toLevel(argv[1]) :  Log::Info, argv[0]);
   
   {
     const char buf[] = "/home/jason/test";
     ParseBuffer pb(buf);
     pb.skipToEnd();
     pb.skipBackToChar('/');
     assert(!pb.bof());
     Data result(pb.position());
     assert(result == "test");
   }

   {
     const char buf[] = "test";
     ParseBuffer pb(buf);
     pb.skipToEnd();
     pb.skipBackToChar('/');
     assert(pb.bof());
     Data result(pb.position());
     assert(result == "test");
   }

   {
     const char buf[] = "/test";
     ParseBuffer pb(buf);
     pb.skipToEnd();
     pb.skipBackToChar('/');
     Data result(pb.position());
     assert(result == "test");
   }

   {
      const char buf[] = "!^.*$!sip:user@example.com!";
      ParseBuffer pb(buf, strlen(buf));

      const char delim = buf[0];
      const char* start = pb.skipChar(delim);
      std::cerr << "start=" << start << std::endl;
      pb.skipToChar(delim);

      Data e1;
      pb.data(e1, start);
      std::cerr << "e1=" << e1 << std::endl;
      assert(e1 == "^.*$");
       
      start = pb.skipChar(delim);
      Data e2;
      pb.skipToChar(delim);
      pb.data(e2, start);
      std::cerr << "e2=" << e2 << std::endl;
      assert(e2 == "sip:user@example.com");
      
      start = pb.skipChar(delim);
      Data e3;
      pb.data(e3, start);
      std::cerr << "e3=" << e3 << std::endl;
      assert(e3.empty());
   }
   
   {
      const char buf[] = "Ducky%20%26";
      ParseBuffer pb(buf, strlen(buf));

      const char* start = pb.skipWhitespace();
      pb.skipToEnd();
      
      Data target;
      pb.dataUnescaped(target, start);
      
      assert(target == "Ducky &");
   }
   
   {
      const char buf[] = "  \r\t\r\n\t  !";
      ParseBuffer pb(buf, strlen(buf));
      
      pb.skipWhitespace();
      assert(*pb.position() == '!');
   }

   {
      const char buf[] = "asdfa1234123edfdf213ref@!\t  \r\t\r\n\t ";
      ParseBuffer pb(buf, strlen(buf));
      
      pb.skipNonWhitespace();
      assert(*pb.position() == '\t');
   }

   {
      std::cerr << "!! Test position" << std::endl;
      char buf[] = "Here is a buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));
      
      pb.skipToChars("buff");
      *pb.position();

      pb.skipToEnd();

      do
      {
         try
         {
            *pb.position();
            assert(false);
         }
         catch (BaseException& e)
         {
            break;
         }
      } while (false);
   }

   {
      std::cerr << "!! Test fail one line" << std::endl;
      
      char buf[] = "Here is a \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));

      do
      {
         try
         {
            pb.skipChars("Here isn't a");
         }
         catch (ParseException& e)
         {
             //std::cerr<< e << std::endl;
             break;
         }
         assert(0);
      } while (false);
   }

   {
      std::cerr << "!! Test fail multiline" << std::endl;
      
      const Data test("Test input");
      char buf[] = "Here is a \r\n buffer with \r\nsome stuff.";
      ParseBuffer pb(buf, strlen(buf), test);

      do
      {
         try
         {
            pb.skipToChars("buff");
            pb.skipChars("buff");
            pb.skipChar('g');
         }
         catch (ParseException& e)
         {
             //using namespace std;
             //cerr << e << endl;
             //cerr << '\'' << e.getMessage() << '\'' << endl;
             //cerr << '\'' << test << '\'' << endl;
            break;
         }
         assert(0);
      } while (false);
   }

   {
      char buf[] = "Content-Languages: English, \r\n French  , \r\n\t LISP   \r \n \n\r \r\n\r\n";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToTermCRLF();
      pb.skipChars("\r\n");
      pb.skipChars("\r\n");
      pb.assertEof();
   }

   {
      char buf[] = "Content-Languages: English, \r\n French  , \r\n\t LISP   \r \n \n\r \r\n\r\n";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars("French");
      pb.skipN(strlen("French"));
      pb.skipWhitespace();
      pb.skipChar(',');
      pb.skipLWS();
      std::cerr << pb.position();
      pb.skipChars("LISP");
   }

   {
      char buf[] = "123456789";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipN(9);
      assert(pb.eof());
   }

   {
      char buf[] = "123456789";
      ParseBuffer pb(buf, strlen(buf));
      try
      {
         char foo = *pb.skipN(9);
         (void)foo;
         assert(0);
      }
      catch (ParseException& e)
      {}
   }

   {
      char buf[] = "Here is a \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars("some");
      std::cerr << pb.position() << std::endl;
      pb.skipChars("some stu");
   }

   {
      char buf[] = "Here is asom \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars("some");
      pb.skipChars("some stuf");
   }

   {
      char buf[] = "Here is asom \t buffer with som stuff.";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars("some");
      pb.assertEof();
   }

   {
      char buf[] = "Here is a \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars(Data("some"));
      pb.skipChars("some stuf");
   }

   {
      char buf[] = "Here is asom \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars(Data("some"));
      pb.skipChars("some stuf");
   }

   {
      char buf[] = "Here is asom \t buffer with som stuff.";
      ParseBuffer pb(buf, strlen(buf));
      pb.skipToChars(Data("some"));
      pb.assertEof();
   }

   {
      char buf[] = "Here is a \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));

      pb.skipChars("Here is a");
   }

   {
      char buf[] = "Here is a \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));

      do
      {
         try
         {
            pb.skipChars("Here isn't a");
         }
         catch (ParseException& e)
         {
            break;
         }
         assert(0);
      } while (false);
   }

   {
      char buf[] = "Here is a buf.";
      ParseBuffer pb(buf, strlen(buf));

      do
      {
         try
         {
            pb.skipChars("Here is a ");
            pb.skipChars("buffer");
         }
         catch (ParseException& e)
         {
            break;
         }
         assert(0);
      } while (false);
   }
   
   {
      const char* buf = "Here is a \t buffer with some stuff.";

      ParseBuffer pb(buf, strlen(buf));
   
      assert(!pb.eof());
      assert(pb.position() == buf);

      pb.skipWhitespace();
      assert(pb.position() == buf);

      pb.skipNonWhitespace();
      assert(*pb.position() == ' ');
   }

   {
      char buf[] = "    \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));   

      pb.reset(pb.end());
      pb.skipBackToChar('s');
      pb.skipBackChar();
      pb.skipBackToChar('s');
      pb.skipBackChar('s');

      assert(Data(pb.position(), 4) == "some");
   }

   {
      char buf[] = "buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));   

      pb.reset(pb.end());
      pb.skipBackToChar('q');
      assert(pb.bof());
      pb.skipChar('b');
   }
   
   {
      char buf[] = "    \t buffer with some stuff.";
      ParseBuffer pb(buf, strlen(buf));   

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
      char buf[] = "jhsj:!hskd;|.";
      ParseBuffer pb(buf, strlen(buf));   

      pb.skipToOneOf(":@");
      assert(*pb.position() == ':');
   }

   {
      char buf[] = "user@host:port";
      ParseBuffer pb(buf, strlen(buf));   

      pb.skipToOneOf(":@");
      assert(*pb.position() == '@');
   }

   {
      char buf[] = "jhsjfhskd;|.";
      ParseBuffer pb(buf, strlen(buf));   

      pb.skipToOneOf(".|;");
      assert(*pb.position() == ';');
   }

   {
      char buf[] = "\"  \\\"Q \t buffer with some stuff.\"Z";
      ParseBuffer pb(buf, strlen(buf));   
      pb.skipWhitespace();
      pb.skipToChar('"');
      pb.skipChar();
      pb.skipToEndQuote();
      assert(*pb.position() == '"');
      pb.skipChar();
      assert(*pb.position() == 'Z');
   }
   
   {
      char buf[] = "17 ";
      ParseBuffer pb(buf, strlen(buf));   
      assert(pb.integer() == 17);
   }
   
   {
      char buf[] = "-17";
      ParseBuffer pb(buf, strlen(buf));   
      assert(pb.integer() == -17);
   }

   {
      char buf[] = "999999999999999999999999999 ";
      ParseBuffer pb(buf, strlen(buf));
      try
      {
         pb.integer();
         assert(0);
      }
      catch(ParseException& e)
      {}
   }
   
   {
      char buf[] = "-999999999999999999999999999 ";
      ParseBuffer pb(buf, strlen(buf));
      try
      {
         pb.integer();
         assert(0);
      }
      catch(ParseException& e)
      {}
   }

#ifndef WIN32
   {
      char buf[] = "2890844526";
      ParseBuffer pb(buf, strlen(buf));   
      assert(pb.uInt64() == 2890844526UL);
   }
#endif

#ifndef RESIP_FIXED_POINT
   {
      char buf[] = "17.71";
      ParseBuffer pb(buf, strlen(buf));   
      float val = pb.floatVal();
      assert(val > 17.70 && val < 17.72);
   }
#endif

   {
      char buf[] = "token another token";
      ParseBuffer pb(buf, strlen(buf));   
      const char *start = pb.position();
      pb.skipToChar(' ');
      pb.skipChar(' ');
      Data t;
      // make t share memry with buf
      pb.data(t, start);
      assert(t.data() == buf);
      // assign copies
      Data t1 = t;
      assert(t1.data() != buf);
      // assign copies
      t = t1;
      assert(t.data() != buf);

      start = pb.position();
      pb.skipToChar(' ');
      pb.skipChar(' ');
      Data t2;
      pb.data(t2, start);
      // should survive scope exit
   }

   std::cerr << "All OK" << std::endl;
   return 0;
}

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
