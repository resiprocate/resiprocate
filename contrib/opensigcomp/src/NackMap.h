#ifndef   __OSC_NACK_MAP
#define   __OSC_NACK_MAP 1

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
  @file NackMap.h
  @brief Header file for osc::NackMap class.
*/


#include "Libc.h"
#include "Types.h"

namespace osc
{
  class Compartment;

  /**
    Hash map used to correlate from NACK messages to the proper compartment.

    RFC 4077 requires SigComp implementations to have the ability,
    upon receiving a NACK message, to locate the compartment to which
    the NACK corresponds. This class tracks SHA-1 hashes for the last
    several messages sent (approximately 4 * the current number of
    compartments) and allows them to be correlated back to the
    compartment to which they correspond.
  */

  class NackMap
  {
    public:
      NackMap();

      NackMap(osc::count_t buckets, 
              osc::count_t bucketSize,
              osc::u32 mask,
              osc::NackMap *source=0);

      ~NackMap();

      osc::NackMap *add(osc::Compartment *compartment, osc::sha1_t &digest);

      void removeOldest();
      osc::count_t getNackCount();
      osc::Compartment * find(const osc::sha1_t &sha1);
    
#ifdef DEBUG
      void dump(std::ostream &out, unsigned int indent);
#endif
    
    private:
      osc::u32 getHash(const osc::sha1_t &sha1);
      void remove(osc::sha1_t &sha1);

      /**
        Helper class for NackMap that allows storage of NACK
        records both by hash value and in chronological order.
      */
      class NackNode
      {
        public:
          NackNode * m_next;
          NackNode * m_prev;
          osc::Compartment * m_compartment;
          osc::sha1_t m_sha1;
    
          NackNode(osc::Compartment * compartment,
                   osc::sha1_t &sha1,
                   NackNode *previous,
                   NackNode * next);
          void unlink();
          ~NackNode();
      };

      // m_bottom and m_top point to the head and tail of a
      // chronologically-ordered list of NackNodes. They are
      // used to retire old nodes when necessary.

      NackNode * m_bottom;
      NackNode * m_top;

      // m_list allows direct hashed look-up of a compartment
      // using the sha-1 hash of a sent message as a key.
      NackNode ** m_list;

      osc::count_t m_nacks;
      osc::count_t m_bucketCount;
      osc::count_t m_bucketSize;
      osc::count_t * m_bucketTops;
      osc::u32 m_mask;
      
  };
  
}
#endif
