#ifndef RESIP_RR_OVERLAY_HXX
#define RESIP_RR_OVERLAY_HXX

namespace resip 
{
class Data;
class BaseException;

class RROverlay
{
   friend bool operator<(const RROverlay& r1, const RROverlay& r2)
   {
      if (r1.mType < r2.mType)
      {
         return true;
      }
      if (r1.mType > r2.mType)
      {
         return false;
      }
      return r1.mDomain < r2.mDomain;
   }

   public:
      RROverlay(const unsigned char *aptr, const unsigned char *abuf, int alen);
      
      class OverlayException : public BaseException
      {
         public:
            OverlayException(const Data& msg, const Data& file, const int line)
               : BaseException(msg, file, line) 
            {
            }
            
            const char* name() const { return "OverlayException"; }
      };

      const unsigned char* data() const { return mData; }
      const unsigned char* msg() const { return mMsg; }
      int msgLength() const { return mMsgLen; }
      int dataLength() const { return mDataLen; }
      int nameLength() const { return mNameLen; }
      int ttl() const { return mTTL; }
      int type() const { return mType; }
      const Data& domain() const { return mDomain; }

   private:
      const unsigned char* mData;
      const unsigned char* mMsg;
      int mMsgLen;
      int mDataLen;
      int mNameLen;
      int mTTL;
      int mType; //short?
      Data mDomain; // the domain name this RR refers to.
};

}

#endif
