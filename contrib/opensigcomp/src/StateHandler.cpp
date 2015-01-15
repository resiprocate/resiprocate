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
  @file StateHandler.cpp
  @brief Implementation of osc::StateHandler class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "State.h"
#include "StateHandler.h"
#include "Types.h"
#include "StateChanges.h"
#include "NackMap.h"
#include "SipDictionary.h"
#include "Compartment.h"

/**
  Creates a StateHandler with a defined maximum state size and a
  maximum cycles per bit

  @param maxStateSpaceInBytes           The state_memory_size parameter as
                                        defined in RFC 3320.

  @param maximumCyclesPerBit            The cycles_per_bit parameter as
                                        defined in RFC 3320.

  @param maxDecompressionSpaceInBytes   The decompression_memory_size parameter
                                        as defined in RFC 3320.

  @param sigcompVersion                 The version of SigComp supported.
                                        In general, 0x02 if NACKs are
                                        to be used; 0x01 otherwise. See
                                        RFC 3320 and RFC 4077 for details.

  @param nackThreshold                  Number of messages to be retained
                                        per compartment for the purposes
                                        of NACK message correlation.
                                        This number represents a maximum
                                        average, and is not enforced on a
                                        per-compartment basis.

  @param assumeRemoteSms                The state_memory_size parameter to
                                        assume that a remote endpoint has
                                        unless and until we are informed
                                        otherwise.

  @param assumeRemoteCpb                The cycles_per_bit parameter to
                                        assume that a remote endpoint has
                                        unless and until we are informed
                                        otherwise.

  @param assumeRemoteDms                The decompression_memory_size parameter
                                        to assume that a remote endpoint has
                                        unless and until we are informed
                                        otherwise.

  @param assumeRemoteVersion            The version of SigComp to assume
                                        that a remote endpoing is using
                                        unless and until we are informed
                                        otherwise.
*/
osc::StateHandler::StateHandler(size_t maxStateSpaceInBytes,
                                size_t maximumCyclesPerBit,
                                size_t maxDecompressionSpaceInBytes,
                                osc::u16 sigcompVersion,
                                osc::count_t nackThreshold,
                                unsigned int assumeRemoteSms,
                                unsigned int assumeRemoteCpb,
                                unsigned int assumeRemoteDms,
                                osc::u16 assumeRemoteVersion)
  : m_compartments(maxStateSpaceInBytes, 4,
                   assumeRemoteCpb,
                   assumeRemoteDms,
                   assumeRemoteSms,
                   assumeRemoteVersion),
    m_hasSipDictionary(false),
    m_nackThreshold(nackThreshold)
{
  DEBUG_STACK_FRAME;
  m_parameters.state_memory_size = maxStateSpaceInBytes;
  m_parameters.cycles_per_bit = maximumCyclesPerBit;
  m_parameters.decompression_memory_size = maxDecompressionSpaceInBytes;
  m_parameters.SigComp_version = sigcompVersion;
  m_nacks = new NackMap();
}

/**
  @return the number of states in the StateHandler
*/
osc::count_t osc::StateHandler::numberOfStates()
{
  DEBUG_STACK_FRAME;
  m_states.readLock();
  osc::count_t count = m_states.count();
  m_states.unlock();
  return count;    
}

/**
  @return the number of compartments in the StateHandler
*/
osc::count_t osc::StateHandler::numberOfCompartments ()
{
  DEBUG_STACK_FRAME;
  m_compartments.readLock ();
  osc::count_t count = m_compartments.getCompartmentCount ();
  m_compartments.unlock ();
  return count;
}

/**
  @note The caller is responsible for locking the compartment
        before calling this method.
*/
void osc::StateHandler::addNack (osc::sha1_t &sha1,
                                 osc::Compartment * compartment)
{
  DEBUG_STACK_FRAME;
  m_nackLock.writeLock();
  m_nacks = m_nacks->add(compartment, sha1);
  m_compartments.readLock();
  osc::count_t compartments = m_compartments.getCompartmentCount();
  m_compartments.unlock();
  while(m_nacks->getNackCount() > m_nackThreshold * compartments)
  {
    m_nacks->removeOldest();
  }
  m_nackLock.unlock();
}

/**
  Finds a Compartment that contains a NACKed Message by its SHA-1 hash.

  @param hash

  @return a pointer to the compartment containing that message
          or null if a match is not found

*/
osc::Compartment *
osc::StateHandler::findNackedCompartment(const osc::sha1_t & hash)
{
  DEBUG_STACK_FRAME;
  
  m_nackLock.readLock();
  osc::Compartment * compartment = m_nacks->find(hash);
  if (compartment)
  {
    compartment->retain();
  }
  m_nackLock.unlock();
  return compartment;
}
  
/**
  @param stateId the Id to match

  @return a pointer to a State if the StateId is matched or
    if the match fails it returns a null pointer.
*/
    
osc::State *
osc::StateHandler::getState(osc::state_id_t const &stateId, 
                            osc::nack_code_t &nackCode)
{
  DEBUG_STACK_FRAME;
  m_states.readLock ();
  State *state = m_states.find (stateId, nackCode);
  if (state != 0)
  {
    // Not a normal retain -- m_states is already locked.
    state->incrementUsageCount ();
  }
  m_states.unlock ();
  return state;
}

/**
  Returns a pointer to a compartment object with the supplied
  CompartmentId or if none can be matched a pointer to a new
  compartment is returned.

  This method also ensures that the NackMap doesn't grow beyond
  its allowed limits.
*/
osc::Compartment *
osc::StateHandler::getCompartment(osc::compartment_id_t const &compartmentId)
{
  DEBUG_STACK_FRAME;
  m_compartments.writeLock ();
  osc::Compartment * compartment =
    m_compartments.getCompartment (compartmentId);
  compartment->retain ();
  osc::count_t compartments = m_compartments.getCompartmentCount();
  m_compartments.unlock ();

  // Trip the NACK map
  m_nackLock.writeLock();
  while(m_nacks->getNackCount() > m_nackThreshold * compartments)
  {
    m_nacks->removeOldest();
  }
  //^m_nacks->setMaxSize(m_nackThreshold * compartments);
  m_nackLock.unlock();
  return compartment;
}

/**
  Increments the usage count associated with a state.

  @param state The state to retain
*/
osc::count_t osc::StateHandler::retainState(osc::State *state)
{
  DEBUG_STACK_FRAME;
  m_states.readLock ();
  osc::count_t count = state->incrementUsageCount();
  m_states.unlock ();
  return count;
}

/**
  Decrements the usage count associated with a state. If the
  usage count reaches 0, will remove the state from the state
  map and delete the state.

  @param state The state to release
*/
osc::count_t osc::StateHandler::releaseState(osc::State *state)
{
  DEBUG_STACK_FRAME;
  m_states.writeLock ();
  osc::count_t count = state->decrementUsageCount();
  if (count == 0)
  {
    m_states.remove(state->getStateId());
    delete state;
  }
  m_states.unlock ();
  return count;
}

/**
  Decrements the retain count associated with a compartment. If the retain
  count reaches 0, will remove the compartment from the compartment
  map. If both the retain count and the nack retain count reach 0,
  this method will delete the compartment.

  @param compartment The compartment to release
*/
osc::count_t osc::StateHandler::releaseCompartment(
    osc::Compartment *compartment)
{
  DEBUG_STACK_FRAME;

  // Must lock both maps because we read nackRetainCount from compartment.
  m_nackLock.readLock();
  m_compartments.writeLock ();

  osc::count_t count = compartment->decrementRetainCount();
  if (count == 0)
  {
    m_compartments.removeCompartment(compartment->getCompartmentId());
  }

  if (compartment->isZombie())
  {
    delete compartment;
  }
  m_compartments.unlock ();
  m_nackLock.unlock();

  return count;
}

/**
  Decrements the nack retain count associated with a compartment. If
  the nack retain count reaches 0, will remove the compartment from the
  compartment map. If both the retain count and the nack retain count
  reach 0, this method will delete the compartment.

  @param compartment The compartment to release

  @note This method assumes that the caller has locked the nackLock
        lock for writing. In practice, this is pretty easy: the only
        way this method can be invoked is by a NackNode in the NackMap
        being removed.  The only way a NackNode can be removed from
        the NackMap is by calling NackMap::removeOldest(). And whenever
        NackMap::removeOldest() is invoked, the nackMap must be locked
        for writing anyway.
*/
osc::count_t osc::StateHandler::releaseCompartmentNack(
    osc::Compartment *compartment)
{
  DEBUG_STACK_FRAME;

  // Must lock both maps because we read retainCount from compartment.
  m_compartments.readLock();

  osc::count_t count = compartment->decrementNackRetainCount();

  if (compartment->isZombie())
  {
    delete compartment;
  }

  m_compartments.unlock ();

  return count;
}

/**
  Adds a state to the state map and the creation queue.

  @param state

  @return pointer to the state that was added.  This may not
          be the same as the state that was passed in.
*/
osc::State *osc::StateHandler::addState(osc::State *state)
{
  DEBUG_STACK_FRAME;
  osc::State *retval;
  state->setCountObserver (this);
  m_states.writeLock ();
  retval = m_states.insert (state);
  m_states.unlock ();
  return retval;
}

/**
  Adds a state to the state map, the creation queue and to a
  Compartment with the given id

  @param state

  @param compartmentId

  @return Returns the amount of state space allocated.

  @warning Once you pass a state pointer into this method, you must
           not use it again. It may be invalid.
*/
size_t
osc::StateHandler::addState(osc::State *state,
                            osc::compartment_id_t const &compartmentId,
                            osc::u16 priority)
{
  DEBUG_STACK_FRAME;
  state->retain();

  osc::Compartment * compartment = getCompartment(compartmentId);
  compartment->writeLock();
  
  // Truncating as defined in RFC 3320 6.2.2
  size_t max_size = compartment->totalStateMemoryAvailable();
  if( (state->getStateSize() + 64) > max_size )
  {
    state->truncate(max_size - 64);
  }

  // Add to the global state map. If this state is a duplicate of
  // a state that is already present, then addState will
  // delete the state that was passed in and return a pointer
  // to its doppelganger.
  state = addState(state);

  compartment->addState(state, priority);
  compartment->unlock();
  compartment->release();

  state->release();
  return 0;
}

/**
  This takes a StateChanges object and applies those changes to the
  provided Compartment.  It will make no changes to a compartment that
  does not exist.

  @param changes

  @param compartment

  @note This method assumes that the compartment has been write-locked
        by the caller.
*/
void
osc::StateHandler::processChanges(osc::StateChanges &changes,
                                  osc::Compartment &compartment)
{
  DEBUG_STACK_FRAME;

  if (changes.getSBit())
  {
    m_compartments.writeLock ();
    m_compartments.removeCompartment (compartment.getCompartmentId());
    m_compartments.unlock();
    return;
  }

  osc::u8 i;

  for (i = 0; i < changes.getNumOperations(); i++)
  {
    switch (changes.getOperationType(i))
    {
      case osc::StateChanges::ADD_STATE:
      {
        osc::u16 priority;
        priority = changes.getPriority(i);
        osc::State *state = changes.getState(i);
        changes.setState(i,0);
        state->retain();

        if (state->getStateSize() + 64 > 
            compartment.totalStateMemoryAvailable())
        {
          state->truncate(compartment.totalStateMemoryAvailable() - 64);
        }

        state = addState(state);
        compartment.addState(state, priority);
        state->release();
        break;
      }

      case osc::StateChanges::REMOVE_STATE:
        compartment.removeState(changes.getStateId(i));
        break;

      default:
        assert(0);
    }
  }

  if (changes.getCpbDmsSms())
  {
    compartment.setCpbDmsSms(changes.getCpbDmsSms());
  }

  if (changes.getRemoteSigcompVersion())
  {
    compartment.setRemoteSigcompVersion(changes.getRemoteSigcompVersion());
  }

  compartment.resetRemoteAdvertizedStates();
  for (i = 0; i < changes.getNumRemoteStates(); i++)
  {
    compartment.addRemoteAdvertizedState(changes.getRemoteState(i));
  }

  if (changes.iBitIsValid())
  {
    compartment.setIBit(changes.getIBit());
  }
  compartment.setRequestedFeedback(changes.getRequestedFeedback());
  compartment.setReturnedFeedback(changes.getReturnedFeedback());

}

/**
  This method removes all the compartments which have not had their
  retain method called since the last time this method was called.

  If the compartments aren't in use elsewhere, they will be
  deallocated.
*/
void osc::StateHandler::removeStaleCompartments()
{
  DEBUG_STACK_FRAME;
  
  m_nackLock.readLock();
  m_compartments.writeLock ();

  osc::compartment_node_t * stales = m_compartments.removeStaleCompartments();
  osc::compartment_node_t * tmp;

  // They're not in the map any more, but we still need to
  // decrement their usage count and delete them if they're
  // no longer referenced.
  while ( stales )
  {
    stales->compartment->decrementRetainCount();
    if (stales->compartment->isZombie())
    {
      delete stales->compartment;
    }
    tmp = stales;
    stales = stales->next;
    delete(tmp);
  }

  m_compartments.unlock ();
  m_nackLock.unlock();

}

/**
  @todo This is a bit inefficient. In the final version, we should
        store these values in the single-byte version specified in
        3320, and decode them for the getCyclesPerBit(), etc. calls.
*/

osc::byte_t
osc::StateHandler::getCpbDmsSms()
{
  DEBUG_STACK_FRAME;
  return Compartment::makeCpbDmsSms(getCyclesPerBit(),
                                    getDecompressionMemorySize(),
                                    getStateMemorySize());
}

osc::StateHandler::~StateHandler()
{
  DEBUG_STACK_FRAME;  
  delete m_nacks;
}

/**
  This method instructs the state manager to advertise and (where
  possible) make use of the predefined SIP/SDP dictionary defined in
  RFC 3485. This action cannot be undone.
*/

void
osc::StateHandler::useSipDictionary()
{
  DEBUG_STACK_FRAME;
  addState(new osc::SipDictionary());
  m_hasSipDictionary = true;
}

osc::count_t
osc::StateHandler::numberOfNacks()
{
  return m_nacks->getNackCount();
}

#ifdef DEBUG
void
osc::StateHandler::dump(std::ostream &out, unsigned int indent)
{
  m_nacks->dump(out,indent);
}

/**
  Debugging-only method
*/
void
osc::StateHandler::processChanges(osc::StateChanges & changes,
                                   osc::compartment_id_t const &compartmentId)
{
  DEBUG_STACK_FRAME;
  osc::Compartment * compartment = getCompartment (compartmentId);
  if (!compartment)
  {
    return;
  }

  compartment->writeLock();
  processChanges(changes, *compartment);
  compartment->unlock();
  compartment->release();
}
#endif
