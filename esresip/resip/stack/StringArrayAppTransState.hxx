#ifndef StringArrayAppTransState_Include_Guard
#define StringArrayAppTransState_Include_Guard

#include "resip/stack/AppTransactionState.hxx"

#include "rutil/CompactStringArray.hxx"

namespace resip
{
/**
   Generic app transaction-state class for storing a fixed number of arbitrary 
   buffers of data.
*/
template <int N>
class StringArrayAppTransState : public AppTransactionState
{
   public:
      explicit StringArrayAppTransState(const resip::Data& identifier) : 
         mIdentifier(identifier){}
      virtual ~StringArrayAppTransState(){}

      virtual AppTransactionState* clone() const 
      {
         return new StringArrayAppTransState(*this);
      }

      inline const CompactStringArray<N>& getArray() const { return mArray;} 
      inline CompactStringArray<N>& getArray() { return mArray;} 

      virtual const resip::Data& identifier() const
      {
         return mIdentifier;
      }

   protected:
      const resip::Data mIdentifier;
      CompactStringArray<N> mArray;

   private:
      // disabled
      StringArrayAppTransState();

}; // class StringArrayAppTransState

} // namespace resip

#endif // include guard
