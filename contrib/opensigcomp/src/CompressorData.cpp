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
  @file CompressorData.cpp
  @brief Implementation of osc::CompressorData class.
*/


 
#include "ProfileStack.h"
#include <assert.h>
#include "CompressorData.h"

/**
  Constructor for osc::CompressorData.
 */
osc::CompressorData::CompressorData()
{
  DEBUG_STACK_FRAME;
}

/**
  Copy constructor for osc::CompressorData.
 */
osc::CompressorData::CompressorData(CompressorData const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::CompressorData.
 */
osc::CompressorData::~CompressorData()
{
  DEBUG_STACK_FRAME;
}

/**
  Assignment operator for osc::CompressorData.
 */
osc::CompressorData &
osc::CompressorData::operator=(CompressorData const &r)
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

#ifdef DEBUG
std::ostream &
osc::operator<< (std::ostream &os, const osc::CompressorData &cd)
{
  DEBUG_STACK_FRAME;
  cd.dump(os);
  return os;
}
#endif
