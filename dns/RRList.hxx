#ifndef RESIP_DNS_RR_LIST
#define RESIP_DNS_RR_LIST

#include <vector>

#include "resiprocate/os/Timer.hxx"
#include "resiprocate/os/Data.hxx"
#include "resiprocate/os/IntrusiveListElement.hxx"


namespace resip
{

template<class T>
class RRList : public IntrusiveListElement<RRList<T>*> 
{
   public:      
      typedef std::vector<T> Records;
      typedef IntrusiveListElement<RRList<T>*> LruList;

      RRList() {}

      explicit RRList(const Data& key)
         : mKey(key)
      {}

      template<class Iter> RRList(const Data& key, Iter begin, Iter end)
         : mKey(key)
      {
         update(begin, end);
      }
      
      template<class Iter> 
      void update(Iter begin, Iter end)
      {
         mRecords.clear();
         mAbsoluteExpiry = ULLONG_MAX;
         for (Iter it = begin; it != end; it++)
         {            
            mRecords.push_back(T(*it));
            if ((unsigned long)it->ttl() < mAbsoluteExpiry) 
            {
               mAbsoluteExpiry = it->ttl();
            }
         }
         mAbsoluteExpiry += Timer::getTimeMs()/1000;
      }

      const Records& records() const
      {
         return mRecords;
      }

      const Data& key() { return mKey; } const
      
      UInt64 absoluteExpiry() const { return mAbsoluteExpiry; }
      UInt64& absoluteExpiry() { return mAbsoluteExpiry; }
   protected:

   private:
      Data mKey;
      //unsigned long mAbsoluteExpiry;
      UInt64 mAbsoluteExpiry;
      Records mRecords;

};

}


#endif
