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

/**
  @file Compressor.cpp
  @brief Implementation of osc::Compressor class.
*/

#include "ProfileStack.h"
#include <assert.h>
#include "Compressor.h"

/**
  Constructor for osc::Compressor.

  @param stateHandler  Reference to state handler that this compressor
                       uses to store and retrieve compression-related
                       state. This must be the same StateHandler that
                       is used by the stack to which the compressor is
                       to be added.
 */
osc::Compressor::Compressor(osc::StateHandler &stateHandler)
  : m_stateHandler(stateHandler)
{
  DEBUG_STACK_FRAME;
}

/**
  Copy constructor for osc::Compressor.
 */
osc::Compressor::Compressor(Compressor const &r)
  : m_stateHandler(r.m_stateHandler)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}
 
/**
  Destructor for osc::Compressor.
 */
osc::Compressor::~Compressor()
{
  DEBUG_STACK_FRAME;
}

/**
  Assignment operator for osc::Compressor.
 */
osc::Compressor &
osc::Compressor::operator=(Compressor const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  /* Assign attributes */
  assert(0);
  return *this;
}

