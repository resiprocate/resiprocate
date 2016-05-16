#if !defined(RESIP_HASHMAP_HXX)
#define RESIP_HASHMAP_HXX 

// !cj! if all these machine have to use map then we can just delete them and use the default 

/**
   @file
   @brief Wraps your platform's implementation of hash_map, hash_multimap, 
      and hash_set.

   @ingroup data_structures
*/

// For GNU libstdc++ the decision which hashmap implementation to use must be
// done on the version of libstdc++ and not on the compiler version.
// Otherwise for example mixing RHEL6 GCC (4.4.7) and Clang 3.4 on RHEL6 will produce
// ABI incompatible results.
// The version encodings are described here:
//   https://gcc.gnu.org/onlinedocs/libstdc++/manual/abi.html
#include <map> // force include of nonstandard <bits/c++config.h>

#  if ( defined(__GLIBCXX__) && (__GLIBCXX__ >= 20080306) ) // >= 4.3.0
#    include <tr1/unordered_map>
#    include <tr1/unordered_set>
#    define HASH_MAP_NAMESPACE std::tr1
#    define HashMap std::tr1::unordered_map
#    define HashSet std::tr1::unordered_set
#    define HashMultiMap std::tr1::unordered_multimap

#    define HashValue(type)                                \
     namespace std                                         \
     {                                                     \
        namespace tr1                                      \
        {                                                  \
           template <>                                     \
           struct hash<type>                               \
           {                                               \
               size_t operator()(const type& data) const;  \
           };                                              \
        }                                                  \
     }
     #define HashValueImp(type, ret) size_t HASH_MAP_NAMESPACE::hash<type>::operator()(const type& data) const { return ret; }
#  elif ( ( defined(__GLIBCPP__) && (__GLIBCPP__ >= 20020514) ) || ( defined(__GLIBCXX__) && (__GLIBCXX__ >= 20020514) ) || defined(_LIBCPP_VERSION) ) // >= 3.1.0 or libc++
#    include <ext/hash_map>
#    include <ext/hash_set>
#    define HASH_MAP_NAMESPACE __gnu_cxx
#    define HashMap __gnu_cxx::hash_map
#    define HashSet __gnu_cxx::hash_set
#    define HashMultiMap __gnu_cxx::hash_multimap
     // this allows us to hash on a pointer as the key 
     namespace HASH_MAP_NAMESPACE
     { 
        template <class T>
        struct hash<T*>
        {
           size_t operator()(const T* t) const
           {
              return size_t(t);
           }
        };
     }

#    define HashValue(type)                           \
     namespace HASH_MAP_NAMESPACE                     \
     {                                                \
        template <>                                   \
        struct hash<type>                             \
        {                                             \
           size_t operator()(const type& data) const; \
        };                                            \
     }                                   
#    define HashValueImp(type, ret) size_t HASH_MAP_NAMESPACE::hash<type>::operator()(const type& data) const { return ret; }
#  elif  defined(__INTEL_COMPILER )
#    include <hash_map>
#    define HASH_MAP_NAMESPACE std
#    define HashMap std::hash_map
#    define HashSet std::hash_set
#    define HashMultiMap std::hash_multimap
#    define HashValue(type)              \
     namespace HASH_MAP_NAMESPACE        \
     {                                   \
     size_t hash_value(const type& data);\
     }                                   
#    define HashValueImp(type, ret) size_t HASH_MAP_NAMESPACE::hash_value(const type& data) { return ret; }
#  elif  defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1900)  // hash_map in stdext namespace is deprecated for VS2015
#    include <unordered_map>
#    include <unordered_set>
#    define HASH_MAP_NAMESPACE std
#    define HashMap std::unordered_map
#    define HashSet std::unordered_set
#    define HashMultiMap std::unordered_multimap

#    define HashValue(type)                            \
     namespace std                                     \
     {                                                 \
        template <>                                    \
        struct hash<type>                              \
        {                                              \
           size_t operator()(const type& data) const;  \
        };                                             \
     }
#    define HashValueImp(type, ret) size_t HASH_MAP_NAMESPACE::hash<type>::operator()(const type& data) const { return ret; }
#  elif  defined(WIN32) && defined(_MSC_VER) && (_MSC_VER >= 1310)  // hash_map is in stdext namespace for VS.NET 2003
#    include <hash_map>
#    include <hash_set>
#    define HASH_MAP_NAMESPACE stdext
#    define HashMap stdext::hash_map
#    define HashSet stdext::hash_set
#    define HashMultiMap stdext::hash_multimap
#    define HashValue(type)              \
     namespace HASH_MAP_NAMESPACE        \
     {                                   \
     size_t hash_value(const type& data);\
     }                                   
#    define HashValueImp(type, ret) size_t HASH_MAP_NAMESPACE::hash_value(const type& data) { return ret; }
#  else
#    include <map>
#    define HashMap std::map
#    define HashSet std::set
#    define HashMultiMap std::multimap
#    define HashValue(type)    
#    define HashValueImp(type, ret) 
#  endif

#endif

/* ====================================================================
 * The Vovida Software License, Version 1.0 
 * 
 * Copyright (c) 2000 Vovida Networks, Inc.  All rights reserved.
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
 * 3. The names "VOCAL", "Vovida Open Communication Application Library",
 *    and "Vovida Open Communication Application Library (VOCAL)" must
 *    not be used to endorse or promote products derived from this
 *    software without prior written permission. For written
 *    permission, please contact vocal@vovida.org.
 *
 * 4. Products derived from this software may not be called "VOCAL", nor
 *    may "VOCAL" appear in their name, without prior written
 *    permission of Vovida Networks, Inc.
 * 
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESSED OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, TITLE AND
 * NON-INFRINGEMENT ARE DISCLAIMED.  IN NO EVENT SHALL VOVIDA
 * NETWORKS, INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT DAMAGES
 * IN EXCESS OF $1,000, NOR FOR ANY INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 * 
 * ====================================================================
 * 
 * This software consists of voluntary contributions made by Vovida
 * Networks, Inc. and many individuals on behalf of Vovida Networks,
 * Inc.  For more information on Vovida Networks, Inc., please see
 * <http://www.vovida.org/>.
 *
 */
