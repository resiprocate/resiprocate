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
  @file CompartmentMap.h
  @brief Header file for osc::CompartmentMap class.
*/

#ifndef __OSC__COMPARTMENT_MAP
#define __OSC__COMPARTMENT_MAP 1
#include "ReadWriteLockable.h"
#include "SigcompMessage.h"
#include "Types.h"
#include "Buffer.h"
#include <assert.h>

#ifdef DEBUG
#include <ostream>
#endif

#define OSC_NO_LIMIT 0
namespace osc
{
  class Compartment;

/**
  Used to create linked lists of compartments.

  This structure is used by the osc::CompartmentMap and
  osc::StateHandler to create and pass around singly-linked lists
  of compartments. Its use is limited strictly to those two classes.
*/

    struct compartment_node_t
    {
      osc::compartment_node_t * next;
      osc::Compartment * compartment;
    };

/**
  Hash map of compartments.

  The CompartmentMap class maintains a hash-based map of osc::Compartment
  instances, keyed off of compartment IDs. It also acts as a factory
  for new compartments.

  @note It is not possible to insert a compartment into the map.
        Compartments end up in the map by calling "getCompartment()"
        with an ID that does not match any existing compartment.
*/

class CompartmentMap: public osc::ReadWriteLockable
{

  protected:
    size_t m_maxStateSpacePerCompartment;

  public:

    CompartmentMap (size_t maxStateSpacePerCompartment = OSC_NO_LIMIT,
                    size_t bucketRehashThreshold = 4,
                    unsigned int assumeCpb = 16,
                    unsigned int assumeDms = 8192,
                    unsigned int assumeSms = 2048,
                    osc::u16 assumeVersion = 1);
    ~CompartmentMap();

    void removeCompartment(osc::compartment_id_t const &cID);
    osc::Compartment * getCompartment(osc::compartment_id_t const &cID);

    osc::count_t getCompartmentCount();
    osc::compartment_node_t * removeStaleCompartments();

    osc::CompartmentMap * operator &(){ return this; }
    osc::CompartmentMap const * operator &() const { return this; }

#ifdef DEBUG
#ifndef NO_TEMPLATES
    template <typename T> void removeCompartment(const T& id);
    template <typename T> osc::Compartment * getCompartment(const T& id);
#else
    void removeCompartment(int id);
    osc::Compartment * getCompartment(int id);
#endif
    void dump(std::ostream &, unsigned int indent = 0) const;
#endif

  private:

    // Copying is verboten
    CompartmentMap(osc::CompartmentMap const &map){ assert(0); }

    /**
      Helper class for CompartmentMap.

      CompartmentBuckets contain an array of one or more
      compartments that have matching hash values.
    */
    class CompartmentBucket
    {
      public:
        CompartmentBucket():
          m_size(4),
          m_count(0)
        {
            m_compartments = new osc::Compartment * [m_size];
            if (!m_compartments)
            {
              m_size = 0;
            }
        }
        ~CompartmentBucket();
        void destroyCompartments();
        size_t add( osc::Compartment * compartment );
        osc::Compartment * contains(osc::compartment_id_t const &cID);
#if defined(_MSC_VER) && (_MSC_VER < 1300)
        void rehash
          (
            CompartmentBucket newBuckets[], 
            osc::u32 mask, 
            size_t top
          );
#else
        void rehash
          (
            osc::CompartmentMap::CompartmentBucket newBuckets[], 
            osc::u32 mask, 
            size_t top
          );
#endif
        bool remove(osc::compartment_id_t const &cID);
        osc::compartment_node_t * getStale(osc::compartment_node_t * node);
#ifdef DEBUG
        void dump(std::ostream &, unsigned int indent = 0) const;
#endif
      private:
#if defined(_MSC_VER) && (_MSC_VER < 1300)
        CompartmentBucket(CompartmentBucket &bucket){ assert(0); }
#else
        CompartmentBucket(osc::CompartmentMap::CompartmentBucket &bucket){ assert(0); }
#endif
        void remove(size_t index);

        osc::Compartment ** m_compartments; ///< Array of compartments in bucket
        size_t m_size; ///<Size of bucket
        osc::count_t m_count; ///<Number of compartments in the bucket
    };
    
    osc::count_t m_bucketCount; ///<The number of buckets
    osc::count_t m_compartments; ///<The number of compartments in the buckets
    size_t m_mask;               ///<The bitmask for determining bucket address
    size_t m_rehashThreshold;   ///<The maximum size that a bucket should grow to before a rehash
    osc::CompartmentMap::CompartmentBucket * m_buckets; ///<The buckets!

    osc::u8  m_assumeCpbDmsSms;
    osc::u16 m_assumeVersion;
    
    void growHash();
    void insertCompartment(osc::Compartment &compartment);

};

#ifdef DEBUG
#ifndef NO_TEMPLATES
/**
  Convenience method to remove a compartment when compartment IDs
  are native types. This method is used only for debugging.

  @note It must be valid to perform byte-wise comparisons on the
        passed in IDs for this method to be valid.
 */
template <typename T> void 
osc::CompartmentMap::removeCompartment(const T& id)
{
  osc::compartment_id_t cid(reinterpret_cast<const osc::byte_t*>(&id),
                            sizeof(id));
  removeCompartment(cid);
}

/**
  Convenience method to get a compartment when compartment IDs
  are native types. This method is used only for debugging.

  @note It must be valid to perform byte-wise comparisons on the
        passed in IDs for this method to be valid.
 */
template <typename T> osc::Compartment *
osc::CompartmentMap::getCompartment(const T& id)
{
  osc::compartment_id_t cid(reinterpret_cast<const osc::byte_t*>(&id),
                            sizeof(id));
  return getCompartment(cid);
}
#else
/**
  Convenience method to remove a compartment when compartment IDs
  are integers. This method is used only for debugging.
 */
inline void 
osc::CompartmentMap::removeCompartment(int id)
{
  osc::compartment_id_t cid(reinterpret_cast<const osc::byte_t*>(&id),
                            sizeof(id));
  removeCompartment(cid);
}

/**
  Convenience method to get a compartment when compartment IDs
  are integers. This method is used only for debugging.
 */
inline osc::Compartment *
osc::CompartmentMap::getCompartment(int id)
{
  osc::compartment_id_t cid(reinterpret_cast<const osc::byte_t*>(&id),
                            sizeof(id));
  return getCompartment(cid);
}
#endif

std::ostream& operator<< (std::ostream &, const osc::CompartmentMap &);
#endif

}
#endif
 
