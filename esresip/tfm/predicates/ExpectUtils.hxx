#ifndef ExpectUtils_hxx_
#define ExpectUtils_hxx_

#include <set>
#include "tfm/CountDown.hxx"
#include "tfm/CheckContacts.hxx"
#include "tfm/CheckFetchedContacts.hxx"
#include "tfm/TestUser.hxx"

std::set<resip::NameAddr> mergeContacts(const TestUser& user1);
std::set<resip::NameAddr> mergeContacts(const TestUser& user1, 
                                        const TestUser& user2);
std::set<resip::NameAddr> mergeContacts(const TestUser& user1, 
                                        const TestUser& user2, 
                                        const TestUser& user3);


#endif

// Copyright 2005 Purplecomm, Inc.
