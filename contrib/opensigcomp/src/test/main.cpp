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

#include <iostream>
#include "TestList.h"

int main(int argc, char **argv)
{
  std::list<std::string> pattern;

  for (int i = 1; i < argc; i++)
  {
    pattern.push_back(argv[i]);
  }

  std::cout << std::endl << "Running unit (class) tests: " << std::endl;
  if (osc::TestList::instance()->runTests(pattern))
  {
    std::cout << std::endl << "All tests run successfully" << std::endl;
    return 0;
  }
  else
  {
    std::cout << std::endl << "Test failure" << std::endl;
    return -1;
  }
}
