#if !defined(TRANSACTIONMAP_HXX)
#define TRANSACTIONMAP_HXX

namespace Vocal2
{

  class State;

  class TransactionMap 
  {
  public:
    State* find( Data& transactionId ) const;
    void add( Data& transactionId, State* state  );
    void remove( Data& transactionId );
  };
  
}

#endif
