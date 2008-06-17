#ifndef P2P_ArrayValue_hxx
#define P2P_ArrayValue_hxx

#include "rutil/Data.hxx"
#include <vector>

namespace p2p
{

class ArrayValue public: AbstractValue 
{
  public:

   void sign();
   bool isValid() const;

   const Data& get(int i);
   void set(int i, const Data& value);
   void clear(int i);

  protected:
   std::vector<const Data> collectSignableData() const = 0;
   std::vector<SingleValue> mValues;
};

} // p2p
