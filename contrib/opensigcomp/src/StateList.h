#ifndef __OSC__STATE_LIST
#define __OSC__STATE_LIST 1

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
  @file StateList.h
  @brief Header file for osc::StateList class.
*/


#include "Types.h"
#include "Buffer.h"

#ifdef DEBUG
#include <ostream>
#endif

namespace osc
{
  class State;

  /**
    Tracks a list of states associated with a compartment.

    The StateHandler uses StateList objects to track locally available
    state associated with a Compartment.

    Compressors use StateList objects to track remotely available states.
    The StateList objects include a flag that indicates whether the
    remote endpoint has acknolwedged receipt of the state. It is up to
    the compressors to mark remote states as acknowledged.

    The StateList class implements the compartment quota logic described
    in RFC 3320, except for state truncation (which is managed by the
    StateHandler).
  */

  class StateList
  {
    public:
      StateList(size_t quota);
      ~StateList();
 
      StateList * operator &(){ return this; }
      StateList const * operator &() const { return this; }

      void removeState(osc::state_id_t const &stateId);
      void addState(osc::State *state, osc::u16 priority, bool acked);

      void reset();

      void ackState(osc::state_id_t const &stateId);
      bool isStateAcked(osc::state_id_t const &stateId) const;

      void setQuota(size_t quota);
      size_t getQuota() const {return m_quota;}
      size_t getQuotaLeft() const {return m_quota - m_stateMemoryUsed;}

      osc::State *getMostRecentAckedState(osc::u16 priority);


#ifdef DEBUG
      void dump(std::ostream &, unsigned indent = 0) const;
#endif

    protected:
      /**
        Tracks state associated with a single state in a StateList.

        Because the StateList class tracks states non-invasively, the
        StateList::StateNode class is used to create linked lists of
        states in the StateList. It also tracks state priorities (used
        for order of state discarding) and whether the state has been
        acked by the remote endpoint.

        Each StateList::StateNode correponds to precisely one State.
      */
      class StateNode
      {
        public:
          StateNode(osc::State *s, osc::u16 priority = 65535);
          ~StateNode();

          StateNode *getNext() const { return m_next; }
          void setNext(StateNode *next) { m_next = next; }

          StateNode *getPrevious() const { return m_previous; }
          void setPrevious(StateNode *previous) { m_previous = previous; }

          osc::State *getState() const { return m_state; }
          osc::u16 getPriority() const { return m_priority; }
          void setPriority(osc::u16 p) { m_priority = p; }

          void ack() { m_acked = true; }
          bool isAcked() const { return m_acked; }
#ifdef DEBUG
          void dump(std::ostream &, unsigned indent = 0) const;
#endif

        private:
          osc::State *m_state;
#if defined(_MSC_VER) && (_MSC_VER < 1300)
          StateNode *m_next;
          StateNode *m_previous;
#else
          osc::StateList::StateNode *m_next;
          osc::StateList::StateNode *m_previous;
#endif
          osc::u16 m_priority;
          bool m_acked;
      };

      void removeStateNode(osc::StateList::StateNode *);
      void addStateNodeBefore(osc::StateList::StateNode *newNode,
                              osc::StateList::StateNode *oldNode);

      const StateNode *findState(osc::state_id_t const &stateId) const;
      StateNode *findState(osc::state_id_t const &stateId);

    private:
      size_t m_quota;
      size_t m_stateMemoryUsed;
      osc::StateList::StateNode *m_stateHead;
      osc::StateList::StateNode *m_stateTail;

      /* if you define these, move them to public */
      StateList(StateList const &);
      StateList& operator=(StateList const &);
#ifdef DEBUG
    public:
#else
    private:
#endif
      osc::count_t numberOfStates();
  };
#ifdef DEBUG
  std::ostream &operator<<(std::ostream &, const osc::StateList &);
#endif
}

#endif
