#if !defined(TRANSACTIONMAP_HXX)
#define TRANSACTIONMAP_HXX

#include <util/Data.hxx>

#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#include <ext/hash_map>
#else
#include <map>
#endif

namespace Vocal2
{
  class TransactionState;

  class TransactionMap 
  {
     public:
        TransactionState* find( const Data& transactionId ) const;
        void add( const Data& transactionId, TransactionState* state  );
        void remove( const Data& transactionId );
        
     private:
#if ( (__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
        typedef __gnu_cxx::hash_map<Data, TransactionState*> Map;
        Map _map;
#else
        typedef std::map<Data, TransactionState*> Map;
        Map _map;
#endif        
        typedef Map::iterator MapIterator;
        typedef Map::const_iterator MapConstIterator;
  };
}



#endif
