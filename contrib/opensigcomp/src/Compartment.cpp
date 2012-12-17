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
  @file Compartment.cpp
  @brief Implementation of osc::Compartment class.
*/

#include "ProfileStack.h"
#include <assert.h>
#include "Compartment.h"
#include "StateHandler.h"
#include "CompressorData.h"
#include "State.h"
#include "Libc.h"

#ifdef LEAK_DEBUG
DEBUG_LEAK_BODY_HOOK(Compartment)
#endif

#ifdef DEBUG
#include <iomanip>
#endif

/**
  Creates an empty compartment and sets the compartmentId to the
  one provided.

  @param compartmentId the compartment Id for this compartment

  @todo The cpbDmsSms parameter should be set according to
        whether we're using SIP. For SIP applications, it
        is safe to assume 8192 bytes of DMS on the remote
        end. For other applications, we should default to
        assuming 2048. Because the Deflate compressor doesn't
        really work with only 2048 bytes of DMS on the remote
        end, hardcoding this to act like we're using SIP is
        quite sensible.
*/

osc::Compartment::Compartment(osc::compartment_id_t const &compartmentId,
                              size_t maxStateSpace,
                              osc::u8  assumeCpbDmsSms,
                              osc::u16 assumeVersion):
    m_localStates(maxStateSpace),
    m_remoteStates(osc::Compartment::getSmsFromByte(assumeCpbDmsSms)),
    m_retainCount(1),
    m_nackRetainCount(0),
    m_countObserver(0),
    m_data(0),
    m_dataTop(0),
    m_dataCount(0),
    m_compartmentId(compartmentId),
    m_stale(true),
    m_zombie(false),
    m_cpbDmsSms(assumeCpbDmsSms),
    m_version(assumeVersion),
    m_numRemoteAdvertizedStates(0),
    m_iBit(false),
    m_reliableTransport(false)

{
  DEBUG_STACK_FRAME;
  m_lookup2Hash = compartmentId.getLookup2Hash();
}

osc::u32 osc::Compartment::getLookup2Hash()
{
  DEBUG_STACK_FRAME;
  return m_lookup2Hash;
}

/**
  @returns in units of bytes the anount of state space that
  this compartment can use
*/
size_t osc::Compartment::totalStateMemoryAvailable()
{
  DEBUG_STACK_FRAME;
  return m_localStates.getQuota();
}

/**
  Destructor for osc::Compartment.
 */
osc::Compartment::~Compartment()
{
  DEBUG_STACK_FRAME;
  for (size_t i = 0; i < m_dataCount; i++)
  {
    delete m_data[i];
  }
  delete[] m_data;
}

/**
  This method adds the state to the list of associated states that this
  compartment is aware of.  If the state is already associated with this
  compartment it has no effect (unless the priority has changed).

  @param state    The state to be added to the compartment.

  @param priority The SigComp retention priority for this state
                  within the compartment
*/
void
osc::Compartment::addState(osc::State *state, osc::u16 priority)
{
  DEBUG_STACK_FRAME;
  m_localStates.addState(state, priority, true);
}

/**
  Will disassociate a state from this compartment if stateId matches
  exactly one associated state.

  @param stateId the id of the state to remove
*/

void osc::Compartment::removeState(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  m_localStates.removeState(stateId);
}

/**
  This method adds the state to the list of remote states that we
  expect the other SigComp stack to know about.

  @param state    The state to be added to the compartment.

  @param priority The SigComp retention priority for this state
                  within the compartment

  @param acked    Indicates whether the state is to be considered
                  acknowledged by the remote endpoint.
*/
void
osc::Compartment::addRemoteState(osc::State *state, osc::u16 priority,
                                 bool acked)
{
  DEBUG_STACK_FRAME;
  m_remoteStates.addState(state, priority, acked);
}

/**
  Will disassociate a state from the list of remote states that we expect
  the other SigComp stack to know about if stateId matches exactly one
  associated state.

  @param stateId the id of the state to remove
*/

void osc::Compartment::removeRemoteState(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  m_remoteStates.removeState(stateId);
}

/**
  Mark the specified remote state to indicate that the far
  side has acknowledged its receipt.
*/
void
osc::Compartment::ackRemoteState(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  m_remoteStates.ackState(stateId);
}

/**
  Check the specified remote state to verify that the far
  side has acknowledged its receipt.
*/
bool
osc::Compartment::isRemoteStateAcked(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  return m_remoteStates.isStateAcked(stateId);
}

/**
  Adjusts our idea of how much state memory is available in the
  remote endpoint.
*/
void
osc::Compartment::setRemoteStateMemorySize(size_t quota)
{
  DEBUG_STACK_FRAME;
  m_remoteStates.setQuota(quota);
}

/**
  Returns the most recent acknowledged state at a
  given priority.

  @note Callers of this method must call release() on 
        the returned state when they are done with it.
*/
osc::State *
osc::Compartment::getMostRecentAckedState(osc::u16 priority)
{
  DEBUG_STACK_FRAME;
  return m_remoteStates.getMostRecentAckedState(priority);
}

/**
  Removes all states we expect the remote endpoint to know about
*/
void
osc::Compartment::resetRemoteStates()
{
  DEBUG_STACK_FRAME;
  m_remoteStates.reset();
}

/**
  Decrements the retain count for this compartment, removing it
  from the StateHandler map, if appropriate.

  @note This method may deallocate this Compartment.
*/
osc::count_t osc::Compartment::release()
{
  DEBUG_STACK_FRAME;

  if (m_countObserver)
  {
    return m_countObserver->releaseCompartment(this);
  }
  else
  {
    osc::count_t count = decrementRetainCount();
    if (m_zombie)
    {
      delete this;
    }
    return count;
  }

  // This should never execute.
  return 0;
}

/**
  @returns Current retain count (after decrementing)

  @note This method is only safe to call from the state handler
        with proper locks invoked. Everyone else should use
        the release() method instead.
*/
osc::count_t
osc::Compartment::decrementRetainCount()
{
  DEBUG_STACK_FRAME;

  if (m_retainCount > 0) 
  {
    m_retainCount--;
  }

  // If there are no longer any references, dispose of the
  // compartment.
  if (m_retainCount == 0 && m_nackRetainCount == 0)
  {
    m_zombie = true;
  }

  return m_retainCount;
}

/**
  Retains the compartment for the purposes of the NackMap so that it won't
  be deleted elsewhere.

  @note This method is only safe to call from the state handler
        with proper locks invoked. Everyone else should call
        StateHandler::getCompartment().

  @returns Current NACK retain count (after incrementing)
*/
 
osc::count_t osc::Compartment::nackRetain()
{
  DEBUG_STACK_FRAME;
  return ++m_nackRetainCount;
}

/**
  Decrements the NACK retain count for this compartment.

  @note This method may deallocate this Compartment.
*/
osc::count_t osc::Compartment::nackRelease()
{
  DEBUG_STACK_FRAME;
  if (m_countObserver)
  {
    return m_countObserver->releaseCompartmentNack(this);
  }
  else
  {
    osc::count_t count = decrementNackRetainCount();
    if (m_zombie)
    {
      delete this;
    }
    return count;
  }

  // This should never execute.
  return 0;
}

/**
  Decrements the NACK retain count, marking the compartment
  as a zombie if it no longer needs to exist.

  @note This method is only safe to call from the state handler
        with proper locks invoked. Everyone else should use
        the nackRelease() method instead.

  @returns Current NACK retain count (after decrementing)
*/
osc::count_t
osc::Compartment::decrementNackRetainCount()
{
  DEBUG_STACK_FRAME;

  if (m_nackRetainCount > 0) 
  {
    m_nackRetainCount--;
  }

  // If there are no longer any references, dispose of the
  // compartment.
  if (m_retainCount == 0 && m_nackRetainCount == 0)
  {
    m_zombie = true;
  }

  return m_nackRetainCount;
}


/**
  Retains the compartment so that it won't be deleted elsewhere.

  @returns Current retain count (after incrementing)

  @note This method is only safe to call from the state handler
        with proper locks invoked. Everyone else should call
        StateHandler::getCompartment().
*/
 
osc::count_t osc::Compartment::retain()
{
  DEBUG_STACK_FRAME;

  setStale(false);
  return ++m_retainCount;
}

/**
  @returns in units of bytes the anount of unused state space
  for this compartment
 */
size_t osc::Compartment::remainingStateMemory()
{
  DEBUG_STACK_FRAME;
  return m_localStates.getQuotaLeft();
}

/**
  @returns the CompartmentId of the compartment
*/
 
osc::compartment_id_t osc::Compartment::getCompartmentId() const
{
  DEBUG_STACK_FRAME;
    return m_compartmentId;
}

/*
  Returns the current retain count for this compartment
*/
osc::count_t osc::Compartment::getRetainCount() const
{
  DEBUG_STACK_FRAME;
  return m_retainCount;
}

/**
  @param observer the StateHandler that will be handling this compartment
*/
void osc::Compartment::setCountObserver(osc::StateHandler * observer)
{
  DEBUG_STACK_FRAME;
  m_countObserver = observer;
}

/**
  @param data the CompressorData to add the compartment
  Adds CompressorData to the compartment

  @retval true Insertion is successful
  @retval false Insertion failed due to e.g. an out-of-memory condition
*/
bool osc::Compartment::addCompressorData(osc::CompressorData *data)
{
  DEBUG_STACK_FRAME;
  if(m_dataCount == m_dataTop)
  {
    size_t newDataTop = m_dataTop * 2;

    // Bootstrap: if we were previously 0, grow to 2.
    if (newDataTop == 0)
    {
      newDataTop = 2;
    }

    osc::CompressorData ** newData = new CompressorData * [newDataTop];
    if (!newData)
    {
      return false;
    }

    if (m_data)
    {
      OSC_MEMMOVE( newData, m_data, 
                   m_dataTop * sizeof( osc::CompressorData * ) );
      delete[] m_data;
    }

    m_dataTop = newDataTop;
    m_data = newData;
  }

  m_data[m_dataCount] = data;
  m_dataCount++;

  return true;
}

/**
  @returns a list of CompressorData associated with the compartment
*/    
osc::CompressorData *
osc::Compartment::getCompressorData(compressor_id_t compressorId)
{
  DEBUG_STACK_FRAME;
  size_t p = m_dataCount;
  while(p > 0)
  {
    p--;
    if( m_data[p]->getDataType() == compressorId )
    {
      return m_data[p];
    }
  }
  return 0;
}

void
osc::Compartment::setCpbDmsSms(osc::u8 cpbDmsSms)
{
  DEBUG_STACK_FRAME;
  m_cpbDmsSms = cpbDmsSms;
  m_remoteStates.setQuota(getRemoteStateMemorySize());
}

osc::Compartment &
osc::Compartment::operator=(osc::Compartment const &r)
{
  DEBUG_STACK_FRAME;
  if (&r == this)
  {
    return *this;
  }
  assert(0);
  return *this;
} 

osc::u8
osc::Compartment::makeCpbDmsSms(unsigned int cpb,
                                unsigned int dms,
                                unsigned int sms)
{
  DEBUG_STACK_FRAME;
  osc::byte_t result = 0;
  unsigned int i;

  i = cpb;

  if (i >= 32)
  {
    if (i < 64)
    {
      result |= 0x40;
    }
    else if (i < 128)
    {
      result |= 0x80;
    }
    else
    {
      result |= 0xC0;
    }
  }

  i = dms;
  if (i >= 2048)
  {
    if (i < 4096)
    {
      result |= 0x08;
    }
    else if (i < 8192)
    {
      result |= 0x10;
    }
    else if (i < 16384)
    {
      result |= 0x18;
    }
    else if (i < 32768)
    {
      result |= 0x20;
    }
    else if (i < 65536)
    {
      result |= 0x28;
    }
    else if (i < 131072)
    {
      result |= 0x30;
    }
    else
    {
      result |= 0x38;
    }
  }

  i = sms;
  if (i >= 2048)
  {
    if (i < 4096)
    {
      result |= 0x01;
    }
    else if (i < 8192)
    {
      result |= 0x02;
    }
    else if (i < 16384)
    {
      result |= 0x03;
    }
    else if (i < 32768)
    {
      result |= 0x04;
    }
    else if (i < 65536)
    {
      result |= 0x05;
    }
    else if (i < 131072)
    {
      result |= 0x06;
    }
    else
    {
      result |= 0x07;
    }
  }

  return result;
}

#ifdef DEBUG
void
osc::Compartment::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  os << std::setw(indent) << ""
     << "[Compartment " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  Compartment ID = " << m_compartmentId << std::endl
     << std::setw(indent) << ""
     << "  Retain Count   = " << m_retainCount << std::endl;

  os << std::setw(indent) << "" 
     << "  Remote Advertized States: " << std::endl;
  for (size_t i = 0; i < m_numRemoteAdvertizedStates; i++)
  {
    os << std::setw(indent) << "" 
       << "    [" << i << "]: " 
       << m_remoteAdvertizedState[i]
       << std::endl;
  }

  os << std::setw(indent) << "" 
     << "  Local States: " << std::endl;
  m_localStates.dump(os, indent + 4);

  os << std::setw(indent) 
     << "" << "  Remote States: " << std::endl;
  m_remoteStates.dump(os, indent + 4);
}

std::ostream &
osc::operator<< (std::ostream &os, const osc::Compartment &b)
{
  DEBUG_STACK_FRAME;
  b.dump(os);
  return os;
}
#endif
