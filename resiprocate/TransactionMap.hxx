
class TransactionMap 
{
public:
  State* find( String TransactionId );
  void add( String TransactionId, State*  );
  void delete( String TransactionId );
};
