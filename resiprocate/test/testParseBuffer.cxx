#include "resiprocate/os/ParseBuffer.hxx"
#include <string.h>
#include <assert.h>
#include "resiprocate/os/Logger.hxx"

using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

int
main(int argc, char** argv)
{
   Log::initialize(Log::COUT, argc > 1 ? Log::toLevel(argv[1]) :  Log::INFO, argv[0]);

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
         catch (ParseBuffer::Exception& e)
         {
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
         catch (ParseBuffer::Exception& e)
         {
            assert(e.getMessage() == test);
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
      catch (ParseBuffer::Exception& e)
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
         catch (ParseBuffer::Exception& e)
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
         catch (ParseBuffer::Exception& e)
         {
            break;
         }
         assert(0);
      } while (false);
   }
   
   {
      char buf[] = "Here is a \t buffer with some stuff.";

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
      char buf[] = "2890844526";
      ParseBuffer pb(buf, strlen(buf));   
      assert(pb.unsignedInteger() == 2890844526UL);
   }

   {
      char buf[] = "17.71";
      ParseBuffer pb(buf, strlen(buf));   
      float val = pb.floatVal();
      assert(val > 17.70 && val < 17.72);
   }

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

