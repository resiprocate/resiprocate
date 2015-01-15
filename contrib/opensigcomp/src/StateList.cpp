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
   of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
   license for additional details.

   To discuss alternate licensing terms, contact info@estacado.net
 *********************************************************************** */

/**
  @file StateList.cpp
  @brief Implementation of osc::StateList class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "StateList.h"
#include "State.h"

/**
  Constructor for osc::StateList.

  @param quota Maximum number of bytes we can store in this
               list. Per RFC3320, each state is considered
               to have an overhead of 64 bytes. Unless specified
               otherwise, we default this to 2048 bytes, which is
               the smallest number more than 0 allowed by spec.
               If the remote endpoint isn't even supporting
               2048 bytes, then things just aren't going to
               work anyway.
 */
osc::StateList::StateList(size_t quota)
  :
    m_quota(quota),
    m_stateMemoryUsed(0),
    m_stateHead(0),
    m_stateTail(0)
{
  DEBUG_STACK_FRAME;
}

/**
  Copy constructor for osc::StateList.
 */
osc::StateList::StateList(StateList const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::StateList.
 */
osc::StateList::~StateList()
{
  DEBUG_STACK_FRAME;
  reset();
}

/**
  Assignment operator for osc::StateList.
 */
osc::StateList &
osc::StateList::operator=(StateList const &r)
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

/**
  This method adds the state to the list of associated states that this
  compartment is aware of.  If the state is already associated with this
  compartment it has no effect (unless the priority has changed).

  @param state    The state to be added to the compartment.

  @param priority The SigComp retention priority for this state
                  within the compartment
*/
void
osc::StateList::addState(osc::State *state, osc::u16 priority, bool acked)
{
  DEBUG_STACK_FRAME;
  StateNode *curr = m_stateHead;
  StateNode *successor = 0;

  while (curr)
  {
    if (!successor && curr->getPriority() <= priority)
    {
      successor = curr;
    }

    // If the added state duplicates an existing state,
    // don't insert it a second time.

    if (curr->getState()->isEquivalentTo(*state))
    {
      // If the state is already present at a higher priority, simply return.
      if (curr->getPriority() > priority)
      {
        return;
      }

      // Otherwise, we need to move the state to the higher
      // priority. Because the list is in priority order,
      // we have necessarily already set successor to the
      // correct value.
      curr->setPriority(priority);

      if (acked)
      {
        curr->ack();
      }

      if ((successor != curr) && (successor != curr->getNext()))
      {
        removeStateNode(curr);
        addStateNodeBefore(curr, successor);
      }
      return;
    }

    curr = curr->getNext();
  }

  m_stateMemoryUsed += state->getStateSize() + 64;

  // Now, we trim any states that make the compartment
  // exceed its quota.
  while (m_stateTail && m_stateMemoryUsed > m_quota)
  {
    curr = m_stateTail;
    if (curr == successor)
    {
      successor = 0;
    }
    removeStateNode(curr);
    m_stateMemoryUsed -= curr->getState()->getStateSize() + 64;
    delete(curr);
  }

  if (state->getStateSize() + 64 <= m_quota)
  {
    StateNode *newNode = new StateNode(state, priority);
    if (acked)
    {
      newNode->ack();
    }
    addStateNodeBefore(newNode, successor);
  }
  else
  {
    m_stateMemoryUsed -= state->getStateSize() + 64;
  }
}

/**
  Removes all states from this list.
*/
void
osc::StateList::reset()
{
  DEBUG_STACK_FRAME;
  StateNode *curr = m_stateHead;
  StateNode *tmp;
  while (curr)
  {
    tmp = curr;
    curr = curr->getNext();
    delete (tmp);
  }
  m_stateHead = 0;
  m_stateTail = 0;
  m_stateMemoryUsed = 0;
}

/**
  Will disassociate a state from this compartment if stateId matches
  exactly one associated state.

  @param stateId the id of the state to remove

  @returns a null pointer if that state id was not removed or
          a pointer to the state if it was
*/

void osc::StateList::removeState(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  StateNode *victim = findState(stateId);
  if (victim)
  {
    removeStateNode(victim);
    m_stateMemoryUsed -= victim->getState()->getStateSize() + 64;
    delete(victim);
  }
}

void
osc::StateList::setQuota(size_t quota)
{
  DEBUG_STACK_FRAME;
  m_quota = quota;

  StateNode *curr;

  while (m_stateTail && m_stateMemoryUsed > m_quota)
  {
    curr = m_stateTail;
    removeStateNode(curr);
    m_stateMemoryUsed -= curr->getState()->getStateSize() + 64;
    delete(curr);
  }
}


void
osc::StateList::ackState(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  StateNode *match = findState(stateId);

  if (match)
  {
    match->ack();
  }
}

bool
osc::StateList::isStateAcked(osc::state_id_t const &stateId) const
{
  DEBUG_STACK_FRAME;
  const StateNode *match = findState(stateId);

  if (match)
  {
    return match->isAcked();
  }

  return false;
}

/**
  @param oldNode  The node before which the new node should be insterted.
                  If set to 0, the new node is inserted at the tail of
                  the list of states.
 */
void
osc::StateList::addStateNodeBefore(osc::StateList::StateNode *newNode,
                                     osc::StateList::StateNode *oldNode)
{
  DEBUG_STACK_FRAME;
  assert(newNode);
  assert(newNode != oldNode);

  if (oldNode)
  {
    newNode->setPrevious(oldNode->getPrevious());
    if (oldNode->getPrevious())
    {
      oldNode->getPrevious()->setNext(newNode);
    }
    else
    {
      m_stateHead = newNode;
    }
    oldNode->setPrevious(newNode);
  }
  // Insert at tail of list
  else
  {
    if (m_stateTail)
    {
      m_stateTail->setNext(newNode);
    }
    else
    {
      m_stateHead = newNode;
    }
    newNode->setPrevious(m_stateTail);
    m_stateTail = newNode;
  }

  newNode->setNext(oldNode);
}

/**
  Removes a state node from the list of states in this compartment.
  Does not perform any checking to determine that the node is
  actually in the list -- so be careful!
 */

void
osc::StateList::removeStateNode(osc::StateList::StateNode *node)
{
  DEBUG_STACK_FRAME;
  if (node->getNext())
  {
    node->getNext()->setPrevious(node->getPrevious());
  }
  else
  {
    m_stateTail = node->getPrevious();
  }

  if (node->getPrevious())
  {
    node->getPrevious()->setNext(node->getNext());
  }
  else
  {
    m_stateHead = node->getNext();
  }
  node->setNext(0);
  node->setPrevious(0);
}

const osc::StateList::StateNode *
osc::StateList::findState(osc::state_id_t const &stateId) const
{
  DEBUG_STACK_FRAME;
  StateNode *curr = m_stateHead;
  StateNode *match = 0;

  if (stateId.size() < 6) 
  {
    return 0;
  }

  while (curr)
  {
    if (curr->getState()->matchesStateId(stateId))
    {
      if (match)
      {
        // Whoops. Double match.
        return 0;
      }
      match = curr;
    }
    curr = curr->getNext();
  }
  return match;
}

osc::StateList::StateNode *
osc::StateList::findState(osc::state_id_t const &stateId)
{
  DEBUG_STACK_FRAME;
  StateNode *curr = m_stateHead;
  StateNode *match = 0;

  if (stateId.size() < 6) 
  {
    return 0;
  }

  while (curr)
  {
    if (curr->getState()->matchesStateId(stateId))
    {
      if (match)
      {
        // Whoops. Double match.
        return 0;
      }
      match = curr;
    }
    curr = curr->getNext();
  }
  return match;
}

/**
  @returns the number of states that the compartment holds
  @note For debugging use only -- not particularly efficient.
*/
osc::count_t osc::StateList::numberOfStates()
{
  DEBUG_STACK_FRAME;
  StateNode *curr = m_stateHead;
  osc::count_t count = 0;

  while (curr)
  {
    count++;
    curr = curr->getNext();
  }

  return count;
}

/**
  Returns the most recent acknowledged state at a
  given priority.

  @note Callers of this method must call release() on
        the returned state when they are done with it.

  @todo This doesn't work if you have multiple compressors
        installed.  One compressor can end up getting a
        state that was installed by another compressor, which
        isn't all that useful. In practice, we should mark acked
        states with not just a boolean flag, but a decompressor
        identifier. Then, this method will take a priority
        and a compressor identifier as arguments, and only 
        return states that have been acked for the specified
        compressor.
*/

osc::State *
osc::StateList::getMostRecentAckedState(osc::u16 priority)
{
  DEBUG_STACK_FRAME;
  osc::StateList::StateNode *curr = m_stateHead;

  while (curr && curr->getPriority() > priority)
  {
    curr = curr->getNext();
  }

  while (curr && curr->getPriority() == priority)
  {
    if (curr->isAcked())
    {
      curr->getState()->retain();
      return curr->getState();
    }
    curr = curr->getNext();
  }

  return 0;
}

osc::StateList::StateNode::StateNode(osc::State *s, osc::u16 priority)
            : m_state(s), m_next(0), m_previous(0), m_priority(priority),
              m_acked(false)
{
  m_state->retain();
}

osc::StateList::StateNode::~StateNode() 
{
  if (m_state) 
  {
    m_state->release();
  }
}


#ifdef DEBUG
void
osc::StateList::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  os << std::setw(indent) << ""
     << "[StateList " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  Quota          = " << m_quota << std::endl
     << std::setw(indent) << ""
     << "  Memory Used    = " << m_stateMemoryUsed << std::endl;

  for (StateNode* i = m_stateHead; i; i=i->getNext())
  {
    i->dump(os, indent + 2);
  }
}

void
osc::StateList::StateNode::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  os << std::setw(indent) << ""
     << "[StateNode " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  Previous = " << m_previous << std::endl
     << std::setw(indent) << ""
     << "  Next     = " << m_next << std::endl
     << std::setw(indent) << ""
     << "  Priority = " << m_priority << std::endl
     << std::setw(indent) << ""
     << "  Acked    = " << (m_acked?"True":"False") << std::endl;
  m_state->dump(os, indent + 2);
}

std::ostream &
osc::operator<< (std::ostream &os, const osc::StateList &b)
{
  DEBUG_STACK_FRAME;
  b.dump(os);
  return os;
}
#endif
