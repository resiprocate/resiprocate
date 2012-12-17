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
   Boston, MA 02110-1307 USA

   Unless your use of this code is goverened by a written and signed
   contract containing provisions to the contrary, this program is
   distributed WITHOUT ANY WARRANTY; without even the implied warranty
   of MERCHANTABILITY of FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

#include <assert.h>
#include <iostream>

#include "Sha1Hasher.h"

int
main(int argc, char **argv)
{
  if (argc != 2)
  {
    std::cout << "Usage: " << argv[0] << " <iterations>" << std::endl;
    exit(__LINE__);
  }
  int iterations = atoi(argv[1]);
  if (iterations <= 0)
  {
    std::cout << "Iterations must be a positive integer, not " << argv[1]
              << std::endl;
    exit(__LINE__);
  }

  osc::byte_t buffer[8192];

  for (unsigned int i = 0; i < sizeof(buffer); i++)
  {
    buffer[i] = i & 0xff;
  }

  osc::Sha1Hasher h;
  osc::byte_t hash[20];
  for (int i = 0; i < iterations; i++)
  {
    h.reset();
    h.addData(buffer,8192);
    h.getHash(hash);
  }

  return 0;
}
