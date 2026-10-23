#include <cassert>
#include <iostream>
#include <utility>

#include "rutil/Data.hxx"
#include "rutil/DataStream.hxx"
#include "rutil/Inserter.hxx"
#include "rutil/LinkedHashMap.hxx"

// LinkedHashMap: hash lookup, insertion-order iteration.

using namespace std;
using namespace resip;

typedef LinkedHashMap<Data, Data, KeyedDataHash> Map;

static Data
keys(const Map& m)
{
   Data result;
   for (Map::const_iterator i = m.begin(); i != m.end(); ++i)
   {
      result += i->first;
   }
   return result;
}

// Every key must be found, at the element iteration reaches it by
static void
checkIndex(Map& m)
{
   size_t n = 0;
   for (Map::iterator i = m.begin(); i != m.end(); ++i, ++n)
   {
      assert(m.find(i->first) == i);
      assert(m.count(i->first) == 1);
   }
   assert(n == m.size());
}

int
main()
{
   {
      Map m;
      assert(m.empty() && m.size() == 0);
      assert(m.find("a") == m.end());

      m["c"] = "3";
      m["a"] = "1";
      m["b"] = "2";
      assert(keys(m) == "cab");
      assert(m.size() == 3 && !m.empty());
      checkIndex(m);

      // reassigning keeps the original position
      m["c"] = "30";
      assert(keys(m) == "cab");
      assert(m.find("c")->second == "30");

      // insert leaves an existing element alone
      pair<Map::iterator, bool> r = m.insert(Map::value_type("a", "100"));
      assert(!r.second && r.first->second == "1");
      r = m.insert(Map::value_type("d", "4"));
      assert(r.second && r.first->first == "d");
      assert(keys(m) == "cabd");

      // erase by key and by iterator keep the order of the rest
      assert(m.erase("a") == 1);
      assert(m.erase("a") == 0);
      assert(keys(m) == "cbd");
      Map::iterator next = m.erase(m.find("b"));
      assert(next->first == "d");
      assert(keys(m) == "cd");
      checkIndex(m);

      m["a"] = "1";
      assert(keys(m) == "cda");

      m.clear();
      assert(m.empty() && m.find("c") == m.end());
      m["x"] = "1";
      assert(keys(m) == "x");
   }

   {
      // copies have their own index
      Map m;
      m["k1"] = "v1";
      m["k2"] = "v2";
      Map copy(m);
      checkIndex(copy);
      copy["k1"] = "changed";
      copy.erase("k2");
      assert(m.find("k1")->second == "v1");
      assert(keys(m) == "k1k2" && keys(copy) == "k1");

      Map assigned;
      assigned["z"] = "z";
      assigned = m;
      checkIndex(assigned);
      assert(keys(assigned) == "k1k2");

      // moves and swaps carry the index along with the elements
      Map moved(std::move(assigned));
      checkIndex(moved);
      assert(keys(moved) == "k1k2");

      Map other;
      other["o"] = "o";
      moved.swap(other);
      checkIndex(moved);
      checkIndex(other);
      assert(keys(moved) == "o" && keys(other) == "k1k2");

      other = std::move(moved);
      checkIndex(other);
      assert(keys(other) == "o");

      const Map& constRef = m;
      assert(constRef.find("k2")->second == "v2");
      assert(constRef.find("nope") == constRef.end());
   }

   {
      // many keys, in order, with lookups still working
      Map m;
      for (int i = 0; i < 1000; ++i)
      {
         m[Data(i)] = Data(i * 2);
      }
      int expect = 0;
      for (Map::const_iterator i = m.begin(); i != m.end(); ++i, ++expect)
      {
         assert(i->first == Data(expect));
      }
      for (int i = 0; i < 1000; i += 7)
      {
         assert(m.find(Data(i))->second == Data(i * 2));
      }
   }

   {
      Map m;
      m["b"] = "2";
      m["a"] = "1";
      Data out;
      {
         DataStream s(out);
         s << Inserter(m);
      }
      assert(out == "[b -> 2, a -> 1]");
   }

   cerr << "All OK" << endl;
   return 0;
}

/* ====================================================================
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
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
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 */
