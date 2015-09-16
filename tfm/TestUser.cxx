#include "cppunit/TestCase.h"
#include "resip/stack/Helper.hxx"
#include "resip/stack/SipStack.hxx"
#include "rutil/DnsUtil.hxx"
#include "rutil/Logger.hxx"
#include "tfm/PortAllocator.hxx"
#include "tfm/Resolver.hxx"
#include "tfm/Sequence.hxx"
#include "tfm/TestUser.hxx"

using namespace resip;
using namespace std;
using namespace boost;

#define RESIPROCATE_SUBSYSTEM resip::Subsystem::TEST

Uri
CreateContact(const resip::Data& username,
              const Data& transport,
              const Data& interfaceObj)
{
   Uri contact;
   contact.user() = username;
   // !jf! should be able to do dns or ip
   // contacts may be DNS
   //static Data localIp = resip::DnsUtil::getLocalIpAddress();
   contact.host() = (interfaceObj.empty() 
                           ? PortAllocator::getNextLocalIpAddress()
                           : interfaceObj);
   contact.port() = PortAllocator::getNextPort();
   contact.param(p_transport) = transport;
   DebugLog (<< "Creating contact: " << contact);
   return contact;
}

TestUser::TestUser(const resip::Uri& aor,
                   const resip::Data& authName,
                   const resip::Data& password,
                   resip::TransportType transport,
                   const resip::Uri& outboundProxy,
                   const resip::Data& interfaceObj,
                   Security* security)
   : TestSipEndPoint(aor, 
                     CreateContact(aor.user(), Tuple::toData(transport), interfaceObj), 
                     outboundProxy, 
                     true, 
                     interfaceObj,
                     security),
     mAuthName(authName),
     mPassword(authName),
     mRegistration(Helper::makeRegister(NameAddr(getAddressOfRecord()), 
                                        NameAddr(getAddressOfRecord()),
                                        mContact)),
     mNonceCount(0)
{
   DebugLog (<< "Creating user " << getContact());
}

TestUser::~TestUser()
{
   unregisterFromTransportDriver();
   clean();
}
     
void TestUser::clean()
{
   TestSipEndPoint::clean();
   //InfoLog (<< "Cleaning user: " << *mRegistration);
   mRegistration->header(h_ProxyAuthorizations).clear();
   mRegistration->remove(h_ProxyAuthenticates);
   resip_assert(!mRegistration->exists(h_ProxyAuthenticates));
   
   mRegistration->header(h_Authorizations).clear();
   mRegistration->remove(h_Authorizations);
   resip_assert(!mRegistration->exists(h_Authorizations));
   //InfoLog (<< "Cleaned user: " << *mRegistration);
}

TestUser::TestUserAction::TestUserAction(TestUser* endPoint)
   : mEndPoint(endPoint)
{
}

TestUser::TestUserAction::~TestUserAction() 
{
}

void 
TestUser::TestUserAction::operator()()
{
   return (*this)(*mEndPoint);
}


boost::shared_ptr<SipMessage>
TestUser::DigestRespond::go(boost::shared_ptr<SipMessage> response)
{
   resip_assert(response->isResponse());
   DebugLog (<< "DigestRespond: " << Inserter(mEndPoint.mRequests));
   
   resip_assert(mEndPoint.mRequests.find(response->header(h_Vias).front().param(p_branch).getTransactionId()) != mEndPoint.mRequests.end());
   boost::shared_ptr<SipMessage> request = mEndPoint.mRequests[response->header(h_Vias).front().param(p_branch).getTransactionId()];
   Helper::addAuthorization(*request, *response, mEndPoint.mAuthName, mEndPoint.mPassword, "foo", mEndPoint.mNonceCount);

   // increment cseq
   request->header(h_CSeq).sequence()++;

   // recompute the branch each time
   request->header(h_Vias).front().remove(p_branch);
   request->header(h_Vias).front().param(p_branch);

   return request;
}

boost::shared_ptr<SipMessage>
TestUser::DigestChallenge::go(boost::shared_ptr<SipMessage> request)
{
   resip_assert(request->isRequest());
   // !jf! should be a UA challenge 401
   boost::shared_ptr<SipMessage> challenge(Helper::makeProxyChallenge(*request, request->header(h_To).uri().host()));;
   return challenge;
}

TestUser::Register::Register(TestUser* endPoint,
                             int requestedExpireSecs,
                             const std::set<resip::NameAddr>& contacts,
                             bool doOutbound)
   : TestSipEndPoint::MessageAction(*endPoint, endPoint->getAddressOfRecord()),
     mRequestedExpireSecs(requestedExpireSecs),
     mContacts(contacts),
     mOutbound(doOutbound)
{
}

boost::shared_ptr<SipMessage>
TestUser::Register::go()
{
   boost::shared_ptr<SipMessage> reg = static_cast<TestUser&>(mEndPoint).mRegistration;

   // increment cseq
   reg->header(h_CSeq).sequence()++;

   // recompute the branch each time
   reg->header(h_Vias).front().remove(p_branch);
   reg->header(h_Vias).front().param(p_branch);

   reg->header(h_Expires).value() = mRequestedExpireSecs;
   reg->header(h_Contacts).clear();

   int regid=1;
   for (set<NameAddr>::const_iterator i = mContacts.begin();
        i != mContacts.end(); i++)
   {
      reg->header(h_Contacts).push_back(*i);
      if(mOutbound)
      {
         reg->header(h_Contacts).back().param(p_Instance)=Data::from(&mEndPoint);
         reg->header(h_Contacts).back().param(p_regid)=regid++;
      }
   }

   //Copy is made to prevent conditions from corrupting mRegistration
   boost::shared_ptr<SipMessage> copy(dynamic_cast<SipMessage*>(reg->clone()));
   
   return copy;
}

resip::Data
TestUser::Register::toString() const
{
   return mEndPoint.getName() + ".register(" + Data(mRequestedExpireSecs) + ")";
}

TestUser::Register* 
TestUser::registerUser(int requestedExpireSecs,
                       const std::set<resip::NameAddr>& contacts)
{
   return new Register(this, requestedExpireSecs, contacts, false);
}

TestUser::Register* 
TestUser::registerUser(int requestedExpireSecs,
                       const resip::NameAddr& contact)
{
   std::set<resip::NameAddr> contacts;
   contacts.insert(contact);
   return registerUser(requestedExpireSecs, contacts);
}

TestUser::Register* 
TestUser::registerUserWithOutbound(int requestedExpireSecs, const std::set<resip::NameAddr>& contacts)
{
   return new Register(this, requestedExpireSecs, contacts, true);
}

TestUser::Register* 
TestUser::registerUserWithOutbound(int requestedExpireSecs, const resip::NameAddr& contact)
{
   std::set<resip::NameAddr> contacts; 
   contacts.insert(contact);
   return registerUserWithOutbound(requestedExpireSecs, contacts);
}

TestSipEndPoint::MessageExpectAction* 
TestUser::digestRespond()
{
   return new DigestRespond(*this);
}

TestSipEndPoint::MessageExpectAction* 
TestUser::digestChallenge()
{
   return new DigestChallenge(*this);
}


TestUser::RemoveAllRegistrationBindings::RemoveAllRegistrationBindings(TestUser *endPoint)
   : TestSipEndPoint::MessageAction(*endPoint, endPoint->getAddressOfRecord())
{
}


boost::shared_ptr<SipMessage>
TestUser::RemoveAllRegistrationBindings::go()
{
   boost::shared_ptr<SipMessage> reg = static_cast<TestUser&>(mEndPoint).mRegistration;

   reg->header(h_CSeq).sequence()++;

   // recompute the branch each time !jf! why here?
   reg->header(h_Vias).front().param(p_branch).reset();
   reg->header(h_Expires).value() = 0;
   reg->header(h_Contacts).clear();
   NameAddr allContacts;
   allContacts.setAllContacts();
   reg->header(h_Contacts).clear();
   reg->header(h_Contacts).push_back(allContacts);
   return reg;
}

resip::Data
TestUser::RemoveAllRegistrationBindings::toString() const
{
   return mEndPoint.getName() + ".RemoveAllRegistrationBindings()";
}

TestUser::RemoveAllRegistrationBindings*
TestUser::removeRegistrationBindings()
{
   return new RemoveAllRegistrationBindings(this);
}



// Copyright 2005 Purplecomm, Inc.
/*
  Copyright (c) 2005, PurpleComm, Inc. 
  All rights reserved.

  Redistribution and use in source and binary forms, with or without modification,
  are permitted provided that the following conditions are met:

  * Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
  * Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.
  * Neither the name of PurpleComm, Inc. nor the names of its contributors may
    be used to endorse or promote products derived from this software without
    specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
