// Copyright 2002 Cathay Networks, Inc. 

#ifndef Inserter_hxx
#define Inserter_hxx

#include <iostream>
#include <utility>
#include <map>
#include <set>
#include <ext/hash_map>

/**
   Allows a (possibly recursive) container of anything with operator<< to be
   dumped to a stream.

   This is particularly useful within a Log call.
   e.g., assuming vector<Vocal::SipContact> contacts;
   DebugLog(<< "Contacts: " << Inserter(contacts));

   To Do:
   Maybe write insert for a map/hash_map?
 */

namespace Vocal2
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

#if ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1) )
#define HM __gnu_cxx::hash_map<K,V,H>
#else
#define HM std::hash_map<K,V,H>
#endif


// hash_map
template <class K, class V, class H>
std::ostream&
insert(std::ostream& s, const HM& c)
{
   s << "[";
   for (typename HM::const_iterator i = c.begin();
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

#undef HM

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
 
} // Vocal2

#endif // ContainerInsert_hxx

