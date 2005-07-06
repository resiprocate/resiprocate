#if !defined(RESIP_REFCOUNTEDDESTROYER_HXX)
#define RESIP_REFCOUNTEDDESTROYER_HXX

namespace resip
{

//designed to be used by composition, does not destroy itself.
template<class T>
class RefCountedDestroyer
{
   public:            
      class Guard
      {
         public:
            void destroy()
            {
               mRefCountedDestroyer.mDestroying = true;
            }
            
            bool destroyed()
            {
               return mRefCountedDestroyer.mDestroying;
            }

            Guard(RefCountedDestroyer& rcd)
               : mRefCountedDestroyer(rcd)
            {
               mRefCountedDestroyer.mCount++;               
            }
            
            ~Guard()
            {
               mRefCountedDestroyer.mCount--;               
               if (mRefCountedDestroyer.mDestroying && mRefCountedDestroyer.mCount == 0)
               {
                  delete mRefCountedDestroyer.mTarget;
               }
            }
         private:
            RefCountedDestroyer& mRefCountedDestroyer;            
      };      
         
      RefCountedDestroyer(T* target) :
         mTarget(target),
         mCount(0),
         mDestroying(false)
      {
      }  
   private:
      friend class Guard;            
      T* mTarget;
      int mCount;
      bool mDestroying;      
};

}

#endif
