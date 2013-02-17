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
#include "StateHandler.h"
#include "SigcompMessage.h"
#include "TestList.h"
#include "osc_generators.h"
#include "NackCodes.h"



bool test_osc_StateHandler()
{
  bool test = true;
    ////////////// State Handler Creation //////////////////
  osc::StateHandler * handler = new osc::StateHandler();
  osc::State * state =  osc::generateRandomState(1024);
  test = test && osc::print_subtest("Handler Creation", handler != 0);
  ////////////// Adding State To Handler /////////////////
  handler->addState(state);
  test = test && osc::print_subtest("Adding State", handler->numberOfStates()==1);
  ////////////// Removing State From Handler /////////////
  state->release();
  test = test && osc::print_subtest("Releasing State", handler->numberOfStates()==0);
  ////////////// Adding Multiple States To Handler ///////
  state =  osc::generateRandomState(100);
  handler->addState(state);
  for(int x =0 ; x < 90;x++)
  {
    handler->addState(osc::generateRandomState(1000));
  }
  test = test && osc::print_subtest("Adding Multiple States:", handler->numberOfStates()==91);
  ////////////// Single Release With Multiple States //////
  state =  osc::generateRandomState(100);
  handler->addState(state);
  state->release();

  test = test && osc::print_subtest("Single Release With Multiple States", handler->numberOfStates() == 91);
  delete handler;
  /////////////// Adding Compartment //////////////////////
  handler = new osc::StateHandler();

  test = test && osc::print_subtest("Adding Compartment",0!=handler->getCompartment(osc::generateRandomCompartmentId(4)));
  /////////////// Adding Multiple Compartments ////////////
  handler->getCompartment(osc::generateRandomCompartmentId(4));
  test = test && osc::print_subtest("Adding Multiple Compartments",2 == handler->numberOfCompartments());

  /////////////// Adding State To Compartment /////////////
  osc::compartment_id_t compartmentId = osc::generateRandomCompartmentId(4);
  osc::Compartment * compartment;
  compartment = handler->getCompartment(compartmentId);
  handler->addState(osc::generateRandomState(100,false),compartmentId,1);
  test = test && osc::print_subtest("Adding State To Managed Compartment", 1 == compartment->numberOfStates());

  // Release compartment (now owned only by handler)
  TEST_ASSERT_EQUAL(compartment->getRetainCount(),2);
  compartment->release();
  TEST_ASSERT_EQUAL(compartment->getRetainCount(),1);

  delete handler;

  ////////////// Aversion of non-stale compartment removal ////////
  handler = new osc::StateHandler();
  compartmentId = osc::generateRandomCompartmentId(4);
  compartment = handler->getCompartment(compartmentId);
  TEST_ASSERT_EQUAL(compartment->getRetainCount(),2);
  compartment->release();
  TEST_ASSERT_EQUAL(compartment->getRetainCount(),1);
  handler->removeStaleCompartments();
  TEST_ASSERT_EQUAL(handler->numberOfCompartments(),1);
  test = test && osc::print_subtest("Nonstale Compartment Removal Aversion", 1 == handler->numberOfCompartments());

  /////////////// Stale Compartment Removal ///////////////
  handler->removeStaleCompartments();
  test = test && osc::print_subtest("Stale Compartment Removal ", 0 == handler->numberOfCompartments());
  delete handler;
  return test; 
  
}

static bool StateHandlerTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_StateHandler,
                                     "test_osc_StateHandler");
