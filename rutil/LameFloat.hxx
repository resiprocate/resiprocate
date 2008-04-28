#ifndef LameFloat_Include_Guard
#define LameFloat_Include_Guard

#include "rutil/Data.hxx"

namespace resip
{

/**
   Class for storing/comparing simple floating point values without using the 
   floating point unit (some embedded devices don't have floating point support)
   The primary motivation for writing this class was to have code that would
   handle numeric caller-caps params, which are free-form floating-point 
   integers.
*/
class LameFloat
{
   public:
      explicit LameFloat(const Data& num);
      LameFloat(long long base, UInt8 negExp);
      LameFloat(const LameFloat& orig);

      LameFloat& operator=(const LameFloat& rhs);

      bool operator<(const LameFloat& rhs) const;
      bool operator>(const LameFloat& rhs) const;
      bool operator==(const LameFloat& rhs) const;
      LameFloat operator-() const;

      inline long long getBase() const { return mBase;} 
      inline void setBase(long long pBase) { mBase = pBase;}
      inline UInt8 getNegExp() const { return mNegExp;} 
      inline void setNegExp(UInt8 pNegExp) { mNegExp = pNegExp;}

      std::ostream& encode(std::ostream& str) const;
      const static LameFloat lf_min;
      const static LameFloat lf_max;

   private:
      long long mBase;
      UInt8 mNegExp;
};

std::ostream& operator<<(std::ostream& str, const LameFloat& val);
}

#endif
