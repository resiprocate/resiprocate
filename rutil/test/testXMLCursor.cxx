#include "rutil/XMLCursor.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Logger.hxx"

#include <iostream>

#include <cassert>

using namespace resip;
using namespace std;

/*
  <foo/>
  <foo></foo>
  <foo>some contents</foo>
  <foo><echild/><child>first</child><child>second</child></foo>
  <foo bar="baz" qwerty = "quux"/>
  <foo bar="baz" qwerty = "quux">contents</foo>
 */

void traverse(XMLCursor& c)
{
   if (c.firstChild())
   {
      traverse(c);
      c.parent();
   }

   // process(c);
   std::cerr << c.getTag() << std::endl;
   
   if (c.nextSibling())
   {
      traverse(c);
   }
}

int
main()
{
   Log::initialize(Log::Cout, Log::Stack, "testXMLCursor");

   {
       // extremely simple doc with leading whitespace and no prolog
       const Data test("\r\n\r\n<reginfo><test>12</test></reginfo>\r\n");

       try
       {
           XMLCursor xmlc(ParseBuffer(test.data(), test.size()));

           assert(xmlc.getTag() == "reginfo");
           traverse(xmlc);
       }
       catch (ParseException& e)
       {
           cerr << e << endl;
           assert(false);
       }
   }

   {
      const Data test(
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
         "          xmlns:ep=\"urn:ietf:params:xml:ns:pidf:rpid:rpid-person\"\r\n"
         "          xmlns:pp=\"urn:ietf:params:xml:ns:pidf:person\"\r\n"
         "          entity=\"sip:chris@xxx.xx.xxx.xx\">\r\n"
         "  <pp:person>\r\n"
         "   <status>\r\n"
         "    <ep:activities>\r\n"
         "     <ep:activity>away</ep:activity>\r\n"
         "    </ep:activities>\r\n"
         "   </status>\r\n"
         "  </pp:person>\r\n"
         "  <tuple id=\"9b6yhF2Gk37o4\" >\r\n"
         "     <status><basic>open</basic></status>\r\n"
         "  </tuple>\r\n"
         "</presence>");

      try
      {
         XMLCursor xmlc(ParseBuffer(test.data(), test.size()));

         assert(xmlc.getTag() == "presence");
         traverse(xmlc);
      }
      catch (ParseException& e)
      {
         cerr << e << endl;
         assert(false);
      }
   }

   // Tests for XML comment handling
   {
      const Data test(
         "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
         "<!--Comment1-->\r\n"
         "<presence xmlns=\"urn:ietf:params:xml:ns:pidf\"\r\n"
         "          xmlns:ep=\"urn:ietf:params:xml:ns:pidf:rpid:rpid-person\"\r\n"
         "          xmlns:pp=\"urn:ietf:params:xml:ns:pidf:person\"\r\n"
         "          entity=\"sip:chris@xxx.xx.xxx.xx\">\r\n"
         "<!--Comment2-->\r\n"
         "  <pp:person>\r\n"
         "   <status>\r\n"
         "   <!--Comment3-->\r\n"
         "    <ep:activities>\r\n"
         "     <ep:activity>away</ep:activity>\r\n"
         "    </ep:activities>\r\n"
         "   </status>\r\n"
         "  </pp:person>\r\n"
         "  <tuple id=\"9b6yhF2Gk37o4\" >\r\n"
         "     <status><basic>open</basic></status>\r\n"
         "<!--Comment4-->\r\n"
         "  </tuple>\r\n"
         "<!--Comment5-->\r\n"
         "</presence>"
         "<!--Comment6-->\r\n");

      try
      {
         XMLCursor xmlc(ParseBuffer(test.data(), test.size()));

         assert(xmlc.getTag() == "presence");
         traverse(xmlc);
      }
      catch (ParseException& e)
      {
         cerr << e << endl;
         assert(false);
      }
   }

   // test assume that whitespace is not significant
   //   may eventually be controlled by the document/element
   // see http://www.w3.org/TR/1998/REC-xml-19980210#sec-white-space
   assert(!XMLCursor::WhitespaceSignificant);

   {
      cerr << "test attributes in self-terminating tag" << endl;
      Data contents("<?xml version=\"1.0\"?><root><foo attr=\"true\"/></root>");
      try
      {      
         XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));

         assert(xmlc.getTag() == "root");
         assert(xmlc.getValue().empty());
         assert(xmlc.getAttributes().empty());
         assert(xmlc.atRoot());
         assert(!xmlc.atLeaf());
         assert(!xmlc.parent());
         assert(xmlc.firstChild());
         assert(xmlc.getValue().empty());
         assert(!xmlc.getAttributes().empty());
         assert(!xmlc.nextSibling());
      }
      catch (ParseException& e)
      {
         cerr << e << endl;
         assert(false);
      }
   }

   {
      cerr << "test empty root" << endl;
      Data contents("<?xml version=\"1.0\"?><foo/>");
      try
      {      
         XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));

         assert(xmlc.getTag() == "foo");
         assert(xmlc.getValue().empty());
         assert(xmlc.getAttributes().empty());
         assert(xmlc.atRoot());
         assert(!xmlc.atLeaf());
         assert(!xmlc.parent());
         assert(!xmlc.firstChild());
         xmlc.reset();
         assert(!xmlc.nextSibling());
         assert(false);
      }
      catch (ParseException& e)
      {
      }
   }

   {
      cerr << "test childless root" << endl;
      Data contents("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                    "<foo>       </foo>");
      XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));
      
      assert(xmlc.getTag() == "foo");
      cerr << "value: |" << xmlc.getValue() << "|" << endl;
      assert(xmlc.getValue().empty());
      assert(xmlc.getAttributes().empty());
      assert(xmlc.atRoot());
      assert(!xmlc.atLeaf());
      assert(!xmlc.parent());
      assert(!xmlc.firstChild());
      xmlc.reset();
      assert(!xmlc.nextSibling());
   }

   {
      cerr << "test childless root with attributes" << endl;
      Data contents("<?xml version=\"1.0\"?><foo bar=\"baz\" attribute  =  'value'  ></foo>");
      XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));
      
      assert(xmlc.getTag() == "foo");
      assert(xmlc.getValue().empty());
      resipCerr << Inserter(xmlc.getAttributes()) << endl;
      assert(xmlc.getAttributes().size() == 2);
      assert(xmlc.getAttributes().find("bar") != xmlc.getAttributes().end());
      assert(xmlc.atRoot());
      assert(!xmlc.atLeaf());
      assert(!xmlc.parent());
      assert(!xmlc.firstChild());
      xmlc.reset();
      assert(!xmlc.nextSibling());
   }

   {
      cerr << "test empty child" << endl;
      Data contents("<?xml version=\"1.0\"?><foo><child/></foo>");
      XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));
      
      assert(xmlc.getTag() == "foo");
      assert(xmlc.getValue().empty());
      assert(xmlc.getAttributes().size() == 0);
      assert(xmlc.atRoot());
      assert(!xmlc.atLeaf());
      assert(!xmlc.parent());

      assert(xmlc.firstChild());
      assert(xmlc.getTag() == "child");
      assert(!xmlc.firstChild());
      assert(!xmlc.nextSibling());
      assert(xmlc.parent());
      assert(xmlc.getTag() == "foo");
      assert(xmlc.firstChild());

      xmlc.reset();
      assert(xmlc.getTag() == "foo");
      assert(!xmlc.nextSibling());
   }

   {
      cerr << "test contentless child" << endl;
      Data contents("<?xml version=\"1.0\"?>    <foo><child></child></foo>");
      XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));
      
      assert(xmlc.getTag() == "foo");
      assert(xmlc.getValue().empty());
      assert(xmlc.getAttributes().size() == 0);
      assert(xmlc.atRoot());
      assert(!xmlc.atLeaf());
      assert(!xmlc.parent());

      assert(xmlc.firstChild());
      assert(xmlc.getTag() == "child");
      assert(!xmlc.firstChild());
      assert(!xmlc.nextSibling());
      assert(xmlc.parent());
      assert(xmlc.getTag() == "foo");
      assert(xmlc.firstChild());
      assert(xmlc.parent());
      assert(xmlc.getTag() == "foo");
      assert(xmlc.firstChild());

      xmlc.reset();
      assert(xmlc.getTag() == "foo");
      assert(!xmlc.nextSibling());
   }

   {
      cerr << "test tree" << endl;
      Data contents("<?xml version=\"1.0\"?>  \n"
                    "<root>\n"
                    "  <P1>\n"
                    "    <A1></A1>\n"
                    "    <A2></A2>\n"
                    "  </P1>\n"
                    "  <P2>\n"
                    "    <B1></B1>\n"
                    "    <B2></B2>\n"
                    "  </P2>\n"
                    " </root> ");

      XMLCursor tree(ParseBuffer(contents.data(), contents.size()));

      assert(tree.atRoot());
      assert(tree.firstChild());
      assert(tree.getTag() == "P1");
      assert(tree.firstChild());
      assert(tree.getTag() == "A1");

      assert(tree.parent());
      assert(tree.getTag() == "P1");
      assert(tree.firstChild());

      assert(tree.nextSibling());
      assert(tree.getTag() == "A2");
      assert(tree.parent());
      assert(!tree.atRoot());
      assert(tree.nextSibling());
      assert(tree.getTag() == "P2");
      assert(tree.firstChild());
      assert(tree.getTag() == "B1");
      assert(tree.nextSibling());
      assert(tree.getTag() == "B2");
      assert(!tree.nextSibling());
      assert(tree.parent());
      assert(!tree.nextSibling());
   }

   {
      cerr << "test leafy tree" << endl;
      Data contents("<?xml version=\"1.0\"?>  <root>Pie<P1 attribute = \"value\">I<A1>wish</A1>I<A2>could</A2>recollect</P1>pi"
                    "<P2>eureka<B1>cried</B1>the<B2>great</B2>professor</P2>christmas</root> ");
      XMLCursor tree(ParseBuffer(contents.data(), contents.size()));

      assert(tree.firstChild());
      assert(tree.atLeaf());
      assert(tree.getTag().empty());
      assert(tree.getValue() == "Pie");
      assert(tree.nextSibling());
      assert(tree.getTag() == "P1");
      assert(tree.getAttributes().size() == 1);
      assert(tree.nextSibling());
      assert(tree.getTag().empty());
      assert(tree.getValue() == "pi");
      assert(tree.nextSibling());
      assert(tree.getTag() == "P2");
      assert(tree.nextSibling());
      assert(tree.getTag().empty());
      assert(tree.getValue() == "christmas");
      assert(!tree.nextSibling());
   }

   {
      cerr << "test CRLF canonicalization" << endl;

      Data contents("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
                    "<list xmlns=\"urn:ietf:params:xml:ns:rmli\"\r\n"
                    "      uri=\"sip:adam-friends@pres.example.com\" version=\"2\"\r\n"
                    "      name=\"Buddy List at COM\" fullState=\"false\">\r\n"
                    "  <resource uri=\"sip:ed@example.net\" name=\"Ed at NET\">\r\n"
                    "    <instance id=\"sdlkmeopdf\" state=\"pending\"/>\r\n"
                    "  </resource>\r\n"
                    "  <resource uri=\"sip:adam-friends@example.org\"\r\n"
                    "            name=\"My Friends at ORG\">\r\n"
                    "    <instance id=\"cmpqweitlp\" state=\"active\"\r\n"
                    "              cid=\"1KQhyE@pres.example.com\"/>\r\n"
                    "  </resource>\r\n"
                    "</list>");

      XMLCursor tree(ParseBuffer(contents.data(), contents.size()));

      cerr << "root tag = |" << tree.getTag() << "|" << endl;
      assert(tree.getTag() == "list");
      tree.firstChild();
      do
      {
         cerr << tree.getTag() << endl;
      }
      while (tree.nextSibling());


      assert(tree.getTag() == "resource");
   }

   cerr << "All OK" << endl;
      
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
