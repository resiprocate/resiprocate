#include "resiprocate/XMLCursor.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Inserter.hxx"

#include <iostream>

using namespace Vocal2;
using namespace std;

/*
  <foo/>
  <foo></foo>
  <foo>some contents</foo>
  <foo><echild/><child>first</child><child>second</child></foo>
  <foo bar="baz" qwerty = "quux"/>
  <foo bar="baz" qwerty = "quux">contents</foo>
 */
int
main()
{
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
      catch (ParseBuffer::Exception& e)
      {
      }
   }

   {
      cerr << "test childless root" << endl;
      Data contents("<?xml version=\"1.0\"?><foo></foo>");
      XMLCursor xmlc(ParseBuffer(contents.data(), contents.size()));
      
      assert(xmlc.getTag() == "foo");
      cerr << "valeu: |" << xmlc.getValue() << "|" << endl;
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
      cerr << Inserter(xmlc.getAttributes()) << endl;
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
      Data contents("<?xml version=\"1.0\"?><foo><child></child></foo>");
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
      Data contents("<?xml version=\"1.0\"?>  <root><P1><A1></A1><A2></A2></P1><P2><B1></B1><B2></B2></P2></root> ");

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
                    "<P2>eureka<B1>cried</B1>the<B2>great</B2>professor</P2>christmans</root> ");
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
      assert(tree.getValue() == "christmans");
      assert(!tree.nextSibling());
   }      

   cerr << "All OK" << endl;
      
   return 0;
}
