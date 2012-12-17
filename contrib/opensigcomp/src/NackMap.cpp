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
  @file NackMap.cpp
  @brief Implementation of osc::NackMap class.
*/


#include "ProfileStack.h"
#include "Libc.h"
#include "Types.h"
#include "Compartment.h"
#include "ReadWriteLockable.h"
#include "NackMap.h"

///////////////////////////////////////////////////////////////////////////
// NackNode Methods

osc::NackMap::NackNode::NackNode(
  osc::Compartment * compartment,osc::sha1_t &sha1,
  NackNode *previous,
  NackNode * next
):
m_compartment(compartment)
{
  DEBUG_STACK_FRAME;
  // Copy the sha1
  OSC_MEMMOVE(m_sha1.digest,sha1.digest,20);
  // Link the node
  m_next = next;
  m_prev = previous;
  // If the compartment is non-NULL
  if(m_compartment)
  { // retain it
    m_compartment->nackRetain();
  }
  if( previous ) // If previous is non-NULL link with it
  { 
    previous->m_next = this;
  }
  if( next ) // If next is non-NULL link with it
  {
    next->m_prev = this;
  }
}

void osc::NackMap::NackNode::unlink()
{
  DEBUG_STACK_FRAME;
  if( m_prev ) // If previous is non-NULL switch its linking
  {
    m_prev->m_next = m_next;
  }
  if( m_next ) // If next is non-NILL switch its linking
  {
    m_next->m_prev = m_prev;
  }
}

osc::NackMap::NackNode::~NackNode()
{
  DEBUG_STACK_FRAME;
  if(m_compartment)
  {
    // Release the compartment! Rawr.
    m_compartment->nackRelease();
  }
}

///////////////////////////////////////////////////////////////////////////
// NackMap Methods

osc::NackMap::NackMap():
  m_bottom(0),m_top(0),m_list(0),m_nacks(0),m_bucketCount(2),m_bucketSize(4),
  m_mask(1)
{
  DEBUG_STACK_FRAME;
  // Allocate a list of node pointers for the buckets
  m_list = new NackNode * [m_bucketCount * m_bucketSize];
  // Allocate new space for the bucket tops
  m_bucketTops = new osc::count_t[m_bucketCount];
  // Clear out the bucket tops
  OSC_MEMSET(m_bucketTops,0,sizeof(osc::count_t)*m_bucketCount);
}

osc::NackMap::NackMap(osc::count_t buckets,
                      osc::count_t bucketSize,
                      osc::u32 mask,
                      osc::NackMap *source):
  m_bottom(0),m_top(0),m_list(0),m_nacks(0),m_bucketCount(buckets),
  m_bucketSize(bucketSize),m_mask(mask)
{
  DEBUG_STACK_FRAME;
  // Allocate a list of node pointers for the buckets
  m_list = new NackNode * [m_bucketCount * m_bucketSize];

  // Allocate new space for the bucket tops
  m_bucketTops = new osc::count_t[m_bucketCount];

  // Clear out the bucket tops
  OSC_MEMSET(m_bucketTops,0,sizeof(osc::count_t)*buckets);

  // If there is a source then copy its contents
  if( source )
  { // Start at the bottom
    NackNode * copyNode = source->m_bottom;
    while( copyNode ) // And iterate upwards
    { // Add each node's info to the new list
      add(copyNode->m_compartment,copyNode->m_sha1);
      copyNode = copyNode->m_next;
    }
  }
  
}

osc::NackMap::~NackMap()
{
  DEBUG_STACK_FRAME;
  for (size_t i = 0; i < m_bucketCount; i++)
  {
    for (size_t j = 0; j < m_bucketTops[i]; j++)
    {
      delete m_list[i*m_bucketSize+j];
    }
  }
  delete[] m_bucketTops;
  delete[] m_list;
}

/**
  Returns a compartment map that contains the provided nack
  and all the nacks contained by this compartment. If it returns
  a new NackMap it will delete the old. So it should be called
  nackMap0 = nackMap0->add(aCompartment, aSHA1);
*/
osc::NackMap * 
osc::NackMap::add(osc::Compartment *compartment,
                  osc::sha1_t &digest)
{
  DEBUG_STACK_FRAME;
    if( compartment )
    { 
      // Get the bucket number for the nack
      osc::u32 hash = getHash(digest);
      osc::u32 bucketTop = m_bucketTops[hash];
      osc::NackMap::NackNode **bucket = &(m_list[hash * m_bucketSize]);

      // Make sure this NACK record isn't a duplicate
      for (size_t i = 0; i < bucketTop; i++)
      {
        if (bucket[i]->m_sha1 == digest)
        {
          return this;
        }
      }

      // Check for space in the bucket
      if( bucketTop >= m_bucketSize ) 
      { 
        // Make a new map if there is not space
        NackMap * newMap = new NackMap(m_bucketCount * 2, 
                                       m_bucketSize + 1,
                                       (m_mask<<1)|1,
                                       this);
        // Delete the old
        delete this;

        // Return the new
        return newMap->add(compartment,digest);
      }

      // Create a new node for the nack
      NackNode * newNode = new NackNode(compartment,digest,m_top,0);

      // Set the top of the list to be the new node
      m_top = newNode;

      // If the bottom has not been set, set it
      if( !m_bottom )
      {
        m_bottom = newNode;
      }

      // Add the node to the list
      bucket[bucketTop] = newNode;

      // Increment the top for that bucket
      m_bucketTops[hash]++;

      // Increment the number of nodes in the list
      m_nacks++; 
    }

    // Return a pointer to the Container that contains that Nack
    return this;
}

void osc::NackMap::removeOldest()
{
  DEBUG_STACK_FRAME;
  // Check top see if there is only one node
  if( m_bottom == m_top)
  {
    // Set the top zero if there is
    m_top = 0;
  }

  // If there is a node in the list
  if(m_bottom)
  {
    // Remove the bottom node
    remove(m_bottom->m_sha1);
  }

}

void osc::NackMap::remove(osc::sha1_t &sha1)
{
  DEBUG_STACK_FRAME;
   // Get the bucket address of the sha1
  osc::u32 hash = getHash(sha1);

   // Get the base of the bucket
  NackNode ** base = &m_list[hash * m_bucketSize];

  // Set the offest from that base to zero
  osc::count_t pos = 0;

  // Find the maximum offset
  osc::count_t top = m_bucketTops[hash];

  // While the offset is less than the max
  while(pos < top)
  { 
    // Check to see if each sha1 matches
    if( base[pos]->m_sha1 == sha1 )
    { 
      // Check to see if that match is the bottom
      if( base[pos] == m_bottom )
      { 
        // Set the new bottom
        m_bottom = m_bottom->m_next;
      }

      //  Remove the node from the list
      base[pos]->unlink();

      // Decrease the top on that bucket
      m_bucketTops[hash]--;

      // Delete the node
      delete base[pos];

      // Decrement the number of nacks stored
      m_nacks--;

      // If it was not the old top
      if( pos != (top-1) )
      {
        // move the old top to fill the gap created
        base[pos] = base[top-1];

        // Relink the swapped in node
        if(base[top-1]->m_next)
        { 
          base[top-1]->m_next->m_prev = base[pos];
        }
        if(base[top-1]->m_prev)
        {
          base[top-1]->m_prev->m_next = base[pos];
        }
      }
      return;
    }
    pos++;
  }  
}

osc::count_t osc::NackMap::getNackCount()
{
  DEBUG_STACK_FRAME;
  return m_nacks; // Return the number of nacks
}

osc::Compartment * osc::NackMap::find(const osc::sha1_t &sha1)
{
  DEBUG_STACK_FRAME;
  osc::u32 hash = getHash(sha1);

  NackNode ** bucket = &(m_list[hash* m_bucketSize]);
  osc::count_t top = m_bucketTops[hash];

  for (osc::count_t pos = 0; pos < top; pos++)
  {
    if (bucket[pos]->m_sha1 == sha1)
    {
      return bucket[pos]->m_compartment;
    }
  }

  // No compartment was found...
  return 0; 
}

osc::u32 osc::NackMap::getHash(const osc::sha1_t &sha1)
{
  DEBUG_STACK_FRAME;
#ifdef UNALIGNED_U32_OKAY
  const osc::u32 *hash = reinterpret_cast<const osc::u32*>(sha1.digest);
  return ((*hash) & m_mask);
#else
  osc::u32 d1 = sha1.digest[1];
  osc::u32 d2 = sha1.digest[2];
  osc::u32 d3 = sha1.digest[3];

  // Bit pack the hash and then take the mask of it 
  osc::u32 hash = ( sha1.digest[0] | (d1<<8) + (d2<<16) + (d3<<24) );
  return (hash & m_mask);
#endif
}
 
#ifdef DEBUG
void osc::NackMap::dump(std::ostream &out, unsigned int indent)
{
  DEBUG_STACK_FRAME;
  NackNode * chainPos = m_bottom;
  out<<"Chain: $->";
  while(chainPos)
  {
    out<<Buffer(chainPos->m_sha1.digest,20,false)<<"->";
    chainPos=chainPos->m_next;
  }
  out<<"0"<<std::endl;
}
#endif
