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
#include "osc_generators.h"
#include "Types.h"
#include "Compartment.h"
#include "State.h"
#include "Buffer.h"

/**
  @note It is the caller's responsibility to delete the returned state
*/
osc::State * osc::generateRandomState(size_t size, bool isBuiltIn)
{
  osc::Buffer buffer(0,size);
  for(size_t c = 0; c < size; c++)
  {
    buffer[c] = rand()%256;
  }
  return new osc::State(buffer, isBuiltIn);
}

osc::compartment_id_t osc::generateRandomCompartmentId(size_t size)
{
  osc::Buffer buffer(0,size);
  for(size_t c = 0; c < size; c++)
  {
    buffer[c] = rand() % 256;
  }
  return buffer;
}

osc::Buffer osc::generateNonRandomUniqueId()
{
  static int gronk = 0xbeeff00d;
  osc::Buffer buffer ((osc::byte_t*)(&gronk),sizeof(int*)); 
  gronk+=4;
  return buffer;
}

