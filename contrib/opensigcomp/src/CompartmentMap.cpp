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
  @file CompartmentMap.cpp
  @brief Implementation of osc::CompartmentMap class.
*/


#include "ProfileStack.h"
#include "Libc.h"
#include "CompartmentMap.h"
#include "Compartment.h"
#include "SigcompMessage.h"
#include "Types.h"

#ifdef DEBUG
#include <iomanip>
#endif

/**
  This constructor creates the compartment map with and sets the max
  storage size for compartments and messages.  

  @param maxStateSpacePerCompartment  The amount of state memory we allow the
                                      remote endpoint to store, on a per-
                                      compartment basis.

  @param bucketRehashThreshold        When a bucket grows to hold more than 
                                      this many compartments, we re-hash
                                      everything into a larger map.
*/
osc::CompartmentMap::CompartmentMap (size_t maxStateSpacePerCompartment,
                                     size_t bucketRehashThreshold,
                                     unsigned int assumeCpb,
                                     unsigned int assumeDms,
                                     unsigned int assumeSms,
                                     osc::u16 assumeVersion):
  m_maxStateSpacePerCompartment(maxStateSpacePerCompartment),
  m_bucketCount(8),
  m_compartments(0),
  m_mask(1|2|4),
  m_rehashThreshold(bucketRehashThreshold),
  m_assumeCpbDmsSms(osc::Compartment::makeCpbDmsSms(assumeCpb,
                                                    assumeDms,
                                                    assumeSms)),
  m_assumeVersion(assumeVersion)
{
  DEBUG_STACK_FRAME;
  m_buckets = new osc::CompartmentMap::CompartmentBucket[m_bucketCount];
  if (!m_buckets)
  {
    m_bucketCount = 0;
  }
}

/**
  @returns the number of compartments 
*/
osc::count_t osc::CompartmentMap::getCompartmentCount()
{
  DEBUG_STACK_FRAME;
    return m_compartments;
}

/**
  This method will remove all compartments that have not been flagged
  as non stale since the last time this was called.

  @returns A list of pointers to the compartments removed

  @note    The caller is responsible for deallocating the
           returned list of compartments.
*/  
osc::compartment_node_t *
osc::CompartmentMap::removeStaleCompartments()
{
  DEBUG_STACK_FRAME;
  compartment_node_t * start = new compartment_node_t;
  if (!start)
  {
    return 0;
  }

  start->compartment = 0;
  start->next = 0;
  compartment_node_t * current = start;

  size_t p;
  for(p = 0; p < m_bucketCount; p++)
  {
    current = m_buckets[p].getStale(current);
  }

  // Skip over the empty starting node
  current = start->next;
  delete start;
  start = current;

  // Traverse the list and remove any stale compartments
  while ( current ) 
  {
    removeCompartment( current->compartment->getCompartmentId() );
    current = current->next;
  }

  return start;
}

/**
  Class Destructor
*/
osc::CompartmentMap::~CompartmentMap()
{
  DEBUG_STACK_FRAME; 
  if ( m_buckets )
  {
    for (size_t i = 0; i < m_bucketCount; i++)
    {
      m_buckets[i].destroyCompartments();
    }
    delete[] m_buckets;
    m_buckets = 0;
  }
}

void
osc::CompartmentMap::growHash()
{
  DEBUG_STACK_FRAME;
  m_mask = 1 | ( m_mask << 1 ); // Extend the mask to another power of two
  osc::count_t newBucketCount = m_bucketCount << 1; // Double the bucket space
  if( newBucketCount < 2 )
  {
    newBucketCount = 2;
  }
  // Create the new buckets
  osc::CompartmentMap::CompartmentBucket * newBuckets = new 
    osc::CompartmentMap::CompartmentBucket[newBucketCount];

  osc::count_t pos;
  for(pos = 0; pos < m_bucketCount; pos++)
  {
    // Rehash the contents of all the buckets
    m_buckets[pos].rehash(newBuckets, m_mask, newBucketCount);
  }
  // Delete the old buckets, ...
  if ( m_buckets )
  {
    delete[] m_buckets;
  }
  // and use the new buckets
  m_buckets = newBuckets;
  m_bucketCount = newBucketCount;
}

void 
osc::CompartmentMap::insertCompartment
  ( osc::Compartment &compartment )
{
  DEBUG_STACK_FRAME;
  // Get the hash for the compartment
  osc::u32 hash = ( compartment.getLookup2Hash() ) & m_mask;
  
  // Make sure the hash fits in the bucket space
  assert( hash < m_bucketCount );
  
  // Check to see that the bucket doesn't contains the compartment
  if( !m_buckets[hash].contains( compartment.getCompartmentId() ) )
  {
    m_compartments++;

    // The old code here was as follows; this is too aggressive.
    //  if( m_buckets[hash].add( &compartment ) > m_rehashThreshold )

    // Add the compartment; if the average bucket size is too large,
    // rehash the buckets

    m_buckets[hash].add( &compartment );
    if( (m_compartments/m_bucketCount) >= m_rehashThreshold )
    {
      growHash();
    }
  }
}

void
osc::CompartmentMap::removeCompartment ( osc::compartment_id_t const &cID )
{
  DEBUG_STACK_FRAME;
  // Get the hash
  osc::u32 hash = ( cID.getLookup2Hash() ) & m_mask;
  // Check to make sure the hash fits in the bucket space
  assert( hash < m_bucketCount ); 
  // Remove the offending compartment
  if( m_buckets[hash].remove( cID ) )
  {
      m_compartments--;
  }
}

/**
  This method will return a compartment that is already in the map
  if it can find a match or it will create a new compartment, insert
  it into the map, and then return it if there is no match.

  @param compartmentId
  @returns a pointer to a compartment
*/

osc::Compartment * 
osc::CompartmentMap::getCompartment ( osc::compartment_id_t const &cID )
{
  DEBUG_STACK_FRAME;
  // Get the hash
  osc::u32 hash = ( cID.getLookup2Hash() ) & m_mask;
  // Check to make sure that hash is inside the bucket space
  assert( hash < m_bucketCount );
  osc::Compartment * compartment = m_buckets[hash].contains( cID );
  if( !compartment ) // Does the compartment exist?
  {
    // If it doesn't exist create it,
    compartment = new Compartment(cID, m_maxStateSpacePerCompartment,
                                  m_assumeCpbDmsSms, m_assumeVersion);
    // Then add it to the hash map
    insertCompartment(*compartment);
  }
  return compartment;
}

///////////////////////////////////////////////////////////////////////////
// CompartmentBucket Methods


osc::CompartmentMap::CompartmentBucket::~CompartmentBucket()
{
  DEBUG_STACK_FRAME;
  if(m_compartments)
  {
    delete[] m_compartments;
  }
}

void
osc::CompartmentMap::CompartmentBucket::destroyCompartments()
{
  DEBUG_STACK_FRAME;
  for (size_t i = 0; i < m_count; i++)
  {
    delete (m_compartments[i]);
  }
}

size_t 
osc::CompartmentMap::CompartmentBucket::add ( osc::Compartment * compartment )
{
  DEBUG_STACK_FRAME;
  if(compartment != 0)
  {
    // Is the bucket too small?
    if(m_count == m_size)
    {
      // Create new bucket space
      osc::Compartment ** new_compartments = 
        new osc::Compartment * [m_size * 2];

      // Move in the old data
      OSC_MEMMOVE( 
        new_compartments, m_compartments, sizeof( osc::Compartment * ) * m_count
      );
      // Delete the old bucket space
      if( m_compartments )
      {
        delete[] m_compartments;
      }

      // Use the new buffer
      m_compartments = new_compartments;
      m_size = m_size * 2;
    }
    // Assign the compartment to head
    m_compartments[m_count] = compartment;
    // Advance the head
    m_count++;
  }
  // Return the size of the bucket
  return m_size;
}

osc::Compartment * 
osc::CompartmentMap::CompartmentBucket::contains
  ( osc::compartment_id_t const &cID )
{
  DEBUG_STACK_FRAME;
  size_t p;
  // Search over the contents of the Bucket
  for(p = 0; p < m_count; p++)
  {
    // Find a matching compartment
    if( m_compartments[p]->getCompartmentId() == cID )
    {
      /*
      // Swap the found compartment for a compartment nearer to the start
      osc::Compartment * t = m_compartments[p/4];
      m_compartments[p/4] = m_compartments[p];
      m_compartments[p] = t;
      */
      return m_compartments[p];
    }
  }
  return 0;
}

void
osc::CompartmentMap::CompartmentBucket::rehash
  (
    osc::CompartmentMap::CompartmentBucket newBuckets[], 
    osc::u32 mask, 
    size_t top
  )
{
  DEBUG_STACK_FRAME;
  size_t p;
  // Search over the contents of the Bucket
  for(p = 0; p < m_count; p++)
  {
    // Rehash.
    osc::u32 hash = ( m_compartments[p]->getLookup2Hash() ) & mask;
    // Check that the hash will fit in the bucket array
    assert(hash < top);
    // Add the compartment to the new hash
    newBuckets[hash].add(m_compartments[p]);
  } 
}

/**
  Remove the specified compartment from this bucket

  @param index Position of the condemned compartment in this bucket
*/
void
osc::CompartmentMap::CompartmentBucket::remove ( size_t index )
{
  DEBUG_STACK_FRAME;
  // Remove the specified compartment by swapping it in the array for
  // the final element
  assert (index < m_count);
  m_compartments[index] = m_compartments[m_count-1];
  m_count--;
}


/**
  Remove the specified compartment from this bucket

  @param cID Compartment ID of the compartment to remove from this bucket
*/
bool
osc::CompartmentMap::CompartmentBucket::remove 
  ( osc::compartment_id_t const &cID )
{
  DEBUG_STACK_FRAME;
  size_t p;

  // Search over the contents of the Bucket
  for(p = 0; p < m_count; p++)
  {
    // Find a matching compartment
    if( m_compartments[p]->getCompartmentId() == cID )
    {
      remove(p);
      return true;
    }
  }

  // No match in this bucket
  return false;
}

/**
  @todo document this

  @param node   The current tail of the list
  @returns      The new tail of the list
*/
osc::compartment_node_t *
osc::CompartmentMap::CompartmentBucket::getStale
  (osc::compartment_node_t * node)
{
  DEBUG_STACK_FRAME;
  size_t p;
  osc::compartment_node_t * oldNode = node;

  for (p = 0; p < m_count; p++)
  {
    // Find stale compartments
    if( m_compartments[p]->isStale() )
    {
      osc::compartment_node_t * newNode = new osc::compartment_node_t;
      newNode->compartment = m_compartments[p];
      newNode->next = 0;
      oldNode->next = newNode;
      oldNode = newNode;
    }
    else
    {
      m_compartments[p]->setStale(true);
    }
  }

  oldNode->next = 0;
  return oldNode;
}

///////////////////////////////////////////////////////////////////////////
// Debug output routines

#ifdef DEBUG
void
osc::CompartmentMap::dump(std::ostream &os, unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  os << std::setw(indent) << ""
     << "[CompartmentMap " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  maxSizePerC  = " << m_maxStateSpacePerCompartment  << std::endl
     << std::setw(indent) << ""
     << "  bucketCount  = " << m_bucketCount  << std::endl
     << std::setw(indent) << ""
     << "  compartments = " << m_compartments  << std::endl;

  std::ios::fmtflags flags = os.flags();
  char fill = os.fill();
  os << std::hex << std::setfill('0') 
     << std::setw(indent) << ""
     << "  mask         = " << std::setw(8) << m_mask  << std::endl;
  os.fill(fill);
  os.flags(flags);

  os << std::setw(indent) << ""
     << "  threshold    = " << m_rehashThreshold  << std::endl;

  for (size_t i = 0; i < m_bucketCount; i++)
  {
    os << std::setw(indent) << ""
       << "  Bucket #" << i << ":" << std::endl;
    m_buckets[i].dump(os, indent+2);
    os << std::endl;
  }
}

std::ostream &
osc::operator<< (std::ostream &os, const osc::CompartmentMap &cm)
{
  DEBUG_STACK_FRAME;
  cm.dump(os);
  return os;
}

void
osc::CompartmentMap::CompartmentBucket::dump(std::ostream &os, 
                                             unsigned int indent) const
{
  DEBUG_STACK_FRAME;
  os << std::setw(indent) << ""
     << "[CompartmentBucket " << this << "]" << std::endl
     << std::setw(indent) << ""
     << "  size       = " << m_size  << std::endl
     << std::setw(indent) << ""
     << "  count      = " << m_count  << std::endl;

  for (size_t i = 0; i < m_count; i++)
  {
    m_compartments[i]->dump(os,indent+2);
    os << std::endl;
  }
}  
#endif
