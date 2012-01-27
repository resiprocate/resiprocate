#ifndef CompactStringArray_Include_Guard
#define CompactStringArray_Include_Guard

#include "rutil/Data.hxx"
#include "rutil/HashMap.hxx"

#include <string>
#include <cassert>

namespace resip
{
/**
   A class for storing a fixed-size array of (dynamic sized) strings in a 
   compact form.
*/
template<int S>
class CompactStringArray
{
   public:
      CompactStringArray()
      {
         memset(mOffsets, 0, sizeof(mOffsets));
      }
      ~CompactStringArray(){}

      std::string getString(size_t index) const
      {
         assert(index<=S-1);
         size_t begin = index==0 ? 0 : mOffsets[index-1];
         size_t end = (index==S-1 ? mBuffer.size() : mOffsets[index]);
         return mBuffer.substr(begin, end-begin);
      }

      Data getData(size_t index) const
      {
         assert(index<=S-1);
         size_t begin = index==0 ? 0 : mOffsets[index-1];
         size_t end = (index==S-1 ? mBuffer.size() : mOffsets[index]);
         return Data(mBuffer.data()+begin, end-begin);
      }

      void setString(size_t index, const Data& str)
      {
         setString(index, str.data(), str.size());
      }

      void setString(size_t index,const std::string& str)
      {
         setString(index, str.data(), str.size());
      }

      void setString(size_t index,const char* str, size_t size)
      {
         assert(index<=S-1);
         size_t begin = index==0 ? 0 : mOffsets[index-1];
         size_t end = (index==S-1 ? mBuffer.size() : mOffsets[index]);
         size_t oldSize = end-begin;
         int sizeDiff=size-oldSize;
         mBuffer.replace(begin, oldSize, str, size);
         while(index < S-1)
         {
            mOffsets[index++]+=sizeDiff;
         }
      }

      void reserve(size_t size)
      {
         mBuffer.reserve(size);
      }

      size_t hash() const
      {
         // ?bwc? Do we need to throw the offsets in here? I really doubt we're
         // going to see very many hash collisions due to identical buffers, but
         // different offsets, in practice.
         Data temp(Data::Share, mBuffer.data(), mBuffer.size());
         return temp.hash();
      }

      bool operator==(const CompactStringArray<S>& rhs) const
      {
         return mBuffer == rhs.mBuffer && 
                           !memcmp(mOffsets,rhs.mOffsets,sizeof(mOffsets));
      }

      std::ostream& encode(std::ostream& strm) const
      {
         return strm << mBuffer;
      }
   private:
      std::string mBuffer;
      size_t mOffsets[S-1];
};

template<int N>
inline std::ostream& operator<<(std::ostream& strm, 
                                 const CompactStringArray<N>& d)
{
   return d.encode(strm);
}

}

namespace HASH_MAP_NAMESPACE
{
template <int N>
struct hash<resip::CompactStringArray<N> >
{
   size_t operator()(const resip::CompactStringArray<N>& a) const
   {
      return a.hash();
   }
};

}

#endif
