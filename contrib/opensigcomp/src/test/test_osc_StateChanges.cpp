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
#include "StateChanges.h"
#include "TestList.h"
#include  "osc_generators.h"
bool test_osc_StateChanges()
{
  bool test = true;
  /////////////////////////////////////////////////////////////
  /// Tests adding four addition changes                    ///
  ////////                ...                               ///
  {
    osc::StateChanges changes;
    changes.addState(osc::generateRandomState(20,false));
    changes.addState(osc::generateRandomState(20,false));
    changes.addState(osc::generateRandomState(20,false));
    changes.addState(osc::generateRandomState(20,false));
    int adds = 0;
    int deletes = 0;
    int proto = 0;
    for (osc::u8 i = 0; i < changes.getNumOperations(); i++)
    {
      switch (changes.getOperationType(i))
      {
        case osc::StateChanges::ADD_STATE: adds++; break;
        case osc::StateChanges::REMOVE_STATE: deletes++; break;
        case osc::StateChanges::PROTO_STATE: proto++; break;
        default: TEST_ASSERT(0);
      }
    }
    test = test && osc::print_subtest("Filling StateChanges with add operations", deletes==0 && adds == 4);
  }
  /////////////////////////////////////////////////////////////
  /// Tests adding four deletion changes                    ///
  ////////                ...                               ///
  {
    osc::StateChanges changes;
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.deleteState(osc::generateNonRandomUniqueId());
    int adds = 0;
    int deletes = 0;
    int proto = 0;
    for (osc::u8 i = 0; i < changes.getNumOperations(); i++)
    {
      switch (changes.getOperationType(i))
      {
        case osc::StateChanges::ADD_STATE: adds++; break;
        case osc::StateChanges::REMOVE_STATE: deletes++; break;
        case osc::StateChanges::PROTO_STATE: proto++; break;
        default: TEST_ASSERT(0);
      }
    }
    test = test && osc::print_subtest("Filling StateChanges with delete operations", deletes==4 && adds == 0);
  } 
  /////////////////////////////////////////////////////////////
  /// Tests adding four delete and for add opperations      ///
  ////////                ...                               ///
  {
    osc::StateChanges changes;
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.deleteState(osc::generateNonRandomUniqueId());
    changes.addState(osc::generateRandomState(20,false));
    changes.addState(osc::generateRandomState(20,false));
    changes.addState(osc::generateRandomState(20,false));
    changes.addState(osc::generateRandomState(20,false));
    int adds = 0;
    int deletes = 0;
    int proto = 0;
    for (osc::u8 i = 0; i < changes.getNumOperations(); i++)
    {
      switch (changes.getOperationType(i))
      {
        case osc::StateChanges::ADD_STATE: adds++; break;
        case osc::StateChanges::REMOVE_STATE: deletes++; break;
        case osc::StateChanges::PROTO_STATE: proto++; break;
        default: TEST_ASSERT(0);
      }
    }
    test = test && osc::print_subtest("Filling StateChanges with delete and add operations", deletes==4 && adds == 4);
  } 

  
  return test;
}

static bool StateChangesTestInitStatus = 
  osc::TestList::instance()->addTest(test_osc_StateChanges,
                                     "test_osc_StateChanges");
