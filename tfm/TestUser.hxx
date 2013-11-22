#ifndef TestUser_hxx
#define TestUser_hxx

#include "tfm/TestSipEndPoint.hxx"

namespace resip
{
class Security;
}

class TestUser : public TestSipEndPoint
{
   public:
      TestUser(const resip::Uri& aor,
               const resip::Data& authName,
               const resip::Data& password, 
               resip::TransportType transport = resip::UDP,
               const resip::Uri& outboundProxy = TestSipEndPoint::NoOutboundProxy,
               const resip::Data& interfaceObj = resip::Data::Empty,
               resip::Security* security=0);
      virtual ~TestUser();
      virtual void clean();

      class TestUserAction : Action
      {
         public:
            explicit TestUserAction(TestUser* endPoint);
            virtual ~TestUserAction();
            using TestEndPoint::Action::operator();
            virtual void operator()();
            virtual void operator()(TestUser& endPoint) = 0;

         protected:
            TestUser* mEndPoint;
      };

      class Register : public TestSipEndPoint::MessageAction
      {
         public:
            Register(TestUser* endPoint,
                     int requestedExpireSecs,
                     const std::set<resip::NameAddr>& contacts,
                     bool doOutbound);

            virtual boost::shared_ptr<resip::SipMessage> go();
            virtual resip::Data toString() const;

         private:
            int mRequestedExpireSecs;
            std::set<resip::NameAddr> mContacts;
            bool mOutbound;
      };
      friend class Register;

      class RemoveAllRegistrationBindings : public TestSipEndPoint::MessageAction
      {
         public:
            RemoveAllRegistrationBindings(TestUser *endPoint);

            virtual boost::shared_ptr<resip::SipMessage> go();
            virtual resip::Data toString() const;
      };
      friend class RemoveAllRegistrationBindings;
      RemoveAllRegistrationBindings* removeRegistrationBindings();
      
      Register* registerUser(int requestedExpireSecs, const std::set<resip::NameAddr>& contacts);
      Register* registerUser(int requestedExpireSecs, const resip::NameAddr& contact);

      Register* registerUserWithOutbound(int requestedExpireSecs, const std::set<resip::NameAddr>& contacts);
      Register* registerUserWithOutbound(int requestedExpireSecs, const resip::NameAddr& contact);

      EXPECT_FUNCTOR(TestUser, DigestRespond);
      MessageExpectAction* digestRespond();
   
      EXPECT_FUNCTOR(TestUser, DigestChallenge);
      MessageExpectAction* digestChallenge();

   protected:
      resip::Data mAuthName;
      resip::Data mPassword;
      boost::shared_ptr<resip::SipMessage> mRegistration;
      unsigned int mNonceCount;
   private:
      // disabled
      TestUser(const TestUser&);
      TestUser& operator=(const TestUser&);
};


#endif
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
