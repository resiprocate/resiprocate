class DataSpecifier
{
   public: 
      KindId kind;
      Generation generation;
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
      const static Uint32 Last = INT_MAX;
      
      class Range
      {
         public:
            Uint32 first;
            Uint32 last;
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
