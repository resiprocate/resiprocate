
class TransactionMap 
{
public:
  State* find( Data TransactionId );
  void add( Data TransactionId, State*  );
  void delete( Data TransactionId );
};
