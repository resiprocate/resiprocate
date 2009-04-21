#ifndef P2P_FetchKind_hxx
#define P2P_FetchKind_hxx

#include "p2p.hxx"

namespace p2p
{

class DataSpecifier
{
   public: 
     KindId kind;
      //encoding?
};

class SpecifySingle : public FetchKind
{
  public:
};

//defualt to entire array
class SpecifyArray : public FetchKind
{
   public:
      class RangeEntry
      {
         public:
            size_t index;
            bool isEnd();
      };
      class Range
      {
         public:
            RangeEntry first;
            RangeEntry last;
      };
      
      void addRange(const RangeEntry& r);
      
      vector<Range>& ranges();
      const vector<Range>& ranges() const;
};

//empty is entire dictionary, th default
class SpecifyDictionary
{
   public:
      std::vector<Data>& keys();
};
   
class FetchReq
{
   public:
      //resoure
      typedef std::list<DataSpecifier> Specifiers;
};
   
class DeleteReq
{
   public:
      typedef std::list<DataSpecifier> Specifiers;
};

} // p2p

#endif // P2P_FetchKind_hxx

//this is boring, no special foo required...stay close to defn.
//are these potential replicas or committed replicas? Seems that timing would be
//an issue.     
/*
struct {
      KindId                  kind;
      uint64                  generation_counter;
      NodeId                  replicas<0..2^16-1>;
} StoreKindResponse;


struct {
      StoreKindResponse       kind_responses<0..2^16-1>;
} StoreAns;
*/
