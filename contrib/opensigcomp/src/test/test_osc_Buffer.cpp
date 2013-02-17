/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2006 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */


#include <iostream>
#include "Buffer.h"
#include "TestList.h"

bool test_osc_Buffer()
{
  // Test default constructor
  osc::Buffer empty;
  TEST_ASSERT(empty.size() == 0);

  // Test byte array constructor
  osc::byte_t bytes[4] = {1,2,3,4};
  osc::Buffer a(bytes, sizeof(bytes));
  TEST_ASSERT_EQUAL(a.size(), sizeof(bytes));
  TEST_ASSERT_EQUAL_BUFFERS(a.data(), bytes, sizeof(bytes));
  TEST_ASSERT(a.data() != bytes);

  // Test comparitor with one empty buffer
  TEST_ASSERT(!(a == empty));
  TEST_ASSERT(!(empty == a));

  // Test copy constructor
  osc::Buffer b(a);
  TEST_ASSERT_EQUAL(a.size(), b.size());
  TEST_ASSERT_EQUAL_BUFFERS(a.data(), b.data(), a.size());
  TEST_ASSERT(a.data() != b.data());

  // Test comparitor with equal buffers
  TEST_ASSERT(a == b);
  TEST_ASSERT(b == a);

  // Test first copy method
  osc::Buffer c;
  c.copy(bytes, sizeof(bytes));
  TEST_ASSERT_EQUAL(c.size(), sizeof(bytes));
  TEST_ASSERT_EQUAL_BUFFERS(c.data(), bytes, sizeof(bytes));
  TEST_ASSERT(c.data() != bytes);

  // Test second copy method
  osc::Buffer d;
  d.copy(a);
  TEST_ASSERT_EQUAL(a.size(), d.size());
  TEST_ASSERT_EQUAL_BUFFERS(a.data(), d.data(), a.size());
  TEST_ASSERT(a.data() != d.data());

  // Test third copy method
  osc::Buffer e;
  e.copy(a, 1, 2);
  TEST_ASSERT_EQUAL(e.size(), 2);
  TEST_ASSERT_EQUAL_BUFFERS(e.data(), bytes+1, e.size());

  // Test comparitor with unequal buffers
  TEST_ASSERT(!(a == e));
  TEST_ASSERT(!(e == a));

  // Test subsume method
  const osc::byte_t *tmp = a.data();
  e.subsume(a);
  TEST_ASSERT_EQUAL(a.size(), 0);
  TEST_ASSERT_EQUAL(e.data(), tmp);
  TEST_ASSERT_EQUAL(e.size(), sizeof(bytes));
  TEST_ASSERT_EQUAL_BUFFERS(e.data(), bytes, sizeof(bytes));
  TEST_ASSERT(e.data() != bytes);

  // Test free method
  b.free();
  TEST_ASSERT_EQUAL(b.size(), 0);

  // Test comparitor with two empty buffers
  TEST_ASSERT(a == b);
  TEST_ASSERT(b == a);
  TEST_ASSERT(a == empty);
  TEST_ASSERT(empty == a);

  // Test assignment operator
  a = e;
  TEST_ASSERT(a == e);
  TEST_ASSERT(e == a);
  a = b;
  TEST_ASSERT(a == b);
  TEST_ASSERT(b == a);
  TEST_ASSERT(a == empty);
  TEST_ASSERT(empty == a);

  // Test operator [] (non-const and const)
  a = b = e;
  a[0] = 7;
  TEST_ASSERT_EQUAL(a.data()[0],7);
  TEST_ASSERT_EQUAL_BUFFERS(a.data()+1, bytes+1, sizeof(bytes)-1);
  TEST_ASSERT_EQUAL(a[0],7);
  TEST_ASSERT_EQUAL(a[1],2);
  TEST_ASSERT_EQUAL(a[2],3);
  TEST_ASSERT_EQUAL(a[3],4);
  const osc::Buffer f(a);
  TEST_ASSERT_EQUAL(f[0],7);
  TEST_ASSERT_EQUAL(f[1],2);
  TEST_ASSERT_EQUAL(f[2],3);
  TEST_ASSERT_EQUAL(f[3],4);

  // Test operator <
  a = b;
  TEST_ASSERT(!(a<b));
  TEST_ASSERT(!(b<a));
  a[3]++;
  TEST_ASSERT(!(a==b));
  TEST_ASSERT(!(a<b));
  TEST_ASSERT(b<a);
  a[3] = 0;
  TEST_ASSERT(!(a==b));
  TEST_ASSERT(a<b);
  TEST_ASSERT(!(b<a));
  e.copy(a, 0, 3);
  TEST_ASSERT(!(a==e));
  TEST_ASSERT(!(a<e));
  TEST_ASSERT(e<a);
  e.free();
  TEST_ASSERT(!(a==e));
  TEST_ASSERT(!(a<e));
  TEST_ASSERT(e<a);

  // Test getMutableBuffer
  osc::byte_t *ebuf = e.getMutableBuffer(4);
  memmove(ebuf, bytes, sizeof(bytes));
  TEST_ASSERT(b==e);

  // Test truncate
  a = b;
  e.copy(a, 0, 3);
  b.truncate(3);
  TEST_ASSERT_EQUAL(b.size(), 3);
  TEST_ASSERT(e==b);

  // Test sha1 hashing
  b = a;
  a.getSha1Digest(c);
  b.getSha1Digest(d);
  TEST_ASSERT_EQUAL(c.size(), 20);
  TEST_ASSERT_EQUAL(d.size(), 20);
  TEST_ASSERT(c==d);
  a[0]++;
  a.getSha1Digest(c);
  b.getSha1Digest(d);
  TEST_ASSERT_EQUAL(c.size(), 20);
  TEST_ASSERT_EQUAL(d.size(), 20);
  TEST_ASSERT(!(c==d));

  // Test hashing (short buffer)
  a = b;
  osc::u32 aHash = a.getLookup2Hash();
  osc::u32 bHash = b.getLookup2Hash();
  TEST_ASSERT_EQUAL(aHash,bHash);
  a[3]++;
  aHash = a.getLookup2Hash();
  bHash = b.getLookup2Hash();
  TEST_ASSERT(aHash != bHash);
  aHash = a.getLookup2Hash(3);
  bHash = b.getLookup2Hash(3);
  TEST_ASSERT_EQUAL(aHash,bHash);
  aHash = a.getLookup2Hash(3);
  bHash = b.getLookup2Hash(2);
  TEST_ASSERT(aHash!=bHash);

  // Test hashing (long buffer)
  osc::byte_t longBytes[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
  a.copy(longBytes, sizeof(longBytes));
  b = a;
  aHash = a.getLookup2Hash();
  bHash = b.getLookup2Hash();
  TEST_ASSERT_EQUAL(aHash,bHash);
  a[3]++;
  aHash = a.getLookup2Hash();
  bHash = b.getLookup2Hash();
  TEST_ASSERT(aHash != bHash);
  a = b;
  a[14]++;
  aHash = a.getLookup2Hash();
  bHash = b.getLookup2Hash();
  TEST_ASSERT(aHash != bHash);
  aHash = a.getLookup2Hash(12);
  bHash = b.getLookup2Hash(12);
  TEST_ASSERT_EQUAL(aHash,bHash);
  aHash = a.getLookup2Hash(10);
  bHash = b.getLookup2Hash(9);
  TEST_ASSERT(aHash!=bHash);

  // Test in-place buffer (ownBytes = false)
  a.copy(bytes, sizeof(bytes));
  osc::Buffer g(bytes, sizeof(bytes), false);
  TEST_ASSERT_EQUAL(g.size(), sizeof(bytes));
  TEST_ASSERT_EQUAL_BUFFERS(g.data(), bytes, sizeof(bytes));
  TEST_ASSERT(a==g);
  TEST_ASSERT_EQUAL(g.data(), bytes);

  return true;
}

static bool BufferTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_Buffer,
                                     "test_osc_Buffer");
