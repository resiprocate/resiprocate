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
  @file StateMap.cpp
  @brief Implementation of osc::StateMap class.
*/



#include "ProfileStack.h"
#include <assert.h>
#include "StateMap.h"
#include "State.h"

/**
  Constructor for osc::StateMap.

  @param buckets  Number of hash table entries to initially allocate
                  for the states. As a rule of thumb, this should
                  be on the order of the number of states that this
                  map will eventually hold. If the map becomes
                  too full for the number of buckets, it will
                  rebalance, so getting this value perfectly
                  correct isn't critical.

  @note The number of buckets will be rounded up to the next power of 2.
 */
osc::StateMap::StateMap(size_t buckets) : 
  m_states(0),
  m_numberOfBuckets(0),
  m_keyBits(0),
  m_numStates(0)
{
  DEBUG_STACK_FRAME;
  resizeHash(buckets);
}

/**
  Copy constructor for osc::StateMap.
 */
osc::StateMap::StateMap(StateMap const &r)
{
  DEBUG_STACK_FRAME;
  /* Assign attributes */
  assert(0);
}

/**
  Destructor for osc::StateMap.
 */
osc::StateMap::~StateMap()
{
  DEBUG_STACK_FRAME;
  // Delete all states in the buckets, and the buckets themselves.
  StateNode *curr;
  StateNode *tmp;

  for (size_t i = 0; i < m_numberOfBuckets; i++)
  {
    curr = m_states[i];
    while (curr)
    {
      tmp = curr;
      curr = curr->getNext();
      if(tmp->getState())
      {
        delete(tmp->getState());
      }
      delete(tmp);
    }
  }

  delete[](m_states);
}

/**
  Assignment operator for osc::StateMap.
 */
osc::StateMap &
osc::StateMap::operator=(StateMap const &r)
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
  Sets up the buckets into which the state will be hashed. If
  there are currently buckets allocated, the states will be copied
  out of those buckets into the new buckets. This method can
  be used to both grow and shrink the size of the map.

  @param buckets Number of buckets to use for the new hash table.

  @note The number of buckets will be rounded up to the next power of 2.
 */
void
osc::StateMap::resizeHash(size_t buckets)
{
  DEBUG_STACK_FRAME;
  osc::StateMap::StateNode **oldHash = m_states;
  size_t oldNumBuckets = m_numberOfBuckets;

  // First, ensure that the number of buckets is an
  // even power of 2.
  m_keyBits = 0;
  while (buckets - 1)
  {
    m_keyBits ++;
    buckets >>= 1;
  }
  buckets = 1 << m_keyBits;
  m_numberOfBuckets = buckets;

  // Now, allocate and zero out a new array of buckets
  m_states = new osc::StateMap::StateNode*[m_numberOfBuckets];

  for (size_t i = 0; i < m_numberOfBuckets; i++)
  {
    m_states[i] = 0;
  }

  // Finally, if there was an old list of buckets, rehash
  // those existing states into the new list of buckets, and
  // delete the old buckets. We do this without checking for
  // duplicates. Note that this code reverses the order of
  // states in a bucket, but this should have no practical
  // effect.
  if (oldHash)
  {
    StateNode *curr;
    StateNode *tmp;
    osc::State *state;
    size_t bucket;

    for (size_t i = 0; i < oldNumBuckets; i++)
    {
      curr = oldHash[i];
      while (curr)
      {
        tmp = curr;
        curr = curr->getNext();
        state = tmp->getState();

        // Insert the state into the new map
        bucket = calculateBucketNumber(state->getStateId().data());
        m_states[bucket] = new StateNode(state, m_states[bucket]);

        delete(tmp);
      }
    }
    delete [] oldHash;
  }
}

/**
  Given a state ID, calculates the proper bucket for that state.
  The current implementation is based on the observation that
  state IDs are based on SHA-1 hash values, which are crypto
  hashes. That gives more than sufficient distribution for a
  hash table. One extremely useful property of using the first
  several bytes as a hash value is that it becomes trivial to 
  find the proper bucket for a state, even if only a partial
  state identifier is available.

  @param  id  Pointer to a buffer containing at least 6 bytes
              of a state ID.

  @note The buffer pointed to by "id" is assumed to have enough
        bytes present to resolve to a bucket. In practice, since
        there must be 6 bytes to look up any state, any valid state
        ID will suffice.
 */
size_t
osc::StateMap::calculateBucketNumber(const osc::byte_t *id)
{
  DEBUG_STACK_FRAME;
  int bitsLeft = m_keyBits;
  size_t bucketNumber = 0;

  assert (m_keyBits < 6*8);

  // Pull in any whole bytes of the state ID that we need to use
  while (bitsLeft >= 8)
  {
    bucketNumber <<= 8;
    bucketNumber |= static_cast<size_t>(*id);
    id++;
    bitsLeft -= 8;
  }

  // Finally, pull in any partial byte of the state ID, if any.
  if (bitsLeft)
  {
    bucketNumber <<= bitsLeft;
    bucketNumber |= (static_cast<size_t>(*id) >> (8 - bitsLeft));
  }

  assert (bucketNumber < m_numberOfBuckets);

  return bucketNumber;
}

/**
  @returns Pointer to the state. This may or may not be the
           same pointer that was passed in -- if such is the
           case, then the State object that was passed in
           has already been deallocatred by this method.
           If the pointer is different than what was passed in, the
           caller must be certain to release it. The safest thing to
           do is to retain() the state passed in and then release()
           whatever is passed out.
 */

osc::State *
osc::StateMap::insert(osc::State *state)
{
  DEBUG_STACK_FRAME;
  const osc::byte_t *stateId = state->getStateId().data();
  size_t bucket = calculateBucketNumber(stateId);

  StateNode *curr = m_states[bucket];
  while (curr)
  {
    if (state->isEquivalentTo(*curr->getState()))
    {
      if (curr->getState() != state)
      {

        // We use incrementUsageCount() here instead of
        // retain() because the state map lock is already
        // locked before we enter this insert() method.

        curr->getState()->incrementUsageCount();
        delete state;
      }
      return curr->getState();
    }
    curr = curr->getNext();
  }

  m_states[bucket] = new StateNode(state, m_states[bucket]);
  m_numStates++;

  /*
     If the average size of a bucket exceeds a
     predefined threshold (currently 4), then we
     rehash into a map that is 8 times as large (giving
     a resultant average depth of 0.5).
  */

  if (m_numStates > m_numberOfBuckets * 4)
  {
    resizeHash(m_numberOfBuckets * 8);
  }

  return state;
}

osc::State *
osc::StateMap::find(const osc::state_id_t &key,
                    osc::nack_code_t &status)
{
  DEBUG_STACK_FRAME;
  osc::State *match = 0;
  status = osc::STATE_NOT_FOUND;

  if (key.size() < 6)
  {
    return 0;
  }

  size_t bucket = calculateBucketNumber(key.data());

  StateNode *curr = m_states[bucket];
  while (curr)
  {
    if (curr->getState()->matchesStateId(key))
    {
      if (!match)
      {
        match = curr->getState();
        status = osc::OK;
      }
      else
      {
        // Multiple matches for the key -- return nothing.
        status = osc::ID_NOT_UNIQUE;
        return 0;
      }
    }
    curr = curr->getNext();
  }

  return match;
}

osc::State *
osc::StateMap::remove(const osc::state_id_t &key)
{
  DEBUG_STACK_FRAME;
  StateNode *previous = 0;
  StateNode *matchPrevious = 0;
  StateNode *match = 0;

  size_t bucket = calculateBucketNumber(key.data());

  StateNode *curr = m_states[bucket];
  while (curr)
  {
    if (curr->getState()->matchesStateId(key))
    {
      if (!match)
      {
        matchPrevious = previous;
        match = curr;
      }
      else
      {
        // Multiple matches for the key -- return nothing.
        return 0;
      }
    }
    previous = curr;
    curr = curr->getNext();
  }

  if (!match)
  {
    return 0;
  }

  // Remove this node from the chain.

  // If matchPrevious is 0, then we're removing from the front of the bucket.
  if (!matchPrevious)
  {
    m_states[bucket] = match->getNext();
  }

  // Otherwise, we need to set the previous node to point to
  // the current node's successor.
  else
  {
    matchPrevious->setNext(match->getNext());
  }

  m_numStates--;
  osc::State *state = match->getState();
  delete match;

  return state;
}
