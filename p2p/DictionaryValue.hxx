#ifndef P2P_DictionayValue_hxx
#define P2P_DictionaryValue_hxx

#include "rutil/Data.hxx"
#include <map>

namespace p2p
{

class DictionaryValue public: AbstractValue 
{
  public:
   DictionaryValue()
      : mMap()
   {}
      
   virtual std::vector<const Data> collectSignableData() const
   {
      assert(0);
   }

//    const resip::Data& get(const resip::Data& key) const;
//    void set(const resip::Data& key, const resip::Data& value);
//    void clear(const resip::Data& key);
   std::map<const Data, const Entry>& dictionary();

  private:
   std::map<const Data, const Entry> mMap;

   class Entry public: Signable {
      Entry(const resip::Data& key, const resip::Data& value)
         : mKey(key),
         mValue(value)
         {}
      
      virtual std::vector<const Data> collectSignableData() const
      {
         std::vector<const Data> result;
         result.add(Data(resip::Data::Borrow, mKey));
         result.add(Data(resip::Data::Borrow, mValue));
         return result;
      }

      bool exists() 
      {
         return mValue == SingleValue::NoValue;
      }
      
     private:
      const resip::Data mKey;
      const resip::Data mValue;
  };
};

} // p2p

#endif // P2P_DictionaryValue_hxx
