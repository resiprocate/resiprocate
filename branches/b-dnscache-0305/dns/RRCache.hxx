#ifndef RESIP_RRCACHE_HXX
#define RESIP_RRCACHE_HXX

#include <set>
#include "resiprocate/os/Data.hxx"
#include "RROverlay.hxx"

using namespace std;

namespace resip
{

class RRCacheBase
{
   public:
      virtual void updateCache(const Data& keyIn, 
                               std::vector<RROverlay>::const_iterator begin, 
                               std::vector<RROverlay>::const_iterator end)=0;
      virtual ~RRCacheBase() {}
};

template<class T>
class RRCache : public RRCacheBase
{
   public:
      typedef typename RRList<T>::LruList LruListType;
      typedef typename RRList<T>::Records Result;

      RRCache() :
         mHead(),
         mLruHead(LruListType::makeList(&mHead))
      {
      }

      ~RRCache()
      {
         clean();
      }
      
      virtual void updateCache(const Data& keyIn, 
                               std::vector<RROverlay>::const_iterator begin, 
                               std::vector<RROverlay>::const_iterator end)
      {

         RRList<T>* key = new RRList<T>(keyIn);         
         typename RRSet::iterator lb = mRRSet.lower_bound(key);
         if (lb != mRRSet.end() &&
             !(mRRSet.key_comp()(key, *lb)))
         {
            (*lb)->update(begin, end);
            touch(*lb);            
         }
         else
         {
            RRList<T>* val = new RRList<T>(keyIn, begin, end);
            mRRSet.insert(lb, val);
            mLruHead->push_back(val);            
         }
         delete key;
      }
                  
      Result lookup(const Data& target)
      {
         RRList<T>* key = new RRList<T>(target);         
         typename RRSet::iterator it = mRRSet.find(key);
         delete key;
         if (it == mRRSet.end())
         {
            return Empty;
         }
         else
         {
            if (Timer::getTimeMs()/1000 >= (*it)->absoluteExpiry())
            {
               delete *it;
               mRRSet.erase(it);
               return Empty;
            }
            else
            {
               touch(*it);               
               return (*it)->records();
            }
         }
      }

      void touch(RRList<T>* node)
      {
         node->remove();
         mLruHead->push_back(node);
      }


   private:
      class CompareT  : public std::binary_function<const RRList<T>*, const RRList<T>*, bool>
      {
         public:
            bool operator()(RRList<T>* lhs, RRList<T>* rhs) const
            {
               return lhs->key() < rhs->key();
            }
      };      

      void clean()
      {
         for (typename set<RRList<T>*, CompareT>::iterator it = mRRSet.begin(); it != mRRSet.end(); it++)
         {
            delete *it;
         }
         mRRSet.clear();
      }

      RRList<T> mHead;
      LruListType* mLruHead;
                  
      typedef std::set<RRList<T>*, CompareT> RRSet;
      RRSet mRRSet;
      Result Empty;
};

}

#endif
