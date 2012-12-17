/* ***********************************************************************
   Open SigComp -- Implementation of RFC 3320 Signaling Compression

   Copyright 2005 Estacado Systems, LLC

   Your use of this code is governed by the license under which it
   has been provided to you. Unless you have a written and signed
   document from Estacado Systems, LLC stating otherwise, your license
   is as provided by the GNU General Public License version 2, a copy
   of which is available in this project in the file named "LICENSE."
   Alternately, a copy of the licence is available by writing to
   the Free Software Foundation, Inc., 59 Temple Place, Suite 330,
   Boston, MA 021110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */


#include <iostream>
#include <stdio.h>
#include "Sha1Hasher.h"
#include "TestList.h"
#include "sha1_hash_vector.h"

bool test_osc_Sha1Hasher()
{
  /* 
    Fill in your test code here. It should return
    true on success, and false on failure. Returning
    false should generally print an error message on
    cout indicating what is wrong plus a filename
    and line number.
  */

  osc::byte_t expected_hash[] = 
  {
    0xA9, 0x99, 0x3E, 0x36,
    0x47, 0x06, 0x81, 0x6A,
    0xBA, 0x3E, 0x25, 0x71,
    0x78, 0x50, 0xC2, 0x6C,
    0x9C, 0xD0, 0xD8, 0x9D
  };

  osc::Sha1Hasher hasher;
  osc::byte_t hash[20];
  hasher.addData((osc::byte_t *)"abc",3);
  hasher.getHash(hash);
  int i,j;

  for (i = 0; i < 20; i++)
  {
    if (hash[i] != expected_hash[i])
    {
      printf("%s:%d: Byte %d is %2.2X; should be %2.2X\n",
              __FILE__, __LINE__, i, hash[i] & 0xFF, expected_hash[i] & 0xFF);
      return false;
    }
  }

  // Run through the official test vectors

  char buffer[15000];
  size_t bufferLength;
  int repeat;
  char value;

  for (i = 0; i < 229; i++)
  {
    hasher.reset();
    bufferLength = 0;

    for (j = 0; j < sha1Vector[i].length; j += 2)
    {
      repeat = sha1Vector[i].message[j];
      value = sha1Vector[i].message[j+1];
      memset(buffer + bufferLength, value, repeat);
      bufferLength += repeat;
    }
    hasher.addData((osc::byte_t*)buffer, bufferLength);

    hasher.getHash(hash);

    for (j = 0; j < 20; j++)
    {
      if (hash[j] != (osc::byte_t)(sha1Vector[i].hash)[j])
      {
        printf("%s:%d: Message %d, byte %d is %2.2X; should be %2.2X\n",
                __FILE__, __LINE__, i, j, 
                hash[i] & 0xFF, 
                sha1Vector[i].hash[j] & 0xFF);
        return false;
      }
    }
  }
  return true;
}

static bool Sha1HasherTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_Sha1Hasher,
                                     "test_osc_Sha1Hasher");
