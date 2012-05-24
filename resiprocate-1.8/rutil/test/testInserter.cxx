#include <ostream>
#include <vector>
#include <set>
#include <map>
#include <list>

#include "rutil/resipfaststreams.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/Data.hxx"

using namespace resip;
using namespace std;

struct Foo
{
   Foo() : 
      count(0), 
      value()
   {
   }

   Foo(int c, const Data& v) : 
      count(c),
      value(v)
   {
   }

   bool operator<(const Foo& rhs) const
   {
      if (count < rhs.count)
      {
         return true;
      }

      if (count > rhs.count)
      {
         return false;
      }

         return value < rhs.value;
   }

   bool operator==(const Foo& rhs) const
   {
      return (!(*this < rhs) && !(rhs < *this));
   }

   int count;
   Data value;
};

HashValue(Foo);
HashValueImp(Foo, data.value.hash());

EncodeStream& operator<<(EncodeStream& str, const Foo& foo)
{
   str << "Foo[" << foo.count << " " << foo.value << "]";
   return str;
}

int
main(int argc, char** argv)
{
   resipCerr << "Inserter tests..." << endl;

   {
      resipCerr << Inserter(Foo(0, "null")) << endl;
   }

   {
      vector<Foo> container;
      
      container.push_back(Foo(1, "foo"));
      container.push_back(Foo(2, "bar"));
      container.push_back(Foo(3, "baz"));

      resipCerr << Inserter(container) << endl;
   }

   {
      set<Foo> container;
      
      container.insert(Foo(1, "foo"));
      container.insert(Foo(2, "bar"));
      container.insert(Foo(3, "baz"));

      resipCerr << Inserter(container) << endl;
   }

   {
      map<int, Foo> container;
      container[1] = Foo(1, "foo");
      container[2] = Foo(2, "bar");
      container[3] = Foo(3, "baz");

      resipCerr << Inserter(container) << endl;
   }

   {
      HashMap<int, Foo> container;
      container[1] = Foo(1, "foo");
      container[2] = Foo(2, "bar");
      container[3] = Foo(3, "baz");

      resipCerr << Inserter(container) << endl;      
   }

   {
      HashSet<Foo> container;
      container.insert(Foo(1, "foo"));
      container.insert(Foo(2, "bar"));
      container.insert(Foo(3, "baz"));

      resipCerr << Inserter(container) << endl;      
   }


   resipCerr << "InserterP tests..." << endl;

   {
      Foo *foo = new Foo(0, "null");
      resipCerr << InserterP(foo) << endl;
      delete foo;
   }

   {
      vector<Foo*> container;
      
      container.push_back(new Foo(1, "foo"));
      container.push_back(new Foo(2, "bar"));
      container.push_back(new Foo(3, "baz"));

      resipCerr << InserterP(container) << endl;

      // cleanup
      vector<Foo*>::iterator it = container.begin(); 
      for(;it!=container.end(); it++)
      {
         delete *it;
      }
   }

   {
      set<Foo*> container;
      
      container.insert(new Foo(1, "foo"));
      container.insert(new Foo(2, "bar"));
      container.insert(new Foo(3, "baz"));

      resipCerr << InserterP(container) << endl;

      // cleanup
      set<Foo*>::iterator it = container.begin(); 
      for(;it!=container.end(); it++)
      {
         delete *it;
      }
   }

   {
      map<int, Foo*> container;
      container[1] = new Foo(1, "foo");
      container[2] = new Foo(2, "bar");
      container[3] = new Foo(3, "baz");

      resipCerr << InserterP(container) << endl;

      // cleanup
      map<int, Foo*>::iterator it = container.begin(); 
      for(;it!=container.end(); it++)
      {
         delete it->second;
      }
   }

   {
      HashMap<int, Foo*> container;
      container[1] = new Foo(1, "foo");
      container[2] = new Foo(2, "bar");
      container[3] = new Foo(3, "baz");

      resipCerr << InserterP(container) << endl;      

      // cleanup
      HashMap<int, Foo*>::iterator it = container.begin(); 
      for(;it!=container.end(); it++)
      {
         delete it->second;
      }
   }

   {
      HashSet<Foo*> container;
      container.insert(new Foo(1, "foo"));
      container.insert(new Foo(2, "bar"));
      container.insert(new Foo(3, "baz"));

      resipCerr << InserterP(container) << endl;      

      // cleanup
      HashSet<Foo*>::iterator it = container.begin(); 
      for(;it!=container.end(); it++)
      {
         delete *it;
      }
   }

   resipCerr << "All Ok" << endl;
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

