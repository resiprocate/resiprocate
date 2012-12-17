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
#include "Compartment.h"
#include "StateHandler.h"
#include "osc_generators.h"
#include "TestList.h"
#include "osc_generators.h"
#include "Types.h"

bool test_osc_Compartment()
{
  bool test = true;
  osc::byte_t cId[3][3] = {{'1','2','3'},{'1','3','2'},{'1','2','3'}};
  osc::byte_t sData[10] = {'A','B','C','D','E','F','G','H','I','J'};
  osc::StateHandler handler;
  osc::compartment_id_t compartmentId[3];
  osc::Compartment * compartments[3];
  for(int x = 0; x < 3;x++)
  {
    compartmentId[x].copy(cId[x],3);
    compartments[x] = handler.getCompartment(compartmentId[x]);
  }
  
  osc::Compartment * compartment = handler.getCompartment(compartmentId[0]);
  test = test && osc::print_subtest("Compartment Creation", compartment !=0);

  test = test && osc::print_subtest("Compartment Retrieval",
      compartment == handler.getCompartment(compartmentId[0]));
  
  osc::Buffer stateData(sData, 10);
  
  osc::State * state = new osc::State(stateData);
  osc::state_id_t id = state->getId();

  // This routine holds onto the state for the duration of its
  // testing.


  state->retain();
  osc::nack_code_t ns;
  handler.addState(state,compartmentId[0],1);
  state = handler.getState(id, ns);

  osc::State *bigState = 0;
  if(compartment != 0)
  {
    test = test && osc::print_subtest("State Insertion",
        1 == compartment->numberOfStates());

    compartment->removeState(state->getStateId());
    test = test && osc::print_subtest("State Removal",
        compartment->numberOfStates() == 0);
  
    state = handler.getState(id, ns);
    handler.addState(state,compartmentId[0],1);
    test = test && osc::print_subtest("State Re-Insertion",
                      compartment->numberOfStates() == 1);
    
    state = handler.getState(id, ns);
    handler.addState(state,compartmentId[0],1);
    state = handler.getState(id, ns);

    size_t size = compartment->remainingStateMemory();
    if(size >= 65)
    {
      bigState = osc::generateRandomState(
                   compartment->totalStateMemoryAvailable() - 64,false);
      compartment->addState(bigState,1);
      test = test && osc::print_subtest("Inserting Single Large State", 
                                   0 == compartment->remainingStateMemory());
    }
  }
  else
  {
    osc::print_subtest("Multiple Tests Skipped Unable To Continue", false);
  }

  if (state)
  {  
    state->release();
  }
  if (bigState)
  {
    if (compartment)
    {
      compartment->removeState(bigState->getId());
    }
    delete bigState;
  }
  return (test);
}

static bool CompartmentTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_Compartment,
                                     "test_osc_Compartment");
