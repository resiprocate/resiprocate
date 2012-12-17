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
  @file DeflateData.cpp
  @brief Implementation of osc::DeflateData class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "Libc.h"
#include "DeflateData.h"

/**
  Constructor for osc::DeflateData.
 */
osc::DeflateData::DeflateData()
  : m_nextState(-1 % osc::DeflateData::MAP_SIZE)
{
  DEBUG_STACK_FRAME;
  OSC_MEMSET(m_nackHash,0,sizeof(m_nackHash));
}

/**
  Copy constructor for osc::DeflateData.
 */
osc::DeflateData::DeflateData(DeflateData const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::DeflateData.
 */
osc::DeflateData::~DeflateData()
{
  DEBUG_STACK_FRAME;
}

/**
  Assignment operator for osc::DeflateData.
 */
osc::DeflateData &
osc::DeflateData::operator=(DeflateData const &r)
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

void
osc::DeflateData::storeCurrStateId(const osc::Buffer &id)
{
  DEBUG_STACK_FRAME;
  m_stateId[m_nextState] = id;
}

void
osc::DeflateData::storeCurrNackHash(const osc::sha1_t &hash)
{
  DEBUG_STACK_FRAME;
  m_nackHash[m_nextState] = (hash.digest[0] << 24) |
                            (hash.digest[1] << 16) |
                            (hash.digest[2] <<  8) |
                            (hash.digest[3]);

}

signed int
osc::DeflateData::findNackHash(const osc::sha1_t &hash)
{
  DEBUG_STACK_FRAME;
  osc::u32 key = (hash.digest[0] << 24) |
                 (hash.digest[1] << 16) |
                 (hash.digest[2] <<  8) |
                 (hash.digest[3]);


  if (key == 0)
  {
    return -1;
  }

  for (signed int i = 0; i < MAP_SIZE; i++)
  {
    if (key == m_nackHash[i])
    {
      m_nackHash[i] = 0;
      return i;
    }
  }
  return -1;
}

void
osc::DeflateData::incrementSerial()
{
  DEBUG_STACK_FRAME;
  m_nextState = ( m_nextState + 1 ) % MAP_SIZE;
}

const osc::Buffer&
osc::DeflateData::getStateId(osc::u8 serial) const
{
  DEBUG_STACK_FRAME;
  return m_stateId[serial % MAP_SIZE];
}

#ifdef DEBUG
void
osc::DeflateData::dump(std::ostream &os) const
{
  DEBUG_STACK_FRAME;
  os << "[DeflateData " << this << "]" << std::endl
     << "  nextState = " << static_cast<int>(m_nextState) << std::endl;
  for (int i = 0; i < MAP_SIZE; i++)
  {
     os << "  stateid [" << i << "] = " << m_stateId[i] << std::endl;
     os << "  nackhash[" << i << "] = " << m_nackHash[i] << std::endl;
  }
}
#endif
