#include <util/Data.hxx>
#include "assert.h"
#include <string>
#include <iostream>

using namespace Vocal2;
using namespace std;

int
main()
{
   {
      assert(Data(0) == "0");
      assert(Data(1) == "1");
      assert(Data(-1) == "-1");
      assert(Data(11) == "11");      
      assert(Data(1234567) == "1234567");
      assert(Data(-1234567) == "-1234567");
   }
      
   {
      Data empt;
      Data empt1;
      assert(empt.size() == 0);
      assert(empt == empt);
      assert(empt == empt1);
      assert(empt1 == empt);
      assert(empt1 == "");

      assert(!(empt != empt));
      assert(!(empt != empt1));
      assert(!(empt1 != empt));
      assert(!(empt1 != ""));
      
      assert(empt1 == "");
      assert("sdf" != empt1);
      assert(Data("SAfdsaf") != empt1);
      empt = empt;
      empt = empt1;
      empt = "sdfasf";
   }

   {
      Data d("qwerty");
      cerr << d << endl;
      assert(strcmp(d.data(), "qwerty") == 0);

      Data e;
      assert(strcmp(e.data(), "") == 0);
      assert(strcmp(e.data(), "") == 0);
      assert(strcmp(e.data(), "") == 0);
   }

   {
      Data d("qwerty");
      assert(strcmp(d.c_str(), "qwerty") == 0);

      Data e;
      assert(strcmp(e.c_str(), "") == 0);
      assert(strcmp(e.c_str(), "") == 0);
      assert(strcmp(e.c_str(), "") == 0);
   }

   {
      Data c = "sadfsdf";
      Data d;
      d = c;
      assert(c == d);
   }

   {
      const char* f = "asdasd";
      Data d = f;
      assert(d == f);
   }

   {
      Data d;
      const char* f = "asdasd";
      d = f;
      assert(d == f);
   }

   {
      Data d("asdfasfdsadf");
      const char* f = "asdasd";
      d = f;
      assert(d == f);
   }

   {
      string f("asdasd");
      Data d = f;
      assert(d == f);
   }
   {
      string f("asdasd");
      Data d = f;
      assert(d.c_str() == f);
   }
   {
      Data d("123");
      assert(d.size() == 3);
   }

   {
      Data d("one");
      Data c("two");
      d += c;
      assert(d == "onetwo");

      Data empt;
      assert(empt + d == d);
      assert(empt + d == "onetwo");
      assert(empt + "three" == "three");
   }

   {
      Data s;
      s = "c=";
      assert(s == "c=");
      s += "foo";
      assert(s == "c=foo");
      s += "\r\n";
      s += "bar";
      s += "\r\n";
      assert (s == "c=foo\r\nbar\r\n");
   }
   {
      Data a("one");
      Data b("two");
      Data c("three");
      
      assert(a+b+c == "onetwothree");
   }
   
   {
      Data d("one");
      cerr << "one + two = " << (d + "two") << endl;
      assert((d + "two") == "onetwo");
   }

}
