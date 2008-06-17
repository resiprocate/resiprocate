#ifndef P2P_ArrayValue_hxx
#define P2P_ArrayValue_hxx

#include "rutil/Data.hxx"

namespace p2p
{

class SingleValue :public AbstractValue
{
   public:
      static const resip::Data NoValue = "";
      
      SingleValue(const resip::Data& value)
         : mValue(value),
           mExists(true)
      {}

      const Data& get() const 
      {
         if (mExists) 
         {
            return mValue;
         } else {
            return NO_VALUE;
         }
      }
      
      void set(const Data& value) 
      {
         mValue = value;
      }

      void clear() 
      {
         mExists = false;
      }

      bool isSet() const 
      {
         return mExists
      }

   private:
      resip::Data mValue;
      bool mExists;
};

   
