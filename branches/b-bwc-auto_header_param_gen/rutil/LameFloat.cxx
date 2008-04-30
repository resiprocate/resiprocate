#include "rutil/LameFloat.hxx"

#include "rutil/ParseBuffer.hxx"

#include <math.h>

namespace resip
{

const LameFloat LameFloat::lf_min(1,255);
const LameFloat LameFloat::lf_max(LLONG_MAX,0);

LameFloat::LameFloat(const Data& num) :
   mBase(0),
   mNegExp(0)
{
   ParseBuffer pb(num.data(), num.size());
   *this = pb.lameFloat();
}

LameFloat::LameFloat(long long base, UInt8 negExp) :
   mBase(base),
   mNegExp(negExp)
{}

LameFloat::LameFloat(const LameFloat& orig) :
   mBase(orig.mBase),
   mNegExp(orig.mNegExp)
{}

LameFloat& 
LameFloat::operator=(const LameFloat& rhs)
{
   if(this != &rhs)
   {
      mBase = rhs.mBase;
      mNegExp = rhs.mNegExp;
   }
   return *this;
}

bool 
LameFloat::operator<(const LameFloat& rhs) const
{
   if(mNegExp >= rhs.mNegExp)
   {
      UInt8 diff=mNegExp-rhs.mNegExp;
      long long shiftedBase=mBase;
      UInt64 modulus=0;
      for(;diff>0;--diff)
      {
         modulus = modulus*10 + shiftedBase%10;
         shiftedBase/=10;
         if(shiftedBase==0)
         {
            break;
         }
      }

      if(shiftedBase == rhs.mBase && modulus !=0)
      {
         return mBase < 0;
      }
      else
      {
         return shiftedBase < rhs.mBase;
      }
   }
   else
   {
      UInt8 diff=rhs.mNegExp-mNegExp;
      long long shiftedBase=rhs.mBase;
      UInt64 modulus=0;
      for(;diff>0;--diff)
      {
         modulus = modulus*10 + shiftedBase%10;
         shiftedBase/=10;
         if(shiftedBase==0)
         {
            break;
         }
      }

      if(mBase == shiftedBase && modulus !=0)
      {
         return rhs.mBase > 0;
      }
      else
      {
         return mBase < shiftedBase;
      }
   }
}

bool 
LameFloat::operator>(const LameFloat& rhs) const
{
   if(mNegExp >= rhs.mNegExp)
   {
      UInt8 diff=mNegExp-rhs.mNegExp; //1
      long long shiftedBase=mBase;
      UInt64 modulus=0;
      for(;diff>0;--diff)
      {
         modulus = modulus*10 + shiftedBase%10;
         shiftedBase/=10;
         if(shiftedBase==0)
         {
            break;
         }
      }

      if(shiftedBase == rhs.mBase && modulus !=0)
      {
         return mBase > 0;
      }
      else
      {
         return shiftedBase > rhs.mBase;
      }
   }
   else
   {
      UInt8 diff=rhs.mNegExp-mNegExp;
      long long shiftedBase=rhs.mBase;
      UInt64 modulus=0;
      for(;diff>0;--diff)
      {
         modulus = modulus*10 + shiftedBase%10;
         shiftedBase/=10;
         if(shiftedBase==0)
         {
            break;
         }
      }

      if(mBase == shiftedBase && modulus !=0)
      {
         return rhs.mBase < 0;
      }
      else
      {
         return mBase > shiftedBase;
      }
   }
}

bool 
LameFloat::operator==(const LameFloat& rhs) const
{
   if(mNegExp == rhs.mNegExp)
   {
      return mBase == rhs.mBase;
   }
   else if(mNegExp > rhs.mNegExp)
   {
      UInt8 diff=mNegExp-rhs.mNegExp;
      long long shiftedBase=mBase;
      UInt64 modulus=0;
      for(;diff>0;--diff)
      {
         modulus = modulus*10 + shiftedBase%10;
         shiftedBase/=10;
         if(shiftedBase==0)
         {
            break;
         }
      }

      return shiftedBase == rhs.mBase && modulus == 0;
   }
   else
   {
      UInt8 diff=rhs.mNegExp-mNegExp;
      long long shiftedBase=rhs.mBase;
      UInt64 modulus=0;
      for(;diff>0;--diff)
      {
         modulus = modulus*10 + shiftedBase%10;
         shiftedBase/=10;
         if(shiftedBase==0)
         {
            break;
         }
      }

      return mBase == shiftedBase && modulus == 0;
   }
}

#ifndef RESIP_FIXED_POINT

LameFloat::operator double() const
{
   return mBase*pow(10, -mNegExp);
}

#endif

LameFloat 
LameFloat::operator-() const
{
   return LameFloat(-mBase, mNegExp);
}

std::ostream& 
LameFloat::encode(std::ostream& str) const
{
   Data temp(Data::from(mBase < 0 ? -mBase : mBase));
   if(mBase < 0)
   {
      str << '-';
   }

   if(temp.size()<=mNegExp)
   {
      str << "0.";
      short decPlacement = temp.size() - mNegExp;
      while(decPlacement<0)
      {
         str << '0';
         ++decPlacement;
      }
      str << temp;
   }
   else
   {
      str.write(temp.data(), temp.size()-mNegExp);
      if(mNegExp)
      {
         str << '.';
         str.write(temp.data()+(temp.size()-mNegExp), mNegExp);
      }
   }

   return str;
}


std::ostream& operator<<(std::ostream& str, const LameFloat& val)
{
   return val.encode(str);
}

} // of namespace resip
