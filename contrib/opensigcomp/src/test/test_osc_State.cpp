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
#include "Types.h"
#include "State.h"
#include "TestList.h"

osc::State * generateRandomState(size_t size)
{
  osc::Buffer buffer(0,size);
  for(size_t c = 0; c < size; c++)
  {
    buffer[c] = rand()%256;
  }
  return new osc::State(buffer);
}
bool test_osc_State()
{
  bool test = true;
  osc::State * state[10];
  for(osc::count_t c = 0; c < 10 ; c++)
  {
    state[c] = generateRandomState(rand()%768 + 245);
  }
  test = test && osc::print_subtest("Equivalence", state[0]->isEquivalentTo(*state[0]));

  test = test && osc::print_subtest( "Retain", (state[0]->retain() + 1) == (state[0]->retain()) );
  test = test && osc::print_subtest( "Handlerless Release", (state[0]->release()-1) == (state[0]->release()) );
  test = test && osc::print_subtest( "Over Release", (state[0]->release()) == (state[0]->release()) );
  test = test && osc::print_subtest( "StateId To State Matching", state[0]->matchesStateId(state[0]->getStateId()));
  for(osc::count_t d = 0; d < 10 ; d++)
  {
    delete state[d];
  }
  return test;
}

static bool StateTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_State,
                                     "test_osc_State");
