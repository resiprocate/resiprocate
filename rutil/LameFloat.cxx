#include "rutil/LameFloat.hxx"

#include "rutil/ParseBuffer.hxx"

namespace resip
{
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
      int div=1;
      for(;diff>0;--diff)
      {
         div*=10;
      }

      if(mBase/div == rhs.mBase && mBase%div !=0)
      {
         return mBase < 0;
      }
      else
      {
         return mBase/div < rhs.mBase;
      }
   }
   else
   {
      UInt8 diff=rhs.mNegExp-mNegExp;
      int div=1;
      for(;diff>0;--diff)
      {
         div*=10;
      }

      if(mBase == rhs.mBase/div && rhs.mBase%div !=0)
      {
         return rhs.mBase > 0;
      }
      else
      {
         return mBase < rhs.mBase/div;
      }
   }
}

bool 
LameFloat::operator>(const LameFloat& rhs) const
{
   if(mNegExp >= rhs.mNegExp)
   {
      UInt8 diff=mNegExp-rhs.mNegExp; //1
      int div=1; // 10
      for(;diff>0;--diff)
      {
         div*=10;
      }

      if(mBase/div == rhs.mBase && mBase%div !=0)
      {
         return mBase > 0;
      }
      else
      {
         return mBase/div > rhs.mBase;
      }
   }
   else
   {
      UInt8 diff=rhs.mNegExp-mNegExp;
      int div=1;
      for(;diff>0;--diff)
      {
         div*=10;
      }

      if(mBase == rhs.mBase/div && rhs.mBase%div !=0)
      {
         return rhs.mBase < 0;
      }
      else
      {
         return mBase > rhs.mBase/div;
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
      int div=1;
      for(;diff>0;--diff)
      {
         div*=10;
      }

      return mBase/div == rhs.mBase && mBase%div == 0;
   }
   else
   {
      UInt8 diff=rhs.mNegExp-mNegExp;
      int div=1;
      for(;diff>0;--diff)
      {
         div*=10;
      }

      return mBase == rhs.mBase/div && rhs.mBase%div == 0;
   }
}

} // of namespace resip
