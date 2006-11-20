#include "tfm/predicates/ExpectUtils.hxx"
#include "rutil/Logger.hxx"

#define RESIPROCATE_SUBSYSTEM Cathay::Subsystem::TEST

using namespace std;
using namespace resip;


set<NameAddr> 
mergeContacts(const TestUser& user1)
{
   set<NameAddr> res(user1.getDefaultContacts());
   return res;         
}             

set<NameAddr> 
mergeContacts(const TestUser& user1, const TestUser& user2)      
{
   set<NameAddr> res(user1.getDefaultContacts());
   res.insert(user2.getDefaultContacts().begin(), user2.getDefaultContacts().end());
   return res;         
}             

set<NameAddr> 
mergeContacts(const TestUser& user1, const TestUser& user2, const TestUser& user3)      
{
   set<NameAddr> res(user1.getDefaultContacts());
   res.insert(user2.getDefaultContacts().begin(), user2.getDefaultContacts().end());
   res.insert(user3.getDefaultContacts().begin(), user3.getDefaultContacts().end());
   return res;         
}             

// Copyright 2005 Purplecomm, Inc.
