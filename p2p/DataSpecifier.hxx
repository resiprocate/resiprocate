#ifndef P2P_DataSpecifier_hxx
#define P2P_DataSpecifier_hxx

#include <list>
#include <vector>

namespace p2p 
{

class DataSpecifier
{
   public: 
      KindId kind;
      Generation generation;
      //encoding?
};

class SpecifySingle :public DataSpecifier
{
  public:
};

//default to entire array
class SpecifyArray :public DataSpecifier
{
   public:
      const static uint32_t Last = INT_MAX;
      
      class Range
      {
         public:
            uint32_t first;
            uint32_t last;
      };
      
      std::vector<Range>& ranges();
      const std::vector<Range>& ranges() const;

   private:
      std::vector<Range>& mRange;
};

//empty is entire dictionary, th default
class SpecifyDictionary
{
   public:
      std::vector<resip::Data>& keys();
};
   
// class DeleteReq
// {
//    public:
//       typedef std::list<DataSpecifier> Specifiers;
// };

} // p2p

#endif P2P_DataSpecifier_hxx
