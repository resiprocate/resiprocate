#ifndef __OSC__STATE_MAP
#define __OSC__STATE_MAP 1

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
  @file StateMap.h
  @brief Header file for osc::StateMap class.
*/


#include "Buffer.h"
#include "NackCodes.h"
#include "ReadWriteLockable.h"

namespace osc
{
  class State;
  class StateMap;

  /**
    Hash map of State objects, keyed by State ID.

    This is a straightforward hash map that takes State ID as a key,
    and returns the state object (if any) associated with that ID. The
    behavior of this map does vary slightly from a traditional StateMap,
    since it allows states to be looked up using partial keys. While
    State IDs are defined to be 20 bytes long, this map can find states
    based on as little as 6 bytes of their state ID.
  */

  class StateMap : public osc::ReadWriteLockable
  {
    public:
      StateMap(size_t buckets = 256);
      ~StateMap();

      osc::State *insert (osc::State*);
      osc::State *find(const osc::state_id_t &,
                       osc::nack_code_t &);
      osc::State *remove(const osc::state_id_t &);
 
      StateMap * operator &(){ return this; }
      StateMap const * operator &() const { return this; }

      size_t count() const { return m_numStates; }

    protected:

      /**
        Node for a non-invasive singly linked list of State objects.
       */
      class StateNode
      {
        public:
          StateNode(osc::State *s, StateNode *n = 0) 
            : m_state(s), m_next(n) {;}

          StateNode *getNext() const { return m_next; }
          State *getState() const { return m_state; }
          void setNext(StateNode *next) { m_next = next; }

        private:
          osc::State *m_state;
#if defined(_MSC_VER) && (_MSC_VER < 1300)
          StateNode *m_next;
#else
          osc::StateMap::StateNode *m_next;
#endif
      };

    private:
      /* if you define these, move them to public */
      StateMap(StateMap const &);
      StateMap& operator=(StateMap const &);

      void resizeHash(size_t buckets);
      size_t calculateBucketNumber(const osc::byte_t *id);

    private:
      StateNode **m_states;
      size_t m_numberOfBuckets;
      osc::u8 m_keyBits;
      size_t m_numStates;
  };
}

#endif
