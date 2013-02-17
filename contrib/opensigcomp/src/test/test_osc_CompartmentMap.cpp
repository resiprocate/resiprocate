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
#include "CompartmentMap.h"
#include "SigcompMessage.h"
#include "TestList.h"

#define NUM_COMPARTMENTS 1200

bool test_osc_CompartmentMap()
{
  osc::CompartmentMap cm(8192);

  TEST_ASSERT_EQUAL(cm.getCompartmentCount(),0);

  osc::Compartment *c[NUM_COMPARTMENTS] = {0};
  size_t i, j;

  // Make sure each ID generates its own unique compartment
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    c[i] = cm.getCompartment(i);

    TEST_ASSERT(c[i]);

    for (j = 0; j < i; j++)
    {
      TEST_ASSERT(c[i] != c[j]);
    }

    TEST_ASSERT_EQUAL(cm.getCompartmentCount(),i+1);
    TEST_ASSERT_EQUAL(c[i]->remainingStateMemory(),8192);
    TEST_ASSERT_EQUAL(c[i]->getRetainCount(),1);
  }

  // Verify the compartments can be retreived
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    TEST_ASSERT_EQUAL(c[i],cm.getCompartment(i));
  }

  // Retain each compartment to make it non-stale
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    c[i]->retain();
    TEST_ASSERT_EQUAL(c[i]->getRetainCount(),2);
    c[i]->release();
    TEST_ASSERT_EQUAL(c[i]->getRetainCount(),1);
  }

  // Nothing should be stale during this first pass.
  osc::compartment_node_t *stale;
  osc::compartment_node_t *tempStale;
  stale = cm.removeStaleCompartments();
  TEST_ASSERT_EQUAL(cm.getCompartmentCount(),NUM_COMPARTMENTS);
  TEST_ASSERT_EQUAL(stale, 0);
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    TEST_ASSERT_EQUAL(c[i],cm.getCompartment(i));
  }

  // Everything should be stale this second time around.
  stale = cm.removeStaleCompartments();
  TEST_ASSERT_EQUAL(cm.getCompartmentCount(),0);

  // The releases in this loop will deallocate the compartments.
  while (stale)
  {
    int retainCount;
    TEST_ASSERT_EQUAL(stale->compartment->getRetainCount(),1);
    retainCount = stale->compartment->release();
    TEST_ASSERT_EQUAL(retainCount, 0);
    tempStale = stale;
    stale = stale->next;
    delete(tempStale);
  }

  // Get some new compartments
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    c[i] = cm.getCompartment(i);
    TEST_ASSERT(c[i]);
  }

  // Retain BUT DON'T RELEASE each compartment.
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    c[i]->retain();
    TEST_ASSERT_EQUAL(c[i]->getRetainCount(),2);
  }

  stale = cm.removeStaleCompartments();
  TEST_ASSERT_EQUAL(stale, 0);
  
  // Retain every even-numbered compartment to make it non-stale
  for (i = 0; i < NUM_COMPARTMENTS; i+=2)
  {
    c[i]->retain();
    TEST_ASSERT_EQUAL(c[i]->getRetainCount(),3);
    c[i]->release();
    TEST_ASSERT_EQUAL(c[i]->getRetainCount(),2);
  }

  // Now collect the stale compartments
  stale = cm.removeStaleCompartments();
  TEST_ASSERT_EQUAL(cm.getCompartmentCount(),NUM_COMPARTMENTS/2);

  while (stale)
  {
    stale->compartment->release();
    tempStale = stale;
    stale = stale->next;
    delete(tempStale);
  }

  // Verify that the odd number compartments are gone and the
  // even number ones remain. We also check removal and recreation
  // of compartments.
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    if (i % 2)
    {
      TEST_ASSERT_EQUAL(c[i]->getRetainCount(),1);
      osc::Compartment *tempComp1 = cm.getCompartment(i);
      TEST_ASSERT(tempComp1 != c[i]);
      cm.removeCompartment(tempComp1->getCompartmentId());
      osc::Compartment *tempComp2 = cm.getCompartment(i);
      TEST_ASSERT(tempComp2 != c[i]);
      TEST_ASSERT(tempComp2 != tempComp1);
      cm.removeCompartment(tempComp2->getCompartmentId());
      delete(tempComp1);
      delete(tempComp2);
    }
    else
    {
      TEST_ASSERT_EQUAL(c[i]->getRetainCount(),2);
      TEST_ASSERT_EQUAL(c[i],cm.getCompartment(i));
    }
  }

  // Remove everything else from the map
  stale = cm.removeStaleCompartments();
  TEST_ASSERT_EQUAL(cm.getCompartmentCount(),0);

  // Delete the stale list (but not the compartments; we do that later)
  while (stale)
  {
    tempStale = stale;
    stale = stale->next;
    delete(tempStale);
  }

  // Finally, deallocate our compartments.
  for (i = 0; i < NUM_COMPARTMENTS; i++)
  {
    delete(c[i]);
  }

  return true;
}

static bool CompartmentTestInitStatus = 
osc::TestList::instance()->addTest(test_osc_CompartmentMap,
                                   "test_osc_CompartmentMap");

