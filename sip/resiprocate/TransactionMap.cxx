#include <sipstack/Logger.hxx>
#include <sipstack/TransactionMap.hxx>
#include <sipstack/TransactionState.hxx>

using namespace Vocal2;

#define VOCAL_SUBSYSTEM Subsystem::SIP

TransactionState* 
TransactionMap::find( const Data& tid ) const
{
   MapConstIterator i = _map.find(tid);
   if (i != _map.end())
   {
      return i->second;
   }
   else
   {
      return 0;
   }
}
 
void 
TransactionMap::add(const Data& tid, TransactionState* state  )
{
   MapIterator i = _map.find(tid);
   if (i != _map.end())
   {
      DebugLog (<< "Trying to replace an existing transaction id with a new state: " << tid);
      
      delete i->second;
      _map.erase(i);

      _map[tid] = state;
   }
   else
   {
      _map[tid] = state;
   }
}
 
void 
TransactionMap::remove(const Data& tid )
{
   MapIterator i = _map.find(tid);
   if (i != _map.end())
   {
      delete i->second;
      _map.erase(i);
   }
   else
   {
      assert(0);
   }
}
 

