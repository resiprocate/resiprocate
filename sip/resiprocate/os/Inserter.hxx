#if !defined(RESIP_INSERTER_HXX)
#define RESIP_INSERTER_HXX 

#include <iostream>
#include <utility>
#include <map>
#include <set>
#include "HashMap.hxx"

/**
   Allows a (possibly recursive) container of anything with operator<< to be
   dumped to a stream.

   This is particularly useful within a Log call.
   e.g., assuming vector<Vocal::SipContact> contacts;
   DebugLog(<< "Contacts: " << Inserter(contacts));

   To Do:
   Maybe write insert for a map/hash_map?
 */

namespace resip
{

/// Completely generic insert function
template <class T>
std::ostream&
insert(std::ostream& s, const T& t)
{
   // use native <<
   s << t;
   return s;
}

#if 0 
// !cj! help - can someone look at this - do we really need it 
// it does not compile under windows 

/// Container generic insert function
template <class T, template <class> class C>
std::ostream&
insert(std::ostream& s, const C<T>& c)
{
   s << "[";
   for (typename C<T>::const_iterator i = c.begin();
        i != c.end(); i++) 
   {
      if (i != c.begin()) 
      {
         s << ", ";
      }
      // recurse
      insert(s, *i);
   }
   s << "]";
   return s;
}
#endif

template <class K, class C>
std::ostream&
insert(std::ostream& s, const std::set <K, C>& c)
{
   s << "[";
   for (typename std::set <K, C>::const_iterator i = c.begin();
        i != c.end(); i++) 
   {
      if (i != c.begin()) 
      {
         s << ", ";
      }
      // recurse
      insert(s, *i);
   }
   s << "]";
   return s;
}


// HashMap
#if defined(HASH_MAP_NAMESPACE)
template <class K, class V, class H>
std::ostream&
insert(std::ostream& s, const HashMap<K,V,H>& c)
{
   s << "[";
   for (typename HashMap<K,V,H>::const_iterator i = c.begin();
        i != c.end(); i++) 
   {
      if (i != c.begin()) 
      {
         s << ", ";
      }
      // recurse
      insert(s, i->first);
      s << " -> ";
      insert(s, i->second);      
   }
   s << "]";
   return s;
}
#endif

// map
template <class K, class V, class H>
std::ostream&
insert(std::ostream& s, const std::map <K, V, H>& c)
{
   s << "[";
   for (typename std::map<K,V, H>::const_iterator i = c.begin();
        i != c.end(); i++) 
   {
      if (i != c.begin()) 
      {
         s << ", ";
      }
      // recurse
      insert(s, i->first);
      s << " -> ";
      insert(s, i->second);      
   }
   s << "]";
   return s;
}


// special case for basic_string container
template <class T>
std::ostream&
insert(std::ostream& s, const std::basic_string<T>& str)
{
   // use native <<
   s << str;
   return s;
}

// special case for pair
template <class T, class U>
std::ostream&
insert(std::ostream& s, const std::pair<T, U>& p)
{
   // use native <<
   s << "<" << p.first << ", " << p.second << ">";
   return s;
}

/// Holder of container to be inserted
template <class T>
class InserterClass
{
   public:
      InserterClass(const T& t)
         : _t(t)
      {}
      
      const T& _t;
};

/// Function to allow an Inserter to be used directly with a stream
template <class T>
std::ostream&
operator<<(std::ostream& s, const InserterClass<T>& inserter)
{
   return insert(s, inserter._t);
}

/// Templatized function to construct an instance of InserterClass for a
/// container to be inserted. The function induces the template type, saving the
/// user from thinking about it.
template <class T>
InserterClass<T>
Inserter(const T& t)
{
   return InserterClass<T>(t);
}

/* Example of use
int
main(int argc, char** argv)
{
   std::vector<std::string> v;
   
   v.push_back("foo");
   v.push_back("bar");
   v.push_back("baz");

   std::cerr << Inserter(v) << std::endl;
   std::cerr << Inserter("nnn") << std::endl; // though you wouldn't bother
}
*/
 
} // resip

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
