#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/DataStream.hxx"
#include "resiprocate/os/Log.hxx"
#include "assert.h"
#include <iostream>

using namespace resip;
using namespace std;

// for friends
class TestData
{
   public:
      void main()
      {

         Log::initialize(Log::COUT, Log::DEBUG, Data::Empty);

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
            Data header(10, true);
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
            Data header(10, true);
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
            Data header(120, true);
            assert(header.empty());

            header += 'c';
            header += " char";
            header += "acters";

            assert(header == "c characters");
         }

         {
            char *txt = "here is some text";
            Data notOwner(Data::Share, txt, strlen(txt));
            assert(!notOwner.mMine);
            
            notOwner += " more text";
            assert(notOwner.mMine);
            assert(notOwner == "here is some text more text");
         }

         {
            char *txt = "here is some text";
            Data notOwner(Data::Share, txt, strlen(txt));
            assert(!notOwner.mMine);
            
            notOwner += '!';
            assert(notOwner.mMine);
            assert(notOwner == "here is some text!");
         }

         {
            char *txt = "here is some text";
            Data notOwner(Data::Share, txt, strlen(txt));
            assert(!notOwner.mMine);
            
            notOwner += Data(" more text");
            assert(notOwner.mMine);
            assert(notOwner == "here is some text more text");
         }

         {
            Data v("some text");
            assert(v.prefix("some"));
            assert(v.prefix("some "));
            assert(!v.prefix("ome "));

            assert(v.prefix(Data::Empty));
            assert(v.prefix("some text"));
            assert(v.prefix(v));
            assert(!v.prefix("some text "));
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
            assert (d1.substr(d1.find("def"), 3) == "def");
            cerr << "substr = " << d1.substr(5,4) << endl;
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

            d1 = Data(100, true);
            d1 ^= Data("0");
            cerr << d1.hex() << endl;


            Data buffer;
            Data working;
            DataStream strm(buffer);
            
            buffer.clear();
            strm << "user=phone";
            strm.flush();
            working ^= buffer;

            buffer.clear();
            strm << "maddr=192.168.1.1";
            strm.flush();
            working ^= buffer;


            Data result = working;
            working.clear();

            buffer.clear();
            strm << "maddr=192.168.1.1";
            strm.flush();
            working ^= buffer;

            buffer.clear();
            strm << "user=phone";
            strm.flush();
            working ^= buffer;

            assert(result == working);
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
            Data d((unsigned long)235235);
            assert(d == "235235");
         }
      }
};

int
main()
{
   TestData td;
   td.main();

   cerr << sizeof(Data) << endl;
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
