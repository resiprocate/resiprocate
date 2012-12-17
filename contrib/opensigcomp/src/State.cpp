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
  @file State.cpp
  @brief Implementation of osc::State class.
*/


#include "ProfileStack.h"
#include "State.h"
#include "StateHandler.h"
#include "Sha1Hasher.h"

#ifdef DEBUG
#include <iomanip>
#endif

osc::State::State(osc::Buffer const &stateData, 
                  osc::u16 address,
                  osc::u16 instruction,
                  osc::u16 mal):
  m_stateId(m_stateIdArray,20,false),
  m_address(address), m_pc(instruction), m_minimumAccessLength(mal),
  m_retainCounter(0),m_countObserver(0)
{
  DEBUG_STACK_FRAME;
  m_buffer.copy(stateData);
  setStateId();
}

osc::State::State(osc::u16 address,
                  osc::u16 instruction,
                  osc::u16 mal):
  m_stateId(m_stateIdArray,20,false),
  m_address(address), m_pc(instruction), m_minimumAccessLength(mal),
  m_retainCounter(0),m_countObserver(0)
{
  DEBUG_STACK_FRAME;
  OSC_MEMSET(m_stateIdArray,0,sizeof(m_stateIdArray));
}

/**
  Special protected constructor for static states.

  States constructed via this constructor do not copy and do not
  own the memory indicated by the stateStart pointer. The memory
  indicated by stateState must outlive this State instance.

  @see SipDictionary
*/
osc::State::State(const osc::byte_t *stateStart,
                  size_t stateSize,
                  osc::u16 address,
                  osc::u16 instruction,
                  osc::u16 mal) :
  m_buffer(stateStart, stateSize, false),
  m_stateId(m_stateIdArray,20,false),
  m_address(address),
  m_pc(instruction),
  m_minimumAccessLength(mal),
  m_retainCounter(0),m_countObserver(0)
{
  DEBUG_STACK_FRAME;
  setStateId();
}


osc::byte_t *
osc::State::getMutableBuffer(size_t s)
{
  DEBUG_STACK_FRAME;
  return m_buffer.getMutableBuffer(s);
}

void
osc::State::finalizeMutableBuffer()
{
  DEBUG_STACK_FRAME;
  setStateId();
}

/**
  @todo If the hash buffer allocation fails,
        the state should be marked as invalid.
        Calls that can get this far should be
        forced to check for valdity. Alternately,
        stateid can be changed to an array member.
*/
void osc::State::setStateId()
{
  DEBUG_STACK_FRAME;
  /*
    RFC 3320 SS 9.4.9:

      The UDVM... calculates a 20-byte SHA-1 hash [RFC-3174] over the
      byte string formed by concatenating the state_length, state_address,
      state_instruction, minimum_access_length and state_value (in the
      order given).  This is the state_identifier.
  */

  osc::Sha1Hasher hasher;
  hasher.addData(m_buffer.size());       // state_length
  hasher.addData(m_address);             // state_address
  hasher.addData(m_pc);                  // state_instruction
  hasher.addData(m_minimumAccessLength); // minimum_access_length
  hasher.addData(m_buffer.data(), m_buffer.size());

  hasher.getHash(m_stateIdArray);
}

void
osc::State::truncate(size_t newSize)
{
  DEBUG_STACK_FRAME;
  m_buffer.truncate(newSize);
  setStateId();
}

bool osc::State::isEquivalentTo(osc::State &state)
{
  DEBUG_STACK_FRAME;
  return m_stateId == state.m_stateId;
}

size_t osc::State::getStateSize() const
{
  DEBUG_STACK_FRAME;
  return m_buffer.size();
}

/**
  Decrements the usage count for this state, removing it
  from the StateHandler map, if appropriate.
  
  @note This method may deallocate this Compartment.
*/  


osc::count_t osc::State::release()
{
  if (m_countObserver)
  {
    return m_countObserver->releaseState(this);
  }
  else
  {
    int usageCount = decrementUsageCount();
    return usageCount;
  }
}

/**
  @returns Current usage count (after decrementing)

  @note This method is only safe to call from the state handler
        with proper locks invoked. Everyone else should use
        the release() method instead.
*/
osc::count_t osc::State::decrementUsageCount()
{
  DEBUG_STACK_FRAME;
  if(m_retainCounter > 0)
  {
    --m_retainCounter;
  }
  return m_retainCounter;
}

/**
  Retains the state so that it won't be deleted elsewhere.

  @returns Current retain count (after incrementing)
*/

osc::count_t osc::State::retain()
{
  DEBUG_STACK_FRAME;
  if (m_countObserver)
  {
    m_countObserver->retainState(this);
  }
  else
  {
    incrementUsageCount();
  }
  return m_retainCounter;
}

/**
  @returns Current usage count (after incrementing)

  @note This method is only safe to call from the state handler
        with proper locks invoked. Everyone else should use
        the retain() method instead.
*/
osc::count_t osc::State::incrementUsageCount()
{
  DEBUG_STACK_FRAME;
  return ++m_retainCounter;
}

void osc::State::setCountObserver(osc::StateHandler * observer)
{
  DEBUG_STACK_FRAME;
  m_countObserver = observer;
}

bool osc::State::matchesStateId(Buffer const &stateId)
{
  DEBUG_STACK_FRAME;
  for (size_t i = 0; i < stateId.size(); i++)
  {
    if (stateId[i] != m_stateId[i])
    {
      return false;
    }
  }
  return true;
}

const osc::Buffer & osc::State::getStateId() const
{
  DEBUG_STACK_FRAME;
  return m_stateId;
}

/**
 returns a reference to the state data.
 */
osc::Buffer &
osc::State::getStateData()
{
  DEBUG_STACK_FRAME;
  return m_buffer;
}

const osc::byte_t *
osc::State::getStateDataRawBuffer() const
{
  DEBUG_STACK_FRAME;
  return m_buffer.data();
}

osc::State::~State()
{
  DEBUG_STACK_FRAME;
}

osc::u16
osc::State::getWord(osc::u16 address)
{
  DEBUG_STACK_FRAME;
  if (address < m_address)
  {
    return 0;
  }

  address -= m_address;

  if (static_cast<size_t>(address + 1) > m_buffer.size())
  {
    return 0;
  }

  return (m_buffer[address] << 8) | m_buffer[address+1];
}

#ifdef DEBUG
void
osc::State::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  os << std::setw(indent) << ""
     << "[State " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  State ID     = " << m_stateId << std::endl
     << std::setw(indent) << ""
     << "  Length       = " << m_buffer.size() << std::endl
     << std::setw(indent) << ""
     << "  Address      = " << m_address << std::endl
     << std::setw(indent) << ""
     << "  Instruction  = " << m_pc << std::endl
     << std::setw(indent) << ""
     << "  Min. Access  = " << m_minimumAccessLength << std::endl
     << std::setw(indent) << ""
     << "  Refcount     = " << m_retainCounter << std::endl;
}

void
osc::State::diff(std::ostream &os, osc::State &s) const
{
  DEBUG_STACK_FRAME;
  os << "Diffing state " << std::endl 
     << "  " << m_stateId << " with "  << std::endl
     << "  " << s.m_stateId << std::endl;

  if (m_address != s.m_address)
  {
    os << "Address differs: " << m_address << " != " << s.m_address
       << std::endl;
  }

  if (m_pc != s.m_pc)
  {
    os << "PC differs: " << m_pc << " != " << s.m_pc << std::endl;
  }

  if (m_minimumAccessLength != s.m_minimumAccessLength)
  {
    os << "Min Access Length differs: " << m_minimumAccessLength << " != " 
       << s.m_minimumAccessLength << std::endl;
  }

  if (m_buffer.size() != s.m_buffer.size())
  {
    os << "Size differs: " << m_buffer.size() << " != " << s.m_buffer.size()
       << std::endl;
  }
  else
  {
    for (size_t i = 0; i < m_buffer.size(); i++)
    {
      if (m_buffer[i] != s.m_buffer[i])
      {
        os << "  Byte " << std::setw(4) << i 
           << " [addr " << std::setw(4) << (i + m_address) << "/0x";
        os.fill('0');
        os << std::hex << std::setw(4) << (i + m_address) << "]" 
           << " differs: " 
           << std::setw(2) << static_cast<int>(m_buffer[i]);
        if (m_buffer[i] >= 0x20 && m_buffer[i] <= 0x7e)
        {
          os << " '" << m_buffer[i] << "'";
        }
        else
        {
          os << "    ";
        }
        os << " != "  
           << std::setw(2) << static_cast<int>(s.m_buffer[i]);

        if (s.m_buffer[i] >= 0x20 && s.m_buffer[i] <= 0x7e)
        {
          os << " '" << s.m_buffer[i] << "'";
        }

        os << std::dec << std::endl;
        os.fill(' ');
      }
    }
  }


}

std::ostream &
osc::operator<< (std::ostream &os, const osc::State &s)
{
  DEBUG_STACK_FRAME;
  s.dump(os);
  return os;
}
#endif
