#if !defined(RESIP_LINKEDHASHMAP_HXX)
#define RESIP_LINKEDHASHMAP_HXX

#include <functional>
#include <iterator>
#include <list>
#include <unordered_map>
#include <utility>

namespace resip
{

/**
   A hash map that iterates in insertion order, so a container filled while
   parsing (eg. XML attributes) encodes back out in the order it was read,
   whatever hasher it uses.  Offers the commonly used subset of the
   std::unordered_map interface; elements are std::pair<const Key, T>.

   Iterators and references stay valid until their element is erased, as with
   std::list.

   @ingroup data_structures
*/
template <class Key, class T, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key> >
class LinkedHashMap
{
   public:
      typedef Key key_type;
      typedef T mapped_type;
      typedef std::pair<const Key, T> value_type;

   private:
      typedef std::list<value_type> List;

   public:
      typedef typename List::size_type size_type;
      typedef typename List::iterator iterator;
      typedef typename List::const_iterator const_iterator;

      LinkedHashMap() {}
      LinkedHashMap(const LinkedHashMap& rhs) : mList(rhs.mList) { reindex(); }
      LinkedHashMap(LinkedHashMap&& rhs) noexcept { swap(rhs); }
      LinkedHashMap& operator=(LinkedHashMap rhs) noexcept { swap(rhs); return *this; }

      iterator begin() { return mList.begin(); }
      iterator end() { return mList.end(); }
      const_iterator begin() const { return mList.begin(); }
      const_iterator end() const { return mList.end(); }
      const_iterator cbegin() const { return mList.begin(); }
      const_iterator cend() const { return mList.end(); }

      bool empty() const { return mList.empty(); }
      size_type size() const { return mList.size(); }

      void clear()
      {
         mIndex.clear();
         mList.clear();
      }

      iterator find(const Key& key)
      {
         typename Index::iterator i = mIndex.find(key);
         return i == mIndex.end() ? mList.end() : i->second;
      }

      const_iterator find(const Key& key) const
      {
         typename Index::const_iterator i = mIndex.find(key);
         return i == mIndex.end() ? mList.end() : const_iterator(i->second);
      }

      size_type count(const Key& key) const { return mIndex.count(key); }

      /// Appends a default-constructed value if key is not present.
      T& operator[](const Key& key)
      {
         iterator i = find(key);
         if (i == mList.end())
         {
            i = append(value_type(key, T()));
         }
         return i->second;
      }

      /// Appends value if its key is not present; otherwise leaves the existing element.
      std::pair<iterator, bool> insert(const value_type& value)
      {
         iterator i = find(value.first);
         if (i != mList.end())
         {
            return std::make_pair(i, false);
         }
         return std::make_pair(append(value), true);
      }

      /// @return the iterator following the erased element
      iterator erase(const_iterator pos)
      {
         mIndex.erase(pos->first);
         return mList.erase(pos);
      }

      size_type erase(const Key& key)
      {
         typename Index::iterator i = mIndex.find(key);
         if (i == mIndex.end())
         {
            return 0;
         }
         mList.erase(i->second);
         mIndex.erase(i);
         return 1;
      }

      void swap(LinkedHashMap& rhs) noexcept
      {
         // std::list::swap keeps iterators valid, so each index still refers to its list
         mList.swap(rhs.mList);
         mIndex.swap(rhs.mIndex);
      }

   private:
      typedef std::unordered_map<Key, iterator, Hash, KeyEqual> Index;

      iterator append(const value_type& value)
      {
         mList.push_back(value);
         iterator i = std::prev(mList.end());
         try
         {
            mIndex.emplace(i->first, i);
         }
         catch (...)
         {
            mList.pop_back();
            throw;
         }
         return i;
      }

      void reindex()
      {
         mIndex.clear();
         mIndex.reserve(mList.size());
         for (iterator i = mList.begin(); i != mList.end(); ++i)
         {
            mIndex.emplace(i->first, i);
         }
      }

      List mList;
      Index mIndex;
};

}

#endif

/* ====================================================================
 *
 * Copyright (c) 2026 SIP Spectrum, Inc. https://www.sipspectrum.com
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 *
 * 3. Neither the name of the author(s) nor the names of any contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR(S) AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR(S) OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * ====================================================================
 */
