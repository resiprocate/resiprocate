#ifndef P2P_ArrayValue_hxx
#define P2P_ArrayValue_hxx

#include "rutil/Data.hxx"
#include <vector>

namespace p2p
{

class ArrayValue : public AbstractValue 
{
  public:

   void sign();
   bool isValid() const;

   const Data& get(size_t i);
   void set(size_t i, const Data& value);
   void clear(size_t i);

  protected:
   std::vector<const Data> collectSignableData() const = 0;
   std::vector<SingleValue> mValues;
};

} // p2p
