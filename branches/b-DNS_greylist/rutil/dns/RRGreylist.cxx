#if defined(HAVE_CONFIG_H)
#include "rutil/config.hxx"
#endif

#include "ares.h"
#include "ares_dns.h"

#include <map>
#include <list>
#include <vector>
#include <algorithm>

#ifndef WIN32
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#ifndef __CYGWIN__
#  include <netinet/in.h>
#  include <arpa/nameser.h>
#  include <resolv.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#else
#include <Winsock2.h>
#include <svcguid.h>
#ifdef USE_IPV6
#include <ws2tcpip.h>
#endif
#endif

#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/BaseException.hxx"
#include "rutil/dns/DnsResourceRecord.hxx"
#include "rutil/dns/DnsAAAARecord.hxx"
#include "rutil/dns/DnsHostRecord.hxx"
#include "rutil/dns/DnsNaptrRecord.hxx"
#include "rutil/dns/DnsSrvRecord.hxx"
#include "rutil/dns/RRGreylist.hxx"
#include "rutil/WinLeakCheck.hxx"
#include "rutil/Timer.hxx"

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::DNS

using namespace resip;
using namespace std;

time_t RRGreylist::theGreylistDurationMs = 32000;

void
RRGreylist::setGreylistDuration(time_t ms)
{
   theGreylistDurationMs=ms;
}

time_t
RRGreylist::getGreylistDuration()
{
   return theGreylistDurationMs;
}

RRGreylist::RRGreylist()
{
}

RRGreylist::~RRGreylist()
{
   for (map<MapKey, Transform*>::iterator it = mTransforms.begin(); it != mTransforms.end(); ++it)
   {
      delete (*it).second;
   }
}

void RRGreylist::greylist(const Data& target,
                            int rrType,
                            const Data& result)
{
   RRGreylist::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      it->second->add(result);
   }
   else
   {
      Transform* transform = new Transform;
      transform->add(result);
      mTransforms.insert(TransformMap::value_type(key, transform));
   }
}

void RRGreylist::removeGreylist(const Data& target,
                                    int rrType)
{
   RRGreylist::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      delete (*it).second;
      mTransforms.erase(it);
   }
}

void RRGreylist::removeFromGreylist(const Data& target,
                                    int rrType,
                                    const Data& result)
{
   RRGreylist::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      it->second->remove(result);
      if(it->second->isEmpty())
      {
         delete it->second;
         mTransforms.erase(it);
      }
   }
}

void RRGreylist::transform(const Data& target,
                      int rrType,
                      std::vector<DnsResourceRecord*>& src)
{
   RRGreylist::MapKey key(target, rrType);
   TransformMap::iterator it = mTransforms.find(key);
   if (it != mTransforms.end())
   {
      bool shouldRemove = it->second->transform(src);
      if (shouldRemove) 
      {
         removeGreylist(target, rrType);
      }
   }
}

RRGreylist::Transform::Transform() 
{
}

RRGreylist::Transform::~Transform()
{
}

void RRGreylist::Transform::add(const Data& result)
{
   if(!mGreylist.empty())
   {
      for(std::vector<ResultWithExpiry>::iterator i=mGreylist.begin();
            i!=mGreylist.end();++i)
      {
         if(isEqualNoCase(i->result,result))
         {
            // .bwc. Entry is already here, but we do refresh its duration.
            i->expiry=Timer::getTimeMs()+theGreylistDurationMs;
            return;
         }
      }
   }

   // .bwc. A corresponding entry was not found, so insert it.
   mGreylist.push_back(ResultWithExpiry());
   mGreylist.back().result=result;
   mGreylist.back().expiry=Timer::getTimeMs()+theGreylistDurationMs;
}

bool
RRGreylist::Transform::transform(RRVector& src)
{
   time_t now=Timer::getTimeMs();
   RRVector::iterator i;
   RRVector greylisted;
   RRVector notGreylisted;
   for(i=src.begin();i!=src.end();++i)
   {
      if(isGreylisted(*i,now))
      {
         greylisted.push_back(*i);
      }
      else
      {
         notGreylisted.push_back(*i);
      }
   }
   
   // .bwc. If all the results were greylisted, leave src alone. If there were
   // some non-greylisted results remaining, return only those.
   if(!notGreylisted.empty())
   {
      src=notGreylisted;
      for(i=src.begin();i!=src.end();++i)
      {
         delete *i;
      }
   }
   
   return mGreylist.empty();
}

bool
RRGreylist::Transform::isGreylisted(DnsResourceRecord* rr, time_t now)
{
   for(Greylist::iterator j=mGreylist.begin();j!=mGreylist.end();++j)
   {
      if(rr->isSameValue(j->result))
      {
         if(j->expiry > now)
         {
            return true;
         }
         else
         {
            mGreylist.erase(j);
            break;
         }
      }
   }
   
   return false;
}

RRGreylist::MapKey::MapKey()
{
}

RRGreylist::MapKey::MapKey(const Data& target, int rrType)
   : mTarget(target), 
     mRRType(rrType)
{
}

bool RRGreylist::MapKey::operator<(const MapKey& rhs) const
{
   if (mRRType < rhs.mRRType)
   {
      return true;
   }
   else if (mRRType > rhs.mRRType)
   {
      return false;
   }
   else
   {
      return mTarget < rhs.mTarget;
   }
}
