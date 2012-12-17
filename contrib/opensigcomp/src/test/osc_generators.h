#ifndef _OSC_GENERATORS_H
#define _OSC_GENERATORS_H
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
*/
#include "Types.h"
#include "Compartment.h"
#include "State.h"

namespace osc
{
  osc::State * generateRandomState(size_t size, bool isBuiltIn=false);
  osc::compartment_id_t generateRandomCompartmentId(size_t size);
  osc::Buffer generateNonRandomUniqueId();
}

#endif

