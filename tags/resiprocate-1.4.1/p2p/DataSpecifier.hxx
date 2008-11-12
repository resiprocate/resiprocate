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
      const static UInt32 Last = INT_MAX;
      
      class Range
      {
         public:
            UInt32 first;
            UInt32 last;
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
