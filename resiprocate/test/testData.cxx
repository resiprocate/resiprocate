#include "sip2/util/Data.hxx"
#include "assert.h"
#include <string>
#include <iostream>

using namespace Vocal2;
using namespace std;




// for friends
class TestData
{
   public:
      void main()
      {
         {
            Data *d = new Data("origin",6);
            {
               Data * t = d;

               d = new Data(*d);

               assert(d->size() == t->size());
               assert(d->mCapacity == t->mCapacity);
               // cout << d->size() << ":" << d->mCapacity << endl;
               delete t;
            }
            delete d;
         }

         {
            char* blah = "12345";
            Data d(blah, 3);
            assert(strlen(d.c_str()) == 3);
         }

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
            
            assert(e == "");
         }

         {
            Data d("qwerty");
            assert(strcmp(d.c_str(), "qwerty") == 0);

            Data e;
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
            const char* f = "asdasd";
            Data d = f;
            assert(!(d < f));
         }

         {
            Data d;
            const char* f = "asdasd";
            d = f;
            assert(!(d < f));
         }

         {
            Data d("asdfasfdsadf");
            const char* f = "asdasd";
            d = f;
            assert(!(d < f));
         }

         {
            string f("asdasd");
            Data d = f;
            assert(!(d < f));
         }
         {
            string f("asdasd");
            Data d = f;
            assert(!(d.c_str() < f));
         }
         
         {
            Data a("qwerty");
            Data b("qwerty");
            assert(!(a < b));
         }
         {
            Data a("qwert");
            Data b("qwerty");
            assert(a < b);
         }
         {
            Data a("qwert");
            Data b("qwerty");
            assert(a < b);
            assert(!(b < a));
         }

         {
            const char* f = "asdasda";
            Data d("asdasd");
            assert(d < f);
         }

         {
            const char * c("asdfasfdsadf");
            Data d(c, 4, true); //share memory
            assert(d < c);
         }

         {
            const char * c("asdfasfdsadf");
            Data d(c, strlen(c), true); // share memory
            assert(!(d < c));
         }

         {
            const char * c("asdfasfdsadf");
            Data d(c, strlen(c), true); // share memory
            assert(!(d < c));
            Data c1(d); // copy, null terminate
            assert(!(d < c1));
            assert(!(c1 < d));
         }

         {
            string f("asdasd");
            Data d = f + "!";
            assert(!(d < f));
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
            Data s;
            s = "c=";
            assert(s == "c=");
            s += 'f';
            assert(s == "c=f");
            assert(s.size() == 3);
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
         {
            Data mixed("MiXed");
            mixed.lowercase();
            assert(mixed == "mixed");
         }
         {
            Data mixed("miXed");
            mixed.uppercase();
            assert(mixed == "MIXED");
         }
         {
            Data a("a");
            Data aa(a);
            assert(a.size() == aa.size());
         }
         {
            Data d("ssd");
            Data c;
            d = c;
            assert(d.empty());
         }
         {
            Data d;
            Data c;
            d = c;
            assert(d.empty());
         }
         {
            char* s = "userB@whistler.gloo.net:6062\r\nCo\031";
            char* o = "S";
            Data d(s, strlen(s), false);
            Data c(o, strlen(o), false);
            
            d = c;
            assert(c == "S");
         }         
      }
};

int
main()
{
   TestData td;
   td.main();
}
