
static const char* const resipId_cxx_version = 
    "@(#) $Id: Id.cxx,v 1.2 2004/04/05 18:54:31 alan Exp $";

#include <typeinfo>
#if defined(RESIP_ID_DEBUG)
# include <typeinfo>
// only works under GCC -- dont use it otherwise.
# include <cxxabi.h>
#endif

// #include "resiprocate/os/Logger.hxx"

#include "resiprocate/os/Id.hxx"
using namespace resip;

namespace resip
{

template <typename D>
Id<D>::Id(D* data)
  : mId(++theGenerator),
    mAuthoritative(true)
{
  // !ah! NOTE: data may not be fully constructed at this point.
  // Don't even think of accessing any of its methods or variables!
  theIdMap.insert( std::pair< value_type, D* > ( mId, data ) );

  // InfoLog(<< mId << " | Id::Id  A | " << myname() << " | " << static_cast<void*>(data));
}

template <typename D>
Id<D>::Id()
  :mId(0),
   mAuthoritative(false)
{
  ;
}

template <typename D>
Id<D>::Id( const Id<D>& other)
{
  *this = other;
  // InfoLog(<< mId << " | Id::Id  N | " << myname() << " | ");
}

template <typename D>
void
Id<D>::operator=(const Id<D>& other)
{
  if (this == &other)
  {
      assert(0);
      return;
  }
  mId = other.mId;
  mAuthoritative = false;
}

template <typename D>
Id<D>::~Id()
{
  if (mId)
  {
    if (mAuthoritative)
    {
        theIdMap.erase(mId);
    }

    // InfoLog( << mId << " | Id::~Id " << (mAuthoritative?'A':'N') << " | " << myname() );

  }
}

template <typename D>
const typename Id<D>::value_type
Id<D>::value() const
{
  return mId;
}

template <typename D>
D*
Id<D>::operator->() const
{
  typename map_type::iterator found = theIdMap.find(mId);
  return found->second;
}

template <typename D>
D&
Id<D>::operator*() const
{
  typename map_type::iterator found = theIdMap.find(mId);
  assert(found != theIdMap.end());
  return *(found->second);
}

template <typename D>
bool
Id<D>::valid() const
{
  if (mId == 0) return false;

  typename map_type::iterator found = theIdMap.find(mId);

  return found != theIdMap.end();

}

#if !defined(WIN32) && defined(__GXX_ABI_VERSION__) && (__GXX_ABI_VERSION >= 102)
# define DO_ABI
#endif

#if defined(DO_ABI)
# include <typeinfo>
# include <cxxabi.h>
#endif

template <typename D>
const Data&
Id<D>::myname() const
{
#if defined(RESIP_ID_DEBUG)
    if (mName == Data::Empty)
    {
        char buffer[1024];
        int status = 1;

#       if defined(DO_ABI)
          unsigned int len = 1024;
          using namespace __cxxabiv1;
#       endif

        using namespace std;
        string msg;
        string n = typeid(this).name();

#       if defined(DO_ABI)
          __cxa_demangle(n.c_str(), buffer, &len, &status);
#       endif

        if (status)
            msg += n;
        else
            msg += buffer;

        mName = Data(msg);
    }
#endif        
    return mName;
}

#if defined(DO_ABI)
# undef DO_ABI
#endif

// You MAY need to fwd declare all your id instantiations here...
// eg)
// template Id<Dialog>;


#if defined(WIN32)
// !kk! VC++ wants explicit definitions for static template members. 
// Should figure out how to do one generic (templatized) definition
// for each such member.
// eg)
// Id<Dialog>::value_type Id<Dialog>::theGenerator = 0;
// Id<Dialog>::map_type Id<Dialog>::theIdMap; 
#endif

};
