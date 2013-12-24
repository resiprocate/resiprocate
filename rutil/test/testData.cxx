#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Log.hxx"
#include "assert.h"
#include <iostream>
#include <limits>
#include <math.h>

using namespace resip;
using namespace std;

#define DOUBLE_EQUALITY(a, b) essentiallyEqual(a, b, std::numeric_limits<double>::epsilon())

// for friends
class TestData
{
   public:

      bool approximatelyEqual(double a, double b, float epsilon)
      {
         return fabs(a - b) <= ( (fabs(a) < fabs(b) ? fabs(b) : fabs(a)) * epsilon);
      }

      bool essentiallyEqual(double a, double b, double epsilon)
      {
         return fabs(a - b) <= ( (fabs(a) > fabs(b) ? fabs(b) : fabs(a)) * epsilon);
      }

      int main()
      {
         Log::initialize(Log::Cout, Log::Debug, Data::Empty);

	 {
	   const char* txt = "buffer";
	   Data d(txt);
	   // copies
	   assert(txt != d.data());

	   const char* b = d.data();
	   d.c_str();
	   // not reallocated
	   assert(b == d.data());
	 }
	 
	 {
	   const char* txt = "buffer";
	   Data d(Data::Share, txt, strlen(txt));
	   // shared
	   assert(txt == d.data());

	   d.c_str();
	   // reallocated
	   assert(txt != d.data());
	 }

	 {
	   const char* txt = "buffer";
	   Data d(Data::Borrow, txt, strlen(txt));
	   // shared
	   assert(txt == d.data());
	   const char* b = d.data();

	   d.c_str();
	   // reallocated
	   assert(b != d.data());
	 }

	 {
	   const int s = 12;
	   char* txt = new char[s];
	   Data d(Data::Take, txt, s);
	   // shared
	   assert(txt == d.data());

	   d.c_str();
	   // reallocated
	   assert(txt != d.data());
	 }

         {
            {
               int length = 16;
               char* buffer = new char [length];
   
               for (int i=0; i<16; ++i)
               {
                  buffer[i] = ' ';
               }
   
               Data target(Data::Take, buffer, length);
               std::cerr << target.c_str() << endl;
            }
            
            {
               Data input("abcdefghij");
               std::cerr << "T0: " << input << std::endl;
               input.replace("a", "b");
               std::cerr << "T1: " << input << std::endl;
               assert(input == "bbcdefghij");
               input.replace("bb", "");
               std::cerr << "T2: " << input << std::endl;
               assert(input == "cdefghij");
            }
            {
               Data input("");
               std::cerr << "T0: " << input << std::endl;
               input.replace("a", "b");
               std::cerr << "T1: " << input << std::endl;
               assert(input == "");
               input.replace("bb", "");
               std::cerr << "T2: " << input << std::endl;
               assert(input == "");
            }

            {
               Data from;
               Data to;
               Data example;

               // asserts
               //example.replace(from, to);
            }

            {
               Data from("a");
               Data to("b");
               Data example;

               example.replace(from, to);
               assert(example.empty());
            }

            {
               Data from("a");
               Data to("b");
               Data example("c");

               example.replace(from, to);
               assert(example == "c");
            }

            {
               Data from("a");
               Data to("b");
               Data example("a");

               example.replace(from, to);
               assert(example == "b");
            }

            {
               Data from("a");
               Data to("b");
               Data example("aaaa");

               example.replace(from, to);
               assert(example == "bbbb");
            }
	    
            {
               Data from("a");
               Data to("b");
               Data example("abracadabra");

               example.replace(from, to);
               assert(example == "bbrbcbdbbrb");
            }

            {
               Data from("aa");
               Data to("b");
               Data example("aa");

               example.replace(from, to);
               assert(example == "b");
            }

            {
               Data from("aa");
               Data to("b");
               Data example("aaaaa");

               example.replace(from, to);
               assert(example == "bba");
            }

            {
               Data from("a");
               Data to("bb");
               Data example("a");

               example.replace(from, to);
               assert(example == "bb");
            }

            {
               Data from("a");
               Data to("bb");
               Data example("abracadabra");

               example.replace(from, to);
               assert(example == "bbbrbbcbbdbbbrbb");
            }

            {
               Data from("a");
               Data to("bb");
               const char* buffer = "abracadabra";
               Data example(Data::Share, buffer, strlen(buffer));

               example.replace(from, to);
               assert(example == "bbbrbbcbbdbbbrbb");
            }

            {
               Data from("a");
               Data to("aa");
               Data example("a");

               example.replace(from, to);
               assert(example == "aa");
            }

            {
               Data from("a");
               Data to("aa");
               Data example("abracadabra");

               example.replace(from, to);
               assert(example == "aabraacaadaabraa");
            }
	    
            {
               Data from("abracadabra");
               Data to("a");
               Data example("abracadabra");

               example.replace(from, to);
               assert(example == "a");
            }

            {
               Data from("abracadabra");
               Data to("");
               Data example("abracadabra");

               example.replace(from, to);
               assert(example == "");
            }

            {
               Data from("abracadabra");
               Data to("");
               Data example("abracadabraabracadabraabracadabra");

               example.replace(from, to);
               assert(example == "");
            }
         }

         {
            Data input("abc123abca");
            std::cerr << "T0: " << input << std::endl;
            input.replace("abc", "ABCD");
            std::cerr << "T1: " << input << std::endl;
            assert(input == "ABCD123ABCDa");
         }
         
         
         {
            const char* s = "a";
            const char* ss = "bb";
            const char* sss = "ccc";

            Data one;
            Data two;
            Data three;

            for (int i = 0; i < 100; ++i)
            {
               one.append(s, strlen(s));
               two.append(ss, strlen(ss));
               three.append(sss, strlen(sss));
            }

            assert(one.size() == 100);
            assert(two.size() == 200);
            assert(three.size() == 300);
         }

         {
            Data httpString("safe");

            Data enc;
            enc = httpString.urlEncoded();

            cerr << "res: " << enc << endl;
         }

         {
            Data httpString("-_.~!$'()*,;=:@/?");
            httpString += "0123456789";

            Data result;
            {
               DataStream str(result);
               httpString.urlEncode(str);
            }
            assert(result == httpString.urlEncoded());
            cerr << ">> " << httpString.urlEncoded() << endl;
            cerr << "<< " << httpString.urlEncoded().urlDecoded() << endl;
            cerr << ".. " << httpString << endl;

            assert(httpString == httpString.urlEncoded().urlDecoded());
            assert(result == httpString);
         }

         {
            Data httpString("http::/foo.com/in word?arg1=\"quote%\"&arg2=\"%%%%%\"");

            Data result;
            {
               DataStream str(result);
               httpString.urlEncode(str);
            }

            cerr << result << endl;
            assert(result == "http::/foo.com/in+word?arg1=%22quote%25%22%26arg2=%22%25%25%25%25%25%22");
            assert(result == httpString.urlEncoded());

            cerr << ">> " << httpString.urlEncoded() << endl;
            cerr << "<< " << httpString.urlEncoded().urlDecoded() << endl;
            cerr << ".. " << httpString << endl;

            assert(httpString == httpString.urlEncoded().urlDecoded());
         }

         {
            Data needsCharEncode("CharEncode % me");
            cerr << "original " << needsCharEncode << endl;
            cerr << "charEncoded " << needsCharEncode.charEncoded() << endl;
            cerr << "charUnencoded " << needsCharEncode.charEncoded().charUnencoded() << endl;

            assert(needsCharEncode.charEncoded().charUnencoded() == needsCharEncode);
         }

         {
            Data needsCharEncode("CharEncode % me");
            needsCharEncode += " \";/?:@&=+%$,/t-_.!~*'()";
            needsCharEncode += char(0);
            needsCharEncode += char(254);
            needsCharEncode += char(17);

            cerr << needsCharEncode.charEncoded() << endl;

            assert(needsCharEncode.charEncoded().charUnencoded() == needsCharEncode);
         }

         {
            Data needsNoCharEncode("dontcharEncodeme");

            cerr << needsNoCharEncode.charEncoded() << endl;

            assert(needsNoCharEncode.charEncoded().charUnencoded() == needsNoCharEncode);            
         }

         {
            Data charEncodeCase("%5b%5D%5B%5d");

            cerr << charEncodeCase.charUnencoded() << endl;

            Data shouldMatch("[][]");
            assert(charEncodeCase.charUnencoded()==shouldMatch);
         }

         {
            Data s1;
            assert(s1.convertInt() == 0);
            
            Data s2("12foo");
            assert(s2.convertInt() == 12);
            
            Data s3("12");
            assert(s3.convertInt() == 12);

            Data s4("foo");
            assert(s4.convertInt() == 0);

	    Data s5("     ");
	    assert(s5.convertInt() == 0);

	    Data s6("    +");
	    assert(s6.convertInt() == 0);

	    Data s7("    +17");
	    assert(s7.convertInt() == 17);

	    Data s8("    -17");
	    assert(s8.convertInt() == -17);

	    Data s9("    --17");
	    assert(s9.convertInt() == 0);
         }

         {
            Data s1;
            assert(s1.convertUnsignedLong() == 0);
            
            Data s2("12foo");
            assert(s2.convertUnsignedLong() == 12);
            
            Data s3("12");
            assert(s3.convertUnsignedLong() == 12);

            Data s4("foo");
            assert(s4.convertUnsignedLong() == 0);

	    Data s5("     ");
	    assert(s5.convertUnsignedLong() == 0);

	    Data s6("    +");
	    assert(s6.convertUnsignedLong() == 0);

	    Data s7("    +17");
	    assert(s7.convertUnsignedLong() == 17);

	    Data s8("    -17");
	    assert(s8.convertUnsignedLong() == 0);
         }

         {
            Data s1;
            assert(s1.convertUInt64() == 0);
            
            Data s2("12foo");
            assert(s2.convertUInt64() == 12);
            
            Data s3("12");
            assert(s3.convertUInt64() == 12);

            Data s4("foo");
            assert(s4.convertUInt64() == 0);

	    Data s5("     ");
	    assert(s5.convertUInt64() == 0);

	    Data s6("    +");
	    assert(s6.convertUInt64() == 0);

	    Data s7("    +17");
	    assert(s7.convertUInt64() == 17);

	    Data s8("    -17");
	    assert(s8.convertUInt64() == 0);
         }

         {
            Data s1;
            assert(s1.convertSize() == 0);
            
            Data s2("12foo");
            assert(s2.convertSize() == 12);
            
            Data s3("12");
            assert(s3.convertSize() == 12);

            Data s4("foo");
            assert(s4.convertSize() == 0);

	    Data s5("     ");
	    assert(s5.convertSize() == 0);

	    Data s6("    +");
	    assert(s6.convertSize() == 0);

	    Data s7("    +17");
	    assert(s7.convertSize() == 17);

	    Data s8("    -17");
	    assert(s8.convertSize() == 0);
         }

#ifndef RESIP_FIXED_POINT
         {
            Data s1;
            assert(s1.convertDouble() == 0);

            Data s2("12foo");
            assert(s2.convertDouble() == 12);

            Data s3("12");
            assert(s3.convertDouble() == 12);

            Data s4("foo");
            assert(s4.convertDouble() == 0);

            Data s5("     ");
            assert(s5.convertDouble() == 0);

            Data s6("    +");
            assert(s6.convertDouble() == 0);

            Data s7("    +17");
            assert(s7.convertDouble() == 17);

            Data s8("    -17");
            assert(s8.convertDouble() == -17);

            Data s9("    --17");
            assert(s9.convertDouble() == 0);
         }

         {
            Data s1(".");
            assert(s1.convertDouble() == 0);

            Data s2("12.12foo");
            assert(DOUBLE_EQUALITY(s2.convertDouble(), 12.12L));

            Data s3("12.12");
            assert(DOUBLE_EQUALITY(s3.convertDouble(), 12.12L));

            Data s4(".foo");
            assert(s4.convertDouble() == 0);

            Data s5("     .");
            assert(s5.convertDouble() == 0);

            Data s6("    +.");
            assert(s6.convertDouble() == 0);

            Data s6a("    -.");
            assert(s6a.convertDouble() == 0);

            Data s7("    +17.17");
            assert(DOUBLE_EQUALITY(s7.convertDouble(), 17.17L));

            Data s8("    -17.17");
            assert(DOUBLE_EQUALITY(s8.convertDouble(), -17.17L));

            Data s9("    -17.17foo");
            assert(DOUBLE_EQUALITY(s9.convertDouble(), -17.17L));

            Data s10("    --17.17");
            assert(s10.convertDouble() == 0);

            Data s11("    -0000.017");
            assert(DOUBLE_EQUALITY(s11.convertDouble(), -0.017L));

            Data s12(".017");
            assert(DOUBLE_EQUALITY(s12.convertDouble(), 0.017L));

            Data s13("    .017");
            assert(DOUBLE_EQUALITY(s13.convertDouble(), 0.017L));

            Data s14("    +.017");
            assert(DOUBLE_EQUALITY(s14.convertDouble(), 0.017L));

            Data s15("    -.017");
            assert(DOUBLE_EQUALITY(s15.convertDouble(), -0.017L));

         }
#endif
         {
            Data s;
            s = "some text";
            s += Data::Empty;
            s += "";

            assert(s == "some text");
         }

         {
            Data d;
            const char *q = "\0";
            d += q;
           
            for(const char *p = 
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789"
                   "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
                *p;
                ++p)
            {
               d += *p;
               d += q;
               d += "~";
            }
           
         }

         {
            Data d;
            assert(d.empty());
            assert(d.c_str()[0] == 0);
         }

         {
            const int maxs = 1024;
            Data a;
            for (int i = 0; i < maxs; i++)
            {
               Data b(a.c_str());
               Data c(b);

               a += "a";
            }
            cerr << "test data size 0.." << maxs << endl;
         }

         // test comparison
         {
            {
               Data c = "sadfsdf";
               Data d;
               d = c;
               assert(c == d);
            }

            {
               const char* f = "asdasd";
               Data d = Data(f);
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
               const char* f = "asdasd";
               Data d = Data(f);
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
               Data d(Data::Share, c, 4);
               assert(d < c);
            }

            {
               const char * c("asdfasfdsadf");
               Data d(Data::Share, c, strlen(c));
               assert(!(d < c));
            }

            {
               const char * c("asdfasfdsadf");
               Data d(Data::Share, c, strlen(c));
               assert(!(d < c));
               Data c1(d); // copy, null terminate
               assert(!(d < c1));
               assert(!(c1 < d));
            }

            {
               const char* f = "asdasd";
               Data d("fsadf");

               assert(!(d <= f));
            }

            {
               const Data f = "asdasd";
               Data d("fsadf");

               assert(!(d <= f));
            }

            {
               const char* f = "asdasd";
               Data d = Data(f);
               assert(d <= f);
            }

            {
               Data d;
               const char* f = "asdasd";
               d = f;
               assert(d <= f);
            }

            {
               Data d("asdfasfdsadf");
               const char* f = "asdasd";
               d = f;
               assert(d <= f);
            }
         
            {
               Data a("qwerty");
               Data b("qwerty");
               assert(a <= b);
            }
            {
               Data a("qwert");
               Data b("qwerty");
               assert(a <= b);
            }
            {
               Data a("qwert");
               Data b("qwerty");
               assert(a <= b);
               assert(!(b <= a));
            }

            {
               const char* f = "asdasda";
               Data d("asdasd");
               assert(d <= f);
            }

            {
               const char * c("asdfasfdsadf");
               Data d(Data::Share, c, 4);
               assert(d <= c);
            }

            {
               const char * c("asdfasfdsadf");
               Data d(Data::Share, c, strlen(c));
               assert(d <= c);
            }

            {
               const char * c("asdfasfdsadf");
               Data d(Data::Share, c, strlen(c));
               assert(d <= c);
               Data c1(d); // copy, null terminate
               assert(d <= c1);
               assert(c1 <= d);
            }
         }

         // test assignment
         {
            {
               Data d("sadfsa");
               d = "gasdfg";

               assert(d == "gasdfg");
            }

            {
               Data d;
               d = "gasdfg";

               assert(d == "gasdfg");
            }

            {
               Data d("sdfsdf");
               Data e(d);
               Data f("fsdgsdafg");
               Data g(f);

               e = g;
               assert(e == g);
            }
         }

         {
            // test resizing
            Data header(10, Data::Preallocate);
            assert(header.empty());

            header += 'c';
            header += " char";
            header += "acters";

            assert(header.size() > 10);
            cerr << header << endl;
            assert(header == "c characters");
         }

         {
            // test resizing
            Data header(10, Data::Preallocate);
            assert(header.empty());

            header += 'c';
            header += " char";
            header += Data("acters");

            assert(header.size() > 10);
            cerr << header << endl;
            assert(header == "c characters");
         }

         {
            // test resizing
            Data header(120, Data::Preallocate);
            assert(header.empty());

            header += 'c';
            header += " char";
            header += "acters";

            assert(header == "c characters");
         }

         {
            const char *txt = "here is some text";
            Data notOwner(Data::Share, txt, strlen(txt));
            assert(notOwner.mShareEnum == Data::Share);
            
            notOwner += " more text";
            assert(notOwner.mShareEnum == Data::Take);
            assert(notOwner == "here is some text more text");
         }

         {
            const char *txt = "here is some text";
            Data notOwner(Data::Share, txt, strlen(txt));
            assert(notOwner.mShareEnum == Data::Share);
            
            notOwner += '!';
            assert(notOwner.mShareEnum == Data::Take);
            assert(notOwner == "here is some text!");
         }

         {
            const char *txt = "here is some text";
            Data notOwner(Data::Share, txt, strlen(txt));
            assert(notOwner.mShareEnum == Data::Share);
            
            notOwner += Data(" more text");
            assert(notOwner.mShareEnum == Data::Take);
            assert(notOwner == "here is some text more text");
         }

         {
            Data v("some text");
            assert(v.prefix("some"));
            assert(v.prefix("some "));
            assert(!v.prefix("ome "));
            assert(!v.prefix("some more text"));

            assert(v.prefix(Data::Empty));
            assert(v.prefix("some text"));
            assert(v.prefix(v));
            assert(!v.prefix("some text "));
         }

         {
            Data v("some text");
            assert(v.postfix("text"));
            assert(v.postfix(" text"));
            assert(!v.postfix("tex"));
            assert(!v.postfix("more some text"));

            assert(v.postfix(Data::Empty));
            assert(v.postfix("some text"));
            assert(v.postfix(v));
            assert(!v.postfix(" some text"));
         }

         {
            Data transport("transport");
            assert(isEqualNoCase(transport, "transport"));
         }
         
         {
            Data d1("0123456789");
            assert(d1.find("0") == 0);
            assert(d1.find("1") == 1);
            assert(d1.find("8") == 8);
            assert(d1.find("9") == 9);
            assert(d1.find("01") == 0);
            assert(d1.find("12") == 1);
            assert(d1.find("a") == Data::npos);
            assert(d1.find("0123456789") == 0);
            assert(d1.find("0123456789a") == Data::npos);

            Data d2;
            assert(d2.find("0") == Data::npos);            
            assert(d2.find("abc") == Data::npos);
            assert(d2.find("") == Data::npos);
         }
         {
            Data d1("abcdefghi");
            assert(d1.find("def") == 3);
            assert(d1.find("def", 3) == 3);
            assert (d1.substr(d1.find("def"), 3) == "def");
            cerr << "substr = " << d1.substr(5,4) << endl;
         }
         {
            Data d1("http://123456/123");
            assert(d1.find("/") == 5);
            assert(d1.find("/", 7) == 13);
         }
         {
            Data d1("0");
            Data d2("0");
            d1 ^= d2;
            cerr << d1.hex() << endl;
            assert(d1.size() == 1);
            assert(d1[0] == 0);

            d1 = "0";
            d1 ^= Data();
            cerr << d1.hex() << endl;
            assert(d1.size() == 1);
            assert(d1[0] == '0');

            d1 = Data();
            d1 ^= Data("0");
            cerr << d1.hex() << endl;
            assert(d1.size() == 1);
            assert(d1[0] == '0');

            d1 = Data();
            d1 ^= Data();
            cerr << d1.hex() << endl;
            assert(d1.size() == 0);


            d1 = "01234";
            d1 ^= Data("01234");
            cerr << d1.hex() << endl;
            assert(d1.size() == 5);
            assert(d1[0] == 0);
            assert(d1[4] == 0);

            d1 = "012";
            d1 ^= Data("01234");
            cerr << d1.hex() << endl;
            assert(d1.size() == 5);
            assert(d1[0] == 0);
            assert(d1[1] == 0);
            assert(d1[2] == 0);
            assert(d1[3] == '3');
            assert(d1[4] == '4');

            d1 ^= Data("01234");
            cerr << d1.hex() << endl;
            assert(d1[0] == '0');
            assert(d1[1] == '1');
            assert(d1[2] == '2');
            assert(d1[3] == 0);
            assert(d1[4] == 0);

            d1 = Data(100, Data::Preallocate);
            d1 ^= Data("0");
            cerr << d1.hex() << endl;

            {
               Data buffer;
               Data working;

               {
                  DataStream strm(buffer);
                  strm << "user=phone";
               }
               working ^= buffer;

               buffer.clear();
               {
                  DataStream strm(buffer);
                  strm << "maddr=192.168.1.1";
               }
               working ^= buffer;

               Data result = working;
               working.clear();

               buffer.clear();
               {
                  DataStream strm(buffer);
                  strm << "maddr=192.168.1.1";
               }
               working ^= buffer;

               buffer.clear();
               {
                  DataStream strm(buffer);
                  strm << "user=phone";
               }
               working ^= buffer;

               assert(result == working);
            }
         }

         {
            Data d("f");
            Data raw = d.fromHex();
            assert(raw.size() == 1);
            assert((unsigned char)(raw[0]) == 0xf);

            d = "Ff";
            raw = d.fromHex();
            assert(raw.size() == 1);
            assert((unsigned char)(raw[0]) == 0xff);

            d = "Fff";
            raw = d.fromHex();
            assert(raw.size() == 2);
            assert((unsigned char)(raw[0]) == 0xf);
            assert((unsigned char)(raw[1]) == 0xff);

            d = "d3b07384d113edec49eaa6238ad5ff00";
            raw = d.fromHex();
            assert(raw.size() == 16);
            assert((unsigned char)raw[0] == 0xd3);
            assert((unsigned char)raw[15] == 0x00);
         }
         
         {
            Data d("012345");
            assert(d[0] == '0');
            assert(d[1] == '1');
            assert(d[2] == '2');
            assert(d[3] == '3');
            assert(d[4] == '4');
            assert(d[5] == '5');
         }

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
            char blah[] = "12345";
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
            assert(Data(UInt64(0)) == "0");
            assert(Data(UInt64(1)) == "1");
            assert(Data(UInt64(18446744073709551615ULL)) == "18446744073709551615");  // 2^64 - 1
         }

         {
            cerr << "!! " << Data(true) << endl;

            assert(Data(true) == "true");
            assert(Data(false) == "false");
         }

         {
            assert(Data('c') == "c");
         }

#ifndef RESIP_FIXED_POINT
         {
            assert(Data(0.21344) == "0.2134");
            assert(Data(0.21347) == "0.2135");
            assert(Data(-0.21347) == "-0.2135");
            assert(Data(-0.21344) == "-0.2134");
            cerr << "!! " << Data(-123454.21344, Data::FiveDigitPrecision) << endl;            
            assert(Data(-123454.21344, Data::FiveDigitPrecision) == "-123454.21344");
            assert(Data(-123454.21344, Data::SevenDigitPrecision) == "-123454.21344");
         }
#endif      
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
            Data d("123");
            assert(d.size() == 3);
         }

         {
            Data d("one");
            Data c("two");
            d += c;
            assert(d == "onetwo");

            Data empt;
            cerr << empt + d << endl;
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
            s += 'c';
            assert(s == "c");
            assert(s.size() == 1);
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
            Data s;
            s = "some text";
            s += Data::Empty;

            assert(s == "some text");
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
            cerr << "test MD5" << endl;
            Data d;
            assert(d.md5() == "d41d8cd98f00b204e9800998ecf8427e");

            Data d1("qwertyuiop");
            assert(d1.md5() == "6eea9b7ef19179a06954edd0f6c05ceb");
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
            assert(!(d != c));
            d = c;
            assert(d.empty());
         }
         {
            char s[] = "userB@whistler.gloo.net:6062\r\nCo\031";
            char o[] = "S";
            Data d(Data::Share, s, strlen(s));
            Data c(Data::Share, o, strlen(o));
            
            d = c;
            assert(c == "S");
         }         
         {
            Data d((UInt64)235235);
            assert(d == "235235");
         }
 
         if (1)
         {
            Data d3("MTIz"); Data e3("123" );
            //cerr << "base64 test " <<e3<< " = "<< d3.base64decode().c_str()<<endl;
            //cerr << "base64 test " <<d3<< " = "<< e3.base64encode().c_str()<<endl;
            assert( d3.base64decode() == e3 );
            assert( e3.base64encode() == d3 );
            
            Data d1("MQ=="); Data e1("1" );
            //cerr << "base64 test "<<e1<<" = <"<<d1.base64decode()<<">"<<endl;
            //cerr << "base64 test hex "<<e1.hex()<<" = <"<<d1.base64decode().hex()<<">"<<endl;
            //cerr << "base64 test "<<d1<<" = <"<<e1.base64encode()<<">"<<endl;
            assert( e1 == d1.base64decode() );
            assert( e1.base64encode() == d1 );
          
            Data d2("MTI=");
            assert( d2.base64decode() == Data("12" ) );
            assert(  Data("12" ).base64encode() == d2 );
            
            Data d4("MTIzNA==");
            assert( d4.base64decode() == Data("1234" ) );
            assert(  Data("1234" ).base64encode() == d4 );
            
            Data d5("MTIzNDU=");
            assert( d5.base64decode() == Data("12345" ) );
            assert(  Data("12345" ).base64encode() == d5 );
            
            Data d6("MTIzNDU2");
            assert( d6.base64decode() == Data("123456" ) );
            assert(  Data("123456" ).base64encode() == d6 );
            
            Data d7("MTIzNDU2Nw==");
            assert( d7.base64decode() == Data("1234567" ) );
            assert(  Data("1234567" ).base64encode() == d7 );
         }
         
         for(int i=0; i<4; ++i)
         {
            const char* buf1="5d7a9b7c02034b5b";
            const char* buf2=" 5d7a9b7c02034b5b";
            const char* buf3="  5d7a9b7c02034b5b";
            const char* buf4="   5d7a9b7c02034b5b";
            const char* ubuf1="5D7A9B7C02034B5B";
            const char* ubuf2=" 5D7A9B7C02034B5B";
            const char* ubuf3="  5D7A9B7C02034B5B";
            const char* ubuf4="   5D7A9B7C02034B5B";

            Data d1(Data::Share, buf1, 16-i);
            Data d2(Data::Share, buf2+1, 16-i);
            Data d3(Data::Share, buf3+2, 16-i);
            Data d4(Data::Share, buf4+3, 16-i);

            Data u1(Data::Share, ubuf1, 16-i);
            Data u2(Data::Share, ubuf2+1, 16-i);
            Data u3(Data::Share, ubuf3+2, 16-i);
            Data u4(Data::Share, ubuf4+3, 16-i);

            // All of these point to the same data, but aligned differently.
            assert(d1.hash()==d2.hash());
            assert(d1.hash()==d3.hash());
            assert(d1.hash()==d4.hash());

            assert(d1.hash()!=u1.hash());
            assert(d1.hash()!=u2.hash());
            assert(d1.hash()!=u3.hash());
            assert(d1.hash()!=u4.hash());

            assert(d1.caseInsensitivehash()==d2.caseInsensitivehash());
            assert(d1.caseInsensitivehash()==d3.caseInsensitivehash());
            assert(d1.caseInsensitivehash()==d4.caseInsensitivehash());

            assert(d1.caseInsensitivehash()==u1.caseInsensitivehash());
            assert(d1.caseInsensitivehash()==u2.caseInsensitivehash());
            assert(d1.caseInsensitivehash()==u3.caseInsensitivehash());
            assert(d1.caseInsensitivehash()==u4.caseInsensitivehash());

            assert(d1.caseInsensitiveTokenHash()==d2.caseInsensitiveTokenHash());
            assert(d1.caseInsensitiveTokenHash()==d3.caseInsensitiveTokenHash());
            assert(d1.caseInsensitiveTokenHash()==d4.caseInsensitiveTokenHash());

            assert(d1.caseInsensitiveTokenHash()==u1.caseInsensitiveTokenHash());
            assert(d1.caseInsensitiveTokenHash()==u2.caseInsensitiveTokenHash());
            assert(d1.caseInsensitiveTokenHash()==u3.caseInsensitiveTokenHash());
            assert(d1.caseInsensitiveTokenHash()==u4.caseInsensitiveTokenHash());

            assert(d1.caseInsensitiveTokenCompare(d1));
            assert(d1.caseInsensitiveTokenCompare(d2));
            assert(d1.caseInsensitiveTokenCompare(d3));
            assert(d1.caseInsensitiveTokenCompare(d4));
            assert(d1.caseInsensitiveTokenCompare(u1));
            assert(d1.caseInsensitiveTokenCompare(u2));
            assert(d1.caseInsensitiveTokenCompare(u3));
            assert(d1.caseInsensitiveTokenCompare(u4));

            assert(d2.caseInsensitiveTokenCompare(d1));
            assert(d2.caseInsensitiveTokenCompare(d2));
            assert(d2.caseInsensitiveTokenCompare(d3));
            assert(d2.caseInsensitiveTokenCompare(d4));
            assert(d2.caseInsensitiveTokenCompare(u1));
            assert(d2.caseInsensitiveTokenCompare(u2));
            assert(d2.caseInsensitiveTokenCompare(u3));
            assert(d2.caseInsensitiveTokenCompare(u4));

            assert(d3.caseInsensitiveTokenCompare(d1));
            assert(d3.caseInsensitiveTokenCompare(d2));
            assert(d3.caseInsensitiveTokenCompare(d3));
            assert(d3.caseInsensitiveTokenCompare(d4));
            assert(d3.caseInsensitiveTokenCompare(u1));
            assert(d3.caseInsensitiveTokenCompare(u2));
            assert(d3.caseInsensitiveTokenCompare(u3));
            assert(d3.caseInsensitiveTokenCompare(u4));

            assert(d4.caseInsensitiveTokenCompare(d1));
            assert(d4.caseInsensitiveTokenCompare(d2));
            assert(d4.caseInsensitiveTokenCompare(d3));
            assert(d4.caseInsensitiveTokenCompare(d4));
            assert(d4.caseInsensitiveTokenCompare(u1));
            assert(d4.caseInsensitiveTokenCompare(u2));
            assert(d4.caseInsensitiveTokenCompare(u3));
            assert(d4.caseInsensitiveTokenCompare(u4));

            assert(u1.caseInsensitiveTokenCompare(d1));
            assert(u1.caseInsensitiveTokenCompare(d2));
            assert(u1.caseInsensitiveTokenCompare(d3));
            assert(u1.caseInsensitiveTokenCompare(d4));
            assert(u1.caseInsensitiveTokenCompare(u1));
            assert(u1.caseInsensitiveTokenCompare(u2));
            assert(u1.caseInsensitiveTokenCompare(u3));
            assert(u1.caseInsensitiveTokenCompare(u4));

            assert(u2.caseInsensitiveTokenCompare(d1));
            assert(u2.caseInsensitiveTokenCompare(d2));
            assert(u2.caseInsensitiveTokenCompare(d3));
            assert(u2.caseInsensitiveTokenCompare(d4));
            assert(u2.caseInsensitiveTokenCompare(u1));
            assert(u2.caseInsensitiveTokenCompare(u2));
            assert(u2.caseInsensitiveTokenCompare(u3));
            assert(u2.caseInsensitiveTokenCompare(u4));

            assert(u3.caseInsensitiveTokenCompare(d1));
            assert(u3.caseInsensitiveTokenCompare(d2));
            assert(u3.caseInsensitiveTokenCompare(d3));
            assert(u3.caseInsensitiveTokenCompare(d4));
            assert(u3.caseInsensitiveTokenCompare(u1));
            assert(u3.caseInsensitiveTokenCompare(u2));
            assert(u3.caseInsensitiveTokenCompare(u3));
            assert(u3.caseInsensitiveTokenCompare(u4));

            assert(u4.caseInsensitiveTokenCompare(d1));
            assert(u4.caseInsensitiveTokenCompare(d2));
            assert(u4.caseInsensitiveTokenCompare(d3));
            assert(u4.caseInsensitiveTokenCompare(d4));
            assert(u4.caseInsensitiveTokenCompare(u1));
            assert(u4.caseInsensitiveTokenCompare(u2));
            assert(u4.caseInsensitiveTokenCompare(u3));
            assert(u4.caseInsensitiveTokenCompare(u4));
         }
         std::cerr << "All OK" << endl;
         return 0;
      }
};

int
main()
{
   TestData td;
   int rVal = td.main();
   return rVal;
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
